/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */

#ifndef PAD_PROFILES_H_
#define PAD_PROFILES_H_

#include <string>

std::string FrontBindingLabel(const std::string & binding);
std::string UsbSourceLabel(unsigned int vendor, unsigned int product, const std::string & source);

#endif
