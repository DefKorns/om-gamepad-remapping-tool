/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */

#include "pad_profiles.h"
#include "localization.h"

#include <linux/input.h>

#include <cstdlib>
#include <map>
#include <utility>

namespace
{
    struct Name
    {
        const char * text;
        bool translate;
    };

    struct Model
    {
        unsigned int vendor;
        unsigned int product;
        std::map<int, Name> keys;
        std::map<int, Name> axes;
        bool axisDpad;
    };

    std::string Text(const Name & name)
    {
        return name.translate ? Translate(name.text) : name.text;
    }

    const std::map<int, Name> & FrontButtons()
    {
        static const std::map<int, Name> names = {
            { 0, { "A", false } }, { 1, { "B", false } }, { 2, { "X", false } }, { 3, { "Y", false } },
            { 4, { "L", false } }, { 5, { "R", false } }, { 6, { "ZL", false } }, { 7, { "ZR", false } },
            { 8, { "Select", false } }, { 9, { "Start", false } }, { 10, { "Home", false } },
            { 11, { "GP_LEFT", true } }, { 12, { "GP_RIGHT", true } }, { 13, { "GP_UP", true } }, { 14, { "GP_DOWN", true } },
        };
        return names;
    }

    const std::map<int, Name> & FrontAxes()
    {
        static const std::map<int, Name> names = {
            { 0, { "GP_LEFT_STICK_X", true } }, { 1, { "GP_LEFT_STICK_Y", true } }, { 2, { "L", false } },
            { 3, { "GP_RIGHT_STICK_X", true } }, { 4, { "GP_RIGHT_STICK_Y", true } }, { 5, { "R", false } },
        };
        return names;
    }

    const std::map<int, Name> & DualShock4Keys()
    {
        static const std::map<int, Name> names = {
            { 304, { "Square", false } }, { 305, { "Cross", false } }, { 306, { "Circle", false } }, { 307, { "Triangle", false } },
            { 308, { "L1", false } }, { 309, { "R1", false } }, { 310, { "L2", false } }, { 311, { "R2", false } },
            { 312, { "Share", false } }, { 313, { "Options", false } }, { 314, { "L3", false } }, { 315, { "R3", false } },
            { 316, { "PS", false } }, { 317, { "Touchpad", false } },
        };
        return names;
    }

    const std::map<int, Name> & DualShock4Axes()
    {
        static const std::map<int, Name> names = {
            { 0, { "GP_LEFT_STICK_X", true } }, { 1, { "GP_LEFT_STICK_Y", true } }, { 2, { "GP_RIGHT_STICK_X", true } },
            { 3, { "L2", false } }, { 4, { "R2", false } }, { 5, { "GP_RIGHT_STICK_Y", true } },
        };
        return names;
    }

    const std::map<int, Name> & Xbox360Keys()
    {
        static const std::map<int, Name> names = {
            { 304, { "A", false } }, { 305, { "B", false } }, { 307, { "X", false } }, { 308, { "Y", false } },
            { 310, { "LB", false } }, { 311, { "RB", false } }, { 314, { "Back", false } }, { 315, { "Start", false } },
            { 316, { "Guide", false } }, { 317, { "LS", false } }, { 318, { "RS", false } },
        };
        return names;
    }

    const std::map<int, Name> & Xbox360Axes()
    {
        static const std::map<int, Name> names = {
            { 0, { "GP_LEFT_STICK_X", true } }, { 1, { "GP_LEFT_STICK_Y", true } }, { 2, { "LT", false } },
            { 3, { "GP_RIGHT_STICK_X", true } }, { 4, { "GP_RIGHT_STICK_Y", true } }, { 5, { "RT", false } },
        };
        return names;
    }

    const std::map<int, Name> & PlayStationClassicKeys()
    {
        static const std::map<int, Name> names = {
            { 304, { "Square", false } }, { 305, { "Cross", false } }, { 306, { "Circle", false } }, { 307, { "Triangle", false } },
            { 308, { "L2", false } }, { 309, { "R2", false } }, { 310, { "L1", false } }, { 311, { "R1", false } },
            { 312, { "Select", false } }, { 313, { "Start", false } },
        };
        return names;
    }

    const Model * FindModel(unsigned int vendor, unsigned int product)
    {
        static const Model models[] = {
            { 0x054c, 0x09cc, DualShock4Keys(), DualShock4Axes(), false },
            { 0x054c, 0x05c4, DualShock4Keys(), DualShock4Axes(), false },
            { 0x054c, 0x0cda, PlayStationClassicKeys(), {}, true },
            { 0x045e, 0x028e, Xbox360Keys(), Xbox360Axes(), false },
        };
        for(const Model & model : models)
            if(model.vendor == vendor && model.product == product)
                return &model;
        return nullptr;
    }

    std::string Lookup(const std::map<int, Name> * names, int code, const std::string & fallbackKey)
    {
        if(names)
        {
            const auto it = names->find(code);
            if(it != names->end())
                return Text(it->second);
        }
        return Translate(fallbackKey) + " " + std::to_string(code);
    }

    std::string DirectionMark(char sign)
    {
        return sign == '+' ? " +" : sign == '-' ? " -" : "";
    }

    std::string DirectionLabel(bool horizontal, char sign)
    {
        if(horizontal)
            return Translate(sign == '-' ? "GP_LEFT" : "GP_RIGHT");
        return Translate(sign == '-' ? "GP_UP" : "GP_DOWN");
    }

    std::string HatLabel(const std::string & source)
    {
        return DirectionLabel(source[source.size() - 2] == 'x', source.back());
    }
}

std::string FrontBindingLabel(const std::string & binding)
{
    if(binding.size() < 2)
        return binding;
    const int index = std::atoi(binding.c_str() + 1);
    if(binding[0] == 'b')
        return Lookup(&FrontButtons(), index, "GP_BUTTON");
    if(binding[0] == 'a')
        return Lookup(&FrontAxes(), index, "GP_AXIS");
    return binding;
}

std::string UsbSourceLabel(unsigned int vendor, unsigned int product, const std::string & source)
{
    if(source.size() < 2)
        return source;
    const Model * model = FindModel(vendor, product);
    const int code = std::atoi(source.c_str() + 1);
    switch(source[0])
    {
    case 'k':
        return Lookup(model ? &model->keys : nullptr, code, "GP_BUTTON");
    case 'a':
        if(model && model->axisDpad && code <= ABS_Y && (source.back() == '+' || source.back() == '-'))
            return DirectionLabel(code == ABS_X, source.back());
        return Lookup(model ? &model->axes : nullptr, code, "GP_AXIS") + DirectionMark(source.back());
    case 'h':
        return source.size() >= 4 ? HatLabel(source) : source;
    default:
        return source;
    }
}
