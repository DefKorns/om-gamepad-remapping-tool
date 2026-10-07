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

#include <SDL.h>

#include <string>
#include <vector>

class InputCapture
{
public:
    InputCapture();
    ~InputCapture();
    InputCapture(const InputCapture &) = delete;
    InputCapture & operator=(const InputCapture &) = delete;

    void HandleEvent(const SDL_Event & event);
    void Update();

    bool HasDevice() const { return joystick_ != nullptr; }
    void Begin();
    void Cancel();
    bool Active() const { return state_ != State::Idle; }
    bool Waiting() const { return state_ == State::Waiting; }
    bool TakeResult(std::string & binding);

private:
    enum class State { Idle, Waiting, Settling };

    void OpenDevice(int deviceIndex);
    void CloseDevice();
    void Accept(const std::string & binding);
    bool AtRest() const;

    SDL_Joystick * joystick_ = nullptr;
    SDL_JoystickID instance_ = -1;
    State state_ = State::Idle;
    std::vector<Sint16> axisRest_;
    std::string pendingAxis_;
    Uint32 pendingAxisAt_ = 0;
    std::string result_;
    Uint32 restSince_ = 0;
};

#endif
