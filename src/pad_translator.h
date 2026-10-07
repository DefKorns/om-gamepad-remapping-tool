/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */

#ifndef PAD_TRANSLATOR_H_
#define PAD_TRANSLATOR_H_

#include "gamepad_mapping.h"
#include "input_nodes.h"

#include <linux/input.h>

#include <vector>

class PadTranslator
{
public:
    PadTranslator(const InputNode & pad, const InputNode & target, const GamepadMapping & padMap, const GamepadMapping & frontMap);
    ~PadTranslator();
    PadTranslator(const PadTranslator &) = delete;
    PadTranslator & operator=(const PadTranslator &) = delete;

    bool Ok() const { return source_ >= 0 && target_ >= 0; }
    int Fd() const { return source_; }
    void Pump();

private:
    enum class SourceKind { Key, Axis, AxisDirection, Hat };

    struct Rule
    {
        SourceKind kind;
        int code;
        int direction;
        bool toKey;
        int targetCode;
        bool pressed;
        bool comboDown;
        bool comboSelect;
    };

    void AddRule(const std::string & field, const std::string & source, const GamepadMapping & frontMap);
    void Apply(const input_event & event);
    void Emit(int type, int code, int value);
    void UpdateHomeCombo();
    float Normalized(int code, int value) const;

    int source_ = -1;
    int target_ = -1;
    std::vector<Rule> rules_;
    std::vector<input_absinfo> sourceAbs_;
    std::vector<input_absinfo> targetAbs_;
    bool homeComboEnabled_ = false;
    bool homeActive_ = false;
};

#endif
