/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */

#ifndef REMAPPER_APP_H_
#define REMAPPER_APP_H_

#include "gamepad_mapping.h"
#include "input_capture.h"
#include "framework/badge.h"
#include "framework/controller.h"
#include "framework/dialog.h"
#include "framework/sdl_helper.h"

#include <memory>
#include <string>
#include <vector>

class RemapperApp
{
public:
    explicit RemapperApp(std::string optionsLocation);
    int Run();

private:
    enum class RowKind { MapAll, Save, Restore, Target };
    enum class Mode { Browse, CaptureOne, Wizard, ExtrasGate, WizardExtras, ConfirmExit };
    enum class FrameEvent { Continue, Quit, PowerButtonPressed };

    struct Row
    {
        RowKind kind;
        int target;
        Texture label;
    };

    FrameEvent PollFrameEvents();
    bool HandleBrowseInput();
    void HandleCapture();

    void Activate(const Row & row);
    void StartCapture(int target);
    void StartWizardStep();
    void StartExtrasGate();
    void EndCapture();
    void Save();
    void Restore();
    void ExitToMenu() const;
    void ResumeUnderlyingUi() const;
    void ShowNotice(const std::string & key);

    std::string TargetLabel(int target) const;
    void RefreshValues();
    void ScrollTo(int row);

    void DrawChrome();
    void DrawRows();
    void DrawCapturePanel();

    std::string optionsLocation_;
    std::string customDbPath_;
    std::unique_ptr<SDL_Context> sdlContext_;
    SDL_Renderer * renderer_ = nullptr;
    std::unique_ptr<Controller> controller_;
    std::unique_ptr<InputCapture> capture_;

    GamepadMapping saved_;
    GamepadMapping mapping_;
    std::vector<Row> rows_;
    std::vector<Texture> values_;
    std::vector<int> wizardOrder_;

    Mode mode_ = Mode::Browse;
    int selected_ = 0;
    int topRow_ = 0;
    int visibleRows_ = 1;
    int rowPitch_ = 0;
    int captureTarget_ = -1;
    size_t wizardStep_ = 0;
    Uint32 captureStartedAt_ = 0;
    Uint32 noticeUntil_ = 0;

    Texture gearIcon_, appTitleText_, appVersionText_, sectionTitle_, creditText_, noticeText_;
    std::unique_ptr<BadgePainter> badges_;
    std::unique_ptr<ConfirmDialog> exitDialog_;
    Badge badgeA_, badgeB_;
};

#endif
