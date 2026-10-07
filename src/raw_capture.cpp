/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */

#include "raw_capture.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cmath>
#include <cstring>
#include <ctime>

namespace
{
    constexpr int AbsCount = ABS_MAX + 1;
    constexpr int KeyBytes = (KEY_MAX + 8) / 8;
    constexpr float AxisThreshold = 0.5f;
    constexpr float RestTolerance = 0.25f;
    constexpr unsigned int ButtonGraceMs = 120;
    constexpr unsigned int SettleMs = 250;
    constexpr unsigned int MaxSettleMs = 1500;

    unsigned int NowMs()
    {
        timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        return static_cast<unsigned int>(now.tv_sec) * 1000u + static_cast<unsigned int>(now.tv_nsec / 1000000);
    }

    bool IsHat(int code)
    {
        return code >= ABS_HAT0X && code <= ABS_HAT3Y;
    }
}

RawCapture::RawCapture(const InputNode & pad)
    : name_(pad.name)
    , abs_(AbsCount)
    , rest_(AbsCount, 0)
{
    fd_ = open(DevicePath(pad).c_str(), O_RDONLY | O_NONBLOCK);
    if(fd_ < 0)
        return;
    for(int code = 0; code < AbsCount; ++code)
        if(ioctl(fd_, EVIOCGABS(code), &abs_[code]) < 0)
            std::memset(&abs_[code], 0, sizeof(input_absinfo));
}

RawCapture::~RawCapture()
{
    if(fd_ >= 0)
        close(fd_);
}

float RawCapture::Deviation(int code, int value) const
{
    const input_absinfo & range = abs_[code];
    if(range.maximum == range.minimum)
        return 0.0f;
    return static_cast<float>(value - rest_[code]) / static_cast<float>(range.maximum - range.minimum);
}

void RawCapture::Begin()
{
    result_.clear();
    pendingAxis_.clear();
    if(fd_ < 0)
        return;
    input_event events[32];
    while(read(fd_, events, sizeof(events)) > 0)
        continue;
    for(int code = 0; code < AbsCount; ++code)
    {
        input_absinfo info;
        rest_[code] = ioctl(fd_, EVIOCGABS(code), &info) == 0 ? info.value : 0;
    }
    state_ = State::Waiting;
}

void RawCapture::Cancel()
{
    pendingAxis_.clear();
    state_ = State::Idle;
}

void RawCapture::Accept(const std::string & binding)
{
    result_ = binding;
    pendingAxis_.clear();
    acceptedAt_ = NowMs();
    restSince_ = 0;
    state_ = State::Settling;
}

void RawCapture::Handle(const input_event & event)
{
    if(state_ != State::Waiting)
        return;
    if(event.type == EV_KEY && event.value == 1)
        Accept("k" + std::to_string(event.code));
    else if(event.type == EV_ABS && event.code < AbsCount)
    {
        if(IsHat(event.code))
        {
            if(event.value == 0)
                return;
            const int hat = (event.code - ABS_HAT0X) / 2;
            const char axis = (event.code - ABS_HAT0X) % 2 ? 'y' : 'x';
            Accept("h" + std::to_string(hat) + axis + (event.value > 0 ? "+" : "-"));
        }
        else if(pendingAxis_.empty())
        {
            const float deviation = Deviation(event.code, event.value);
            if(std::fabs(deviation) > AxisThreshold)
            {
                pendingAxis_ = "a" + std::to_string(event.code) + (deviation > 0 ? "+" : "-");
                pendingAxisAt_ = NowMs();
            }
        }
    }
}

bool RawCapture::AtRest() const
{
    unsigned char keys[KeyBytes];
    std::memset(keys, 0, sizeof(keys));
    if(ioctl(fd_, EVIOCGKEY(sizeof(keys)), keys) >= 0)
        for(unsigned char byte : keys)
            if(byte)
                return false;
    for(int code = 0; code < AbsCount; ++code)
    {
        input_absinfo info;
        if(ioctl(fd_, EVIOCGABS(code), &info) != 0 || abs_[code].maximum == abs_[code].minimum)
            continue;
        if(IsHat(code) ? info.value != 0 : std::fabs(Deviation(code, info.value)) > RestTolerance)
            return false;
    }
    return true;
}

void RawCapture::Update()
{
    if(fd_ < 0)
        return;
    input_event events[32];
    ssize_t bytes;
    while((bytes = read(fd_, events, sizeof(events))) > 0)
        for(int i = 0; i < static_cast<int>(bytes / sizeof(input_event)); ++i)
            Handle(events[i]);

    if(state_ == State::Waiting && !pendingAxis_.empty() && NowMs() - pendingAxisAt_ >= ButtonGraceMs)
        Accept(pendingAxis_);
    if(state_ != State::Settling)
        return;
    if(NowMs() - acceptedAt_ >= MaxSettleMs)
        state_ = State::Idle;
    else if(!AtRest())
        restSince_ = 0;
    else if(restSince_ == 0)
        restSince_ = NowMs();
    else if(NowMs() - restSince_ >= SettleMs)
        state_ = State::Idle;
}

bool RawCapture::TakeResult(std::string & binding)
{
    if(state_ != State::Idle || result_.empty())
        return false;
    binding = result_;
    result_.clear();
    return true;
}
