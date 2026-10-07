/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */

#include "input_nodes.h"

#include <dirent.h>
#include <limits.h>
#include <stdlib.h>

#include <algorithm>
#include <fstream>

namespace
{
    const char * const ClassDir = "/sys/class/input/";
    const std::string CloverconPrefix = "Nintendo Clovercon";
    constexpr unsigned int UsbBus = 0x03;

    std::string ReadLine(const std::string & path)
    {
        std::ifstream in(path);
        std::string line;
        std::getline(in, line);
        return line;
    }

    unsigned int ReadHex(const std::string & path)
    {
        return static_cast<unsigned int>(std::strtoul(ReadLine(path).c_str(), nullptr, 16));
    }

    int InputNumberOf(const std::string & devicePath)
    {
        const size_t at = devicePath.rfind("/input");
        return at == std::string::npos ? -1 : std::atoi(devicePath.c_str() + at + 6);
    }

    bool HasAbsAxes(const std::string & deviceDir)
    {
        const std::string abs = ReadLine(deviceDir + "capabilities/abs");
        return !abs.empty() && abs != "0";
    }
}

std::vector<InputNode> ListInputNodes()
{
    std::vector<InputNode> nodes;
    DIR * dir = opendir(ClassDir);
    if(!dir)
        return nodes;
    while(dirent * entry = readdir(dir))
    {
        const std::string node = entry->d_name;
        if(node.compare(0, 5, "event") != 0)
            continue;
        const std::string deviceDir = ClassDir + node + "/device/";
        char resolved[PATH_MAX];
        if(!realpath(deviceDir.c_str(), resolved))
            continue;
        const std::string devicePath = resolved;
        if(!HasAbsAxes(deviceDir))
            continue;
        nodes.push_back(InputNode{ node, ReadLine(deviceDir + "name"), InputNumberOf(devicePath),
                                   ReadHex(deviceDir + "id/bustype"), ReadHex(deviceDir + "id/vendor"), ReadHex(deviceDir + "id/product"),
                                   devicePath.find("/devices/virtual/") != std::string::npos });
    }
    closedir(dir);
    std::sort(nodes.begin(), nodes.end(), [](const InputNode & a, const InputNode & b) { return a.inputNumber < b.inputNumber; });
    return nodes;
}

bool IsClovercon(const InputNode & node)
{
    return node.name.compare(0, CloverconPrefix.size(), CloverconPrefix) == 0;
}

bool IsUsbPad(const InputNode & node)
{
    return node.bus == UsbBus && !IsClovercon(node);
}

std::vector<InputNode> UsbPads(const std::vector<InputNode> & nodes)
{
    std::vector<InputNode> pads;
    std::copy_if(nodes.begin(), nodes.end(), std::back_inserter(pads), IsUsbPad);
    return pads;
}

const InputNode * CloverconFor(const InputNode & pad, const std::vector<InputNode> & nodes)
{
    std::vector<const InputNode *> taken;
    for(const InputNode & other : nodes)
    {
        if(!IsUsbPad(other))
            continue;
        const InputNode * match = nullptr;
        for(const InputNode & candidate : nodes)
            if(IsClovercon(candidate) && candidate.isVirtual && candidate.inputNumber > other.inputNumber
               && std::find(taken.begin(), taken.end(), &candidate) == taken.end())
            {
                match = &candidate;
                break;
            }
        if(match)
            taken.push_back(match);
        if(other.node == pad.node)
            return match;
    }
    return nullptr;
}

std::string DevicePath(const InputNode & node)
{
    return "/dev/input/" + node.node;
}
