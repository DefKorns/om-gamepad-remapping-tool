/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */

#ifndef CAPTURE_SOURCE_H_
#define CAPTURE_SOURCE_H_

#include <string>

class CaptureSource
{
public:
    virtual ~CaptureSource() = default;
    virtual void Update() = 0;
    virtual bool HasDevice() const = 0;
    virtual void Begin() = 0;
    virtual void Cancel() = 0;
    virtual bool Waiting() const = 0;
    virtual bool TakeResult(std::string & binding) = 0;
    virtual const std::string & ResultSource() const = 0;
};

#endif
