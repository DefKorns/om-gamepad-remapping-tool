/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */

#include "gamepad_mapping.h"

#include <algorithm>
#include <cstdio>
#include <fstream>

namespace
{
    const std::string CloverconGuid = "180000004e696e74656e646f20436c00";
    const std::string CloverconName = "Nintendo Clovercon";
    const std::string PlatformField = "platform";
    const std::string StockLine = "180000004e696e74656e646f20436c00,Nintendo Clovercon,leftx:a0,lefty:a1,rightx:a3,righty:a4,dpup:b13,dpdown:b14,dpleft:b11,dpright:b12,a:b0,b:b1,y:b3,x:b2,back:b8,start:b9,guide:b10,leftshoulder:b4,leftshoulder:a2,rightshoulder:b5,rightshoulder:a5,lefttrigger:b6,righttrigger:b7,platform:Linux,";

    std::vector<std::string> SplitFields(const std::string & line)
    {
        std::vector<std::string> parts;
        size_t start = 0;
        while(start < line.size())
        {
            size_t comma = line.find(',', start);
            if(comma == std::string::npos)
                comma = line.size();
            if(comma > start)
                parts.push_back(line.substr(start, comma - start));
            start = comma + 1;
        }
        return parts;
    }

    bool IsCloverconLine(const std::string & line)
    {
        return line.compare(0, CloverconGuid.size(), CloverconGuid) == 0;
    }
}

const std::vector<MappingTarget> & MappingTargets()
{
    static const std::vector<MappingTarget> targets = {
        { "dpup", "GP_UP", true, false },
        { "dpdown", "GP_DOWN", true, false },
        { "dpleft", "GP_LEFT", true, false },
        { "dpright", "GP_RIGHT", true, false },
        { "a", "A", false, false },
        { "b", "B", false, false },
        { "x", "X", false, false },
        { "y", "Y", false, false },
        { "leftshoulder", "L", false, false },
        { "rightshoulder", "R", false, false },
        { "back", "Select", false, false },
        { "start", "Start", false, false },
        { "lefttrigger", "ZL", false, true },
        { "righttrigger", "ZR", false, true },
        { "guide", "Home", false, true },
        { "leftx", "GP_LEFT_STICK_X", true, true },
        { "lefty", "GP_LEFT_STICK_Y", true, true },
        { "rightx", "GP_RIGHT_STICK_X", true, true },
        { "righty", "GP_RIGHT_STICK_Y", true, true },
    };
    return targets;
}

GamepadMapping GamepadMapping::Parse(const std::string & line)
{
    GamepadMapping mapping;
    const std::vector<std::string> parts = SplitFields(line);
    for(size_t i = 2; i < parts.size(); ++i)
    {
        const size_t colon = parts[i].find(':');
        if(colon == std::string::npos)
            continue;
        std::string field = parts[i].substr(0, colon);
        if(field != PlatformField)
            mapping.fields_.emplace_back(std::move(field), parts[i].substr(colon + 1));
    }
    return mapping;
}

GamepadMapping GamepadMapping::Defaults()
{
    return Parse(StockLine);
}

std::string GamepadMapping::Get(const std::string & field) const
{
    const auto it = std::find_if(fields_.begin(), fields_.end(), [&](const std::pair<std::string, std::string> & f) { return f.first == field; });
    return it == fields_.end() ? std::string() : it->second;
}

void GamepadMapping::Set(const std::string & field, const std::string & binding)
{
    const auto first = std::find_if(fields_.begin(), fields_.end(), [&](const std::pair<std::string, std::string> & f) { return f.first == field; });
    const auto position = first - fields_.begin();
    fields_.erase(std::remove_if(fields_.begin(), fields_.end(), [&](const std::pair<std::string, std::string> & f) { return f.first == field; }), fields_.end());
    if(binding.empty())
        return;
    const auto insertAt = fields_.begin() + std::min<std::ptrdiff_t>(position, static_cast<std::ptrdiff_t>(fields_.size()));
    fields_.emplace(insertAt, field, binding);
}

std::string GamepadMapping::ToLine() const
{
    std::string line = CloverconGuid + "," + CloverconName + ",";
    for(const auto & f : fields_)
        line += f.first + ":" + f.second + ",";
    return line + PlatformField + ":Linux,";
}

bool GamepadMapping::operator==(const GamepadMapping & other) const
{
    return fields_ == other.fields_;
}

GamepadMapping LoadActiveMapping(const std::string & dbPath)
{
    std::ifstream in(dbPath);
    std::string line;
    while(std::getline(in, line))
    {
        if(!line.empty() && line.back() == '\r')
            line.pop_back();
        if(IsCloverconLine(line))
            return GamepadMapping::Parse(line);
    }
    return GamepadMapping::Defaults();
}

bool WriteMappingDb(const std::string & baseDbPath, const std::string & outPath, const GamepadMapping & mapping)
{
    std::vector<std::string> lines;
    {
        std::ifstream in(baseDbPath);
        std::string line;
        while(std::getline(in, line))
        {
            if(!line.empty() && line.back() == '\r')
                line.pop_back();
            lines.push_back(line);
        }
    }
    const auto clovercon = std::find_if(lines.begin(), lines.end(), IsCloverconLine);
    if(clovercon == lines.end())
        lines.push_back(mapping.ToLine());
    else
        *clovercon = mapping.ToLine();

    const std::string tempPath = outPath + ".tmp";
    {
        std::ofstream out(tempPath, std::ios::trunc);
        for(const std::string & line : lines)
            out << line << '\n';
        if(!out)
            return false;
    }
    return std::rename(tempPath.c_str(), outPath.c_str()) == 0;
}
