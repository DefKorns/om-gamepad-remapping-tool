/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */

#include "pad_map.h"

#include <sys/stat.h>

#include <cstdio>
#include <fstream>

namespace
{
    std::string MapFileName(const InputNode & pad)
    {
        char name[32];
        std::snprintf(name, sizeof(name), "%04x_%04x.map", pad.vendor, pad.product);
        return name;
    }

    bool Exists(const std::string & path)
    {
        struct stat info;
        return stat(path.c_str(), &info) == 0;
    }
}

std::string PadMapPath(const InputNode & pad)
{
    return std::string(PadMapDir) + MapFileName(pad);
}

std::string PadProfilePath(const InputNode & pad)
{
    return std::string(PadProfileDir) + MapFileName(pad);
}

std::string ActivePadMapPath(const InputNode & pad)
{
    if(Exists(PadMapPath(pad)))
        return PadMapPath(pad);
    if(Exists(PadProfilePath(pad)))
        return PadProfilePath(pad);
    return std::string();
}

bool LoadPadMap(const std::string & path, GamepadMapping & mapping)
{
    std::ifstream in(path);
    if(!in)
        return false;
    std::string line;
    while(std::getline(in, line))
    {
        if(!line.empty() && line.back() == '\r')
            line.pop_back();
        const size_t equals = line.find('=');
        if(equals != std::string::npos && equals > 0)
            mapping.Set(line.substr(0, equals), line.substr(equals + 1));
    }
    return true;
}

bool SavePadMap(const std::string & path, const GamepadMapping & mapping)
{
    mkdir(PadMapDir, 0755);
    const std::string tempPath = path + ".tmp";
    {
        std::ofstream out(tempPath, std::ios::trunc);
        for(const auto & field : mapping.Fields())
            out << field.first << '=' << field.second << '\n';
        if(!out)
            return false;
    }
    return std::rename(tempPath.c_str(), path.c_str()) == 0;
}
