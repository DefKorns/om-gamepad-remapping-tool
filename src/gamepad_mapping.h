/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */

#ifndef GAMEPAD_MAPPING_H_
#define GAMEPAD_MAPPING_H_

#include <string>
#include <utility>
#include <vector>

struct MappingTarget
{
    std::string field;
    std::string label;
    bool translateLabel;
    bool extra;
};

const std::vector<MappingTarget> & MappingTargets();

class GamepadMapping
{
public:
    static GamepadMapping Parse(const std::string & line);
    static GamepadMapping Defaults();

    std::string Get(const std::string & field) const;
    void Set(const std::string & field, const std::string & binding);
    std::string ToLine() const;
    bool operator==(const GamepadMapping & other) const;
    bool operator!=(const GamepadMapping & other) const { return !(*this == other); }

private:
    std::vector<std::pair<std::string, std::string>> fields_;
};

GamepadMapping LoadActiveMapping(const std::string & dbPath);
bool WriteMappingDb(const std::string & baseDbPath, const std::string & outPath, const GamepadMapping & mapping);

#endif
