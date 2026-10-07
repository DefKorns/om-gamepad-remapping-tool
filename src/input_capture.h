/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */

#ifndef INPUT_CAPTURE_H_
#define INPUT_CAPTURE_H_

#include "capture_source.h"

#include <SDL.h>

#include <map>
#include <string>
#include <vector>

class InputCapture : public CaptureSource
{
public:
    InputCapture();
    ~InputCapture() override;
    InputCapture(const InputCapture &) = delete;
    InputCapture & operator=(const InputCapture &) = delete;

    void HandleEvent(const SDL_Event & event);
    void Update() override;

    bool HasDevice() const override { return !pads_.empty(); }
    void Begin() override;
    void Cancel() override;
    bool Waiting() const override { return state_ == State::Waiting; }
    bool TakeResult(std::string & binding) override;
    const std::string & ResultSource() const override { return resultSource_; }

private:
    enum class State { Idle, Waiting, Settling };

    struct Pad
    {
        SDL_Joystick * joystick;
        std::vector<Sint16> axisRest;
    };

    void OpenDevice(int deviceIndex);
    void CloseDevice(SDL_JoystickID instance);
    void Accept(const std::string & binding, SDL_JoystickID instance);
    bool AtRest() const;

    std::map<SDL_JoystickID, Pad> pads_;
    State state_ = State::Idle;
    std::string pendingAxis_;
    SDL_JoystickID pendingAxisPad_ = -1;
    Uint32 pendingAxisAt_ = 0;
    std::string result_;
    std::string resultSource_;
    Uint32 restSince_ = 0;
    Uint32 acceptedAt_ = 0;
};

#endif
