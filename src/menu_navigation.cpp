/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */

#include "menu_navigation.h"

#include <cstdlib>
#include <vector>

namespace
{
    std::vector<std::string> Split(const std::string & text, char separator)
    {
        std::vector<std::string> parts;
        size_t start = 0;
        while(start <= text.size())
        {
            size_t end = text.find(separator, start);
            if(end == std::string::npos)
                end = text.size();
            parts.push_back(text.substr(start, end - start));
            start = end + 1;
        }
        return parts;
    }
}

std::string BuildReturnCommand(const std::string & optionsLocation)
{
    const char * stackEnv = std::getenv("OM_BACK_STACK");
    const std::string stack = stackEnv ? stackEnv : "";
    const std::string launcher = "usleep 50000 && ";
    if(stack.empty())
        return launcher + optionsLocation + "options --commandPath " + optionsLocation + "controller/commands/ --scriptPath " + optionsLocation + "controller/scripts/ --title \"CONTROLLER_OPTIONS\" &";

    std::vector<std::string> entries = Split(stack, ';');
    const std::vector<std::string> parent = Split(entries.back(), ',');
    entries.pop_back();
    std::string remaining;
    for(size_t i = 0; i < entries.size(); ++i)
        remaining += (i ? ";" : "") + entries[i];

    const std::string path = parent.size() > 0 ? parent[0] : optionsLocation + "controller/commands/";
    const std::string scriptPath = parent.size() > 1 ? parent[1] : "";
    const std::string titleKey = parent.size() > 2 ? parent[2] : "CONTROLLER_OPTIONS";
    return launcher + "OM_BACK_STACK=\"" + remaining + "\" " + optionsLocation + "options --commandPath " + path
        + (scriptPath.empty() ? "" : " --scriptPath " + scriptPath) + " --title \"" + titleKey + "\" &";
}
