/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */

#include "pad_translator.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <fstream>

namespace
{
    const int CloverconKeys[] = { 304, 305, 307, 308, 310, 311, 312, 313, 314, 315, 316, 704, 705, 706, 707 };
    constexpr int CloverconKeyCount = sizeof(CloverconKeys) / sizeof(CloverconKeys[0]);
    constexpr int AbsCount = ABS_MAX + 1;
    constexpr float DirectionThreshold = 0.5f;
    constexpr int KeyRepeat = 2;
    constexpr int HomeKey = 316;
    constexpr long HomeComboDisabled = 0x7FFF;
    const char * const HomeComboParam = "/sys/module/clovercon/parameters/home_combination";

    bool HomeComboEnabled()
    {
        std::ifstream in(HomeComboParam);
        long value = HomeComboDisabled;
        return (in >> value) && value != HomeComboDisabled;
    }

    std::vector<input_absinfo> ReadAbs(int fd)
    {
        std::vector<input_absinfo> abs(AbsCount);
        for(int axis = 0; axis < AbsCount; ++axis)
            if(ioctl(fd, EVIOCGABS(axis), &abs[axis]) < 0)
                std::memset(&abs[axis], 0, sizeof(input_absinfo));
        return abs;
    }

    int Scale(int value, const input_absinfo & from, const input_absinfo & to)
    {
        if(from.maximum == from.minimum)
            return to.minimum;
        return to.minimum + static_cast<int>(static_cast<long>(value - from.minimum) * (to.maximum - to.minimum) / (from.maximum - from.minimum));
    }
}

PadTranslator::PadTranslator(const InputNode & pad, const InputNode & target, const GamepadMapping & padMap, const GamepadMapping & frontMap)
{
    source_ = open(DevicePath(pad).c_str(), O_RDONLY | O_NONBLOCK);
    const int targetQuery = open(DevicePath(target).c_str(), O_RDONLY | O_NONBLOCK);
    target_ = open(DevicePath(target).c_str(), O_WRONLY | O_NONBLOCK);
    if(source_ < 0 || targetQuery < 0 || target_ < 0)
    {
        if(targetQuery >= 0)
            close(targetQuery);
        return;
    }
    homeComboEnabled_ = HomeComboEnabled();
    sourceAbs_ = ReadAbs(source_);
    targetAbs_ = ReadAbs(targetQuery);
    close(targetQuery);
    for(const auto & field : padMap.Fields())
        AddRule(field.first, field.second, frontMap);
    if(ioctl(source_, EVIOCGRAB, 1) < 0)
    {
        close(source_);
        source_ = -1;
    }
}

PadTranslator::~PadTranslator()
{
    if(source_ >= 0)
    {
        ioctl(source_, EVIOCGRAB, 0);
        close(source_);
    }
    if(target_ >= 0)
        close(target_);
}

void PadTranslator::AddRule(const std::string & field, const std::string & source, const GamepadMapping & frontMap)
{
    std::string binding = frontMap.Get(field);
    if(binding.empty())
        binding = GamepadMapping::Defaults().Get(field);
    if(binding.size() < 2 || source.size() < 2)
        return;

    Rule rule{ SourceKind::Key, 0, 0, binding[0] == 'b', 0, false, field == "dpdown", field == "back" };
    const int index = std::atoi(binding.c_str() + 1);
    if(rule.toKey)
    {
        if(index < 0 || index >= CloverconKeyCount)
            return;
        rule.targetCode = CloverconKeys[index];
    }
    else if(binding[0] == 'a')
        rule.targetCode = index;
    else
        return;

    switch(source[0])
    {
    case 'k':
        rule.kind = SourceKind::Key;
        rule.code = std::atoi(source.c_str() + 1);
        break;
    case 'a':
        rule.code = std::atoi(source.c_str() + 1);
        rule.direction = source.back() == '+' ? 1 : source.back() == '-' ? -1 : 0;
        rule.kind = rule.direction == 0 ? SourceKind::Axis : SourceKind::AxisDirection;
        break;
    case 'h':
    {
        if(source.size() < 4)
            return;
        const int hat = std::atoi(source.c_str() + 1);
        const char axis = source[source.size() - 2];
        rule.kind = SourceKind::Hat;
        rule.code = ABS_HAT0X + 2 * hat + (axis == 'y' ? 1 : 0);
        rule.direction = source.back() == '+' ? 1 : -1;
        break;
    }
    default:
        return;
    }
    rules_.push_back(rule);
}

float PadTranslator::Normalized(int code, int value) const
{
    const input_absinfo & range = sourceAbs_[code];
    if(range.maximum == range.minimum)
        return 0.0f;
    return 2.0f * (value - range.minimum) / static_cast<float>(range.maximum - range.minimum) - 1.0f;
}

void PadTranslator::Emit(int type, int code, int value)
{
    input_event event;
    std::memset(&event, 0, sizeof(event));
    event.type = static_cast<__u16>(type);
    event.code = static_cast<__u16>(code);
    event.value = value;
    if(write(target_, &event, sizeof(event)) < 0)
        return;
}

void PadTranslator::Apply(const input_event & event)
{
    for(Rule & rule : rules_)
    {
        bool pressed = rule.pressed;
        if(event.type == EV_KEY && rule.kind == SourceKind::Key && event.code == rule.code)
        {
            if(event.value == KeyRepeat)
                continue;
            pressed = event.value != 0;
        }
        else if(event.type == EV_ABS && event.code == rule.code && event.code < AbsCount)
        {
            if(rule.kind == SourceKind::Axis)
            {
                if(!rule.toKey)
                    Emit(EV_ABS, rule.targetCode, Scale(event.value, sourceAbs_[event.code], targetAbs_[rule.targetCode]));
                continue;
            }
            if(rule.kind == SourceKind::AxisDirection)
                pressed = rule.direction * Normalized(event.code, event.value) > DirectionThreshold;
            else if(rule.kind == SourceKind::Hat)
                pressed = rule.direction * event.value > 0;
            else
                continue;
        }
        else
            continue;

        if(pressed == rule.pressed)
            continue;
        rule.pressed = pressed;
        if(rule.toKey)
            Emit(EV_KEY, rule.targetCode, pressed ? 1 : 0);
        else
            Emit(EV_ABS, rule.targetCode, pressed ? targetAbs_[rule.targetCode].maximum : targetAbs_[rule.targetCode].minimum);
        if(rule.comboDown || rule.comboSelect)
            UpdateHomeCombo();
    }
}

void PadTranslator::UpdateHomeCombo()
{
    if(!homeComboEnabled_)
        return;
    bool down = false;
    bool select = false;
    for(const Rule & rule : rules_)
    {
        down = down || (rule.comboDown && rule.pressed);
        select = select || (rule.comboSelect && rule.pressed);
    }
    const bool combo = down && select;
    if(combo == homeActive_)
        return;
    homeActive_ = combo;
    Emit(EV_KEY, HomeKey, combo ? 1 : 0);
}

void PadTranslator::Pump()
{
    input_event events[16];
    ssize_t bytes;
    while((bytes = read(source_, events, sizeof(events))) > 0)
    {
        const int count = static_cast<int>(bytes / sizeof(input_event));
        for(int i = 0; i < count; ++i)
        {
            if(events[i].type == EV_SYN)
                Emit(EV_SYN, SYN_REPORT, 0);
            else
                Apply(events[i]);
        }
    }
}
