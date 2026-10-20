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
    constexpr Uint32 MaxSettleMs = 1500;

    bool IsClovercon(int deviceIndex)
    {
        char guid[64];
        SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(deviceIndex), guid, sizeof(guid));
        return std::strcmp(guid, CloverconGuid) == 0;
    }

    std::vector<Sint16> AxisPositions(SDL_Joystick * joystick)
    {
        std::vector<Sint16> positions;
        for(int i = 0; i < SDL_JoystickNumAxes(joystick); ++i)
            positions.push_back(SDL_JoystickGetAxis(joystick, i));
        return positions;
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
    for(auto & pad : pads_)
        SDL_JoystickClose(pad.second.joystick);
    pads_.clear();
    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

void InputCapture::OpenDevice(int deviceIndex)
{
    if(!IsClovercon(deviceIndex))
        return;
    SDL_Joystick * joystick = SDL_JoystickOpen(deviceIndex);
    if(!joystick)
        return;
    const SDL_JoystickID instance = SDL_JoystickInstanceID(joystick);
    if(pads_.count(instance))
    {
        SDL_JoystickClose(joystick);
        return;
    }
    pads_[instance] = Pad{ joystick, AxisPositions(joystick) };
}

void InputCapture::CloseDevice(SDL_JoystickID instance)
{
    const auto pad = pads_.find(instance);
    if(pad == pads_.end())
        return;
    SDL_JoystickClose(pad->second.joystick);
    pads_.erase(pad);
    if(pads_.empty())
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
        CloseDevice(event.jdevice.which);
        return;
    default:
        break;
    }
    if(state_ != State::Waiting)
        return;

    if(event.type == SDL_JOYBUTTONDOWN && pads_.count(event.jbutton.which))
        Accept("b" + std::to_string(event.jbutton.button), event.jbutton.which);
    else if(event.type == SDL_JOYHATMOTION && pads_.count(event.jhat.which) && event.jhat.value != SDL_HAT_CENTERED)
        Accept("h" + std::to_string(event.jhat.hat) + "." + std::to_string(event.jhat.value), event.jhat.which);
    else if(event.type == SDL_JOYAXISMOTION && pendingAxis_.empty())
    {
        const auto pad = pads_.find(event.jaxis.which);
        if(pad == pads_.end() || event.jaxis.axis >= pad->second.axisRest.size())
            return;
        if(std::abs(event.jaxis.value - pad->second.axisRest[event.jaxis.axis]) > AxisThreshold)
        {
            pendingAxis_ = "a" + std::to_string(event.jaxis.axis);
            pendingAxisPad_ = event.jaxis.which;
            pendingAxisAt_ = SDL_GetTicks();
        }
    }
}

void InputCapture::Update()
{
    if(state_ == State::Waiting && !pendingAxis_.empty() && SDL_GetTicks() - pendingAxisAt_ >= ButtonGraceMs)
        Accept(pendingAxis_, pendingAxisPad_);
    if(state_ != State::Settling)
        return;
    if(SDL_GetTicks() - acceptedAt_ >= MaxSettleMs)
        state_ = State::Idle;
    else if(!AtRest())
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
    if(pads_.empty())
        return;
    for(auto & pad : pads_)
        pad.second.axisRest = AxisPositions(pad.second.joystick);
    state_ = State::Waiting;
}

void InputCapture::Cancel()
{
    pendingAxis_.clear();
    state_ = State::Idle;
}

void InputCapture::Accept(const std::string & binding, SDL_JoystickID instance)
{
    const auto pad = pads_.find(instance);
    const char * name = pad == pads_.end() ? nullptr : SDL_JoystickName(pad->second.joystick);
    resultSource_ = name ? name : "";
    result_ = binding;
    pendingAxis_.clear();
    restSince_ = 0;
    acceptedAt_ = SDL_GetTicks();
    state_ = State::Settling;
}

bool InputCapture::AtRest() const
{
    for(const auto & entry : pads_)
    {
        const Pad & pad = entry.second;
        for(int i = 0; i < SDL_JoystickNumButtons(pad.joystick); ++i)
            if(SDL_JoystickGetButton(pad.joystick, i))
                return false;
        for(int i = 0; i < SDL_JoystickNumHats(pad.joystick); ++i)
            if(SDL_JoystickGetHat(pad.joystick, i) != SDL_HAT_CENTERED)
                return false;
        for(size_t i = 0; i < pad.axisRest.size(); ++i)
            if(std::abs(SDL_JoystickGetAxis(pad.joystick, static_cast<int>(i)) - pad.axisRest[i]) > AxisRestTolerance)
                return false;
    }
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
