/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */

#include "input_capture.h"

#include <cstdlib>
#include <cstring>

namespace
{
    const char * const CloverconGuid = "180000004e696e74656e646f20436c00";
    constexpr int AxisThreshold = 16000;
    constexpr int AxisRestTolerance = 8000;
    constexpr Uint32 ButtonGraceMs = 120;
    constexpr Uint32 SettleMs = 250;

    bool IsClovercon(int deviceIndex)
    {
        char guid[64];
        SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(deviceIndex), guid, sizeof(guid));
        return std::strcmp(guid, CloverconGuid) == 0;
    }
}

InputCapture::InputCapture()
{
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    SDL_JoystickEventState(SDL_ENABLE);
}

InputCapture::~InputCapture()
{
    CloseDevice();
    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

void InputCapture::OpenDevice(int deviceIndex)
{
    if(joystick_ || !IsClovercon(deviceIndex))
        return;
    joystick_ = SDL_JoystickOpen(deviceIndex);
    if(joystick_)
        instance_ = SDL_JoystickInstanceID(joystick_);
}

void InputCapture::CloseDevice()
{
    if(joystick_)
        SDL_JoystickClose(joystick_);
    joystick_ = nullptr;
    instance_ = -1;
    state_ = State::Idle;
}

void InputCapture::HandleEvent(const SDL_Event & event)
{
    switch(event.type)
    {
    case SDL_JOYDEVICEADDED:
        OpenDevice(event.jdevice.which);
        return;
    case SDL_JOYDEVICEREMOVED:
        if(event.jdevice.which == instance_)
            CloseDevice();
        return;
    default:
        break;
    }
    if(state_ != State::Waiting || !joystick_)
        return;

    if(event.type == SDL_JOYBUTTONDOWN && event.jbutton.which == instance_)
        Accept("b" + std::to_string(event.jbutton.button));
    else if(event.type == SDL_JOYHATMOTION && event.jhat.which == instance_ && event.jhat.value != SDL_HAT_CENTERED)
        Accept("h" + std::to_string(event.jhat.hat) + "." + std::to_string(event.jhat.value));
    else if(event.type == SDL_JOYAXISMOTION && event.jaxis.which == instance_ && pendingAxis_.empty()
            && event.jaxis.axis < axisRest_.size() && std::abs(event.jaxis.value - axisRest_[event.jaxis.axis]) > AxisThreshold)
    {
        pendingAxis_ = "a" + std::to_string(event.jaxis.axis);
        pendingAxisAt_ = SDL_GetTicks();
    }
}

void InputCapture::Update()
{
    if(state_ == State::Waiting && !pendingAxis_.empty() && SDL_GetTicks() - pendingAxisAt_ >= ButtonGraceMs)
        Accept(pendingAxis_);
    if(state_ != State::Settling)
        return;
    if(!AtRest())
        restSince_ = 0;
    else if(restSince_ == 0)
        restSince_ = SDL_GetTicks();
    else if(SDL_GetTicks() - restSince_ >= SettleMs)
        state_ = State::Idle;
}

void InputCapture::Begin()
{
    result_.clear();
    pendingAxis_.clear();
    axisRest_.clear();
    if(!joystick_)
        return;
    for(int i = 0; i < SDL_JoystickNumAxes(joystick_); ++i)
        axisRest_.push_back(SDL_JoystickGetAxis(joystick_, i));
    state_ = State::Waiting;
}

void InputCapture::Cancel()
{
    pendingAxis_.clear();
    state_ = State::Idle;
}

void InputCapture::Accept(const std::string & binding)
{
    result_ = binding;
    pendingAxis_.clear();
    restSince_ = 0;
    state_ = State::Settling;
}

bool InputCapture::AtRest() const
{
    if(!joystick_)
        return true;
    for(int i = 0; i < SDL_JoystickNumButtons(joystick_); ++i)
        if(SDL_JoystickGetButton(joystick_, i))
            return false;
    for(int i = 0; i < SDL_JoystickNumHats(joystick_); ++i)
        if(SDL_JoystickGetHat(joystick_, i) != SDL_HAT_CENTERED)
            return false;
    for(size_t i = 0; i < axisRest_.size(); ++i)
        if(std::abs(SDL_JoystickGetAxis(joystick_, static_cast<int>(i)) - axisRest_[i]) > AxisRestTolerance)
            return false;
    return true;
}

bool InputCapture::TakeResult(std::string & binding)
{
    if(state_ != State::Idle || result_.empty())
        return false;
    binding = result_;
    result_.clear();
    return true;
}
