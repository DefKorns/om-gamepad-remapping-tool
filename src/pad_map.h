/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */

#ifndef PAD_MAP_H_
#define PAD_MAP_H_

#include "gamepad_mapping.h"
#include "input_nodes.h"

#include <string>

constexpr const char * PadMapDir = "/etc/options_menu/inputs/pads/";
constexpr const char * PadProfileDir = "/etc/options_menu/inputs/profiles/";
constexpr const char * PadMapperPauseFlag = "/tmp/pad_mapper.pause";

std::string PadMapPath(const InputNode & pad);
std::string PadProfilePath(const InputNode & pad);
std::string ActivePadMapPath(const InputNode & pad);
bool LoadPadMap(const std::string & path, GamepadMapping & mapping);
bool SavePadMap(const std::string & path, const GamepadMapping & mapping);

#endif
