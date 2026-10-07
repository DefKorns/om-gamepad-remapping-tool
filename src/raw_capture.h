/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */

#ifndef RAW_CAPTURE_H_
#define RAW_CAPTURE_H_

#include "capture_source.h"
#include "input_nodes.h"

#include <linux/input.h>

#include <string>
#include <vector>

class RawCapture : public CaptureSource
{
public:
    explicit RawCapture(const InputNode & pad);
    ~RawCapture() override;
    RawCapture(const RawCapture &) = delete;
    RawCapture & operator=(const RawCapture &) = delete;

    void Update() override;
    bool HasDevice() const override { return fd_ >= 0; }
    void Begin() override;
    void Cancel() override;
    bool Waiting() const override { return state_ == State::Waiting; }
    bool TakeResult(std::string & binding) override;
    const std::string & ResultSource() const override { return name_; }

private:
    enum class State { Idle, Waiting, Settling };

    void Handle(const input_event & event);
    void Accept(const std::string & binding);
    float Deviation(int code, int value) const;
    bool AtRest() const;

    int fd_ = -1;
    std::string name_;
    State state_ = State::Idle;
    std::vector<input_absinfo> abs_;
    std::vector<int> rest_;
    std::string pendingAxis_;
    unsigned int pendingAxisAt_ = 0;
    std::string result_;
    unsigned int acceptedAt_ = 0;
    unsigned int restSince_ = 0;
};

#endif
