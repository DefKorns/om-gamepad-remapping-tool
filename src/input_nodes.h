/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */

#ifndef INPUT_NODES_H_
#define INPUT_NODES_H_

#include <string>
#include <vector>

struct InputNode
{
    std::string node;
    std::string name;
    int inputNumber;
    unsigned int bus;
    unsigned int vendor;
    unsigned int product;
    bool isVirtual;
};

std::vector<InputNode> ListInputNodes();
bool IsClovercon(const InputNode & node);
bool IsUsbPad(const InputNode & node);
std::vector<InputNode> UsbPads(const std::vector<InputNode> & nodes);
const InputNode * CloverconFor(const InputNode & pad, const std::vector<InputNode> & nodes);
std::string DevicePath(const InputNode & node);

#endif
