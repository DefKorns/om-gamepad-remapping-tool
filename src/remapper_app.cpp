/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */

#include "remapper_app.h"
#include "menu_navigation.h"
#include "pad_map.h"
#include "framework/draw_helpers.h"
#include "framework/powerwatch.h"
#include "framework/utf8.h"
#include "localization.h"

#include <sys/stat.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>

namespace
{
    const char * const ActiveDbPath = "/etc/sdl2/gamecontrollerdb.txt";
    const char * const StockDbPath = "/var/squashfs/etc/sdl2/gamecontrollerdb.txt";
    constexpr Uint32 CaptureTimeoutMs = 5000;
    constexpr Uint32 NoticeMs = 4000;
    constexpr int RowGlyphSize = 16;
    constexpr int ActionRowCount = 4;
    constexpr int ControllerRowIndex = 0;
    constexpr int SaveRowIndex = 2;
    constexpr int PanelMarginX = 160;
    constexpr int PanelH = 250;
    constexpr int PanelPromptSize = 20;
    constexpr int PanelTargetSize = 40;
    constexpr int PanelSmallSize = 16;
    constexpr int PanelPromptY = 40;
    constexpr int PanelTargetY = 100;
    constexpr int PanelProgressY = 145;
    constexpr int PanelLastCaptureY = 168;
    constexpr int NoticeFontSize = 14;
    constexpr int NoticeLinePitch = 20;
    constexpr int PanelTimerBottom = 50;
    constexpr int PanelHintBottom = 25;
    constexpr int TimerBarH = 6;
    constexpr int TimerBarInset = 60;
    constexpr int SectionAccentGap = 14;
    constexpr int CreditFontSize = 16;
    constexpr int SelectionBorderW = 2;
    constexpr int SelectionPad = 2;
    constexpr int DividerOffset = 3;

    int TextWidth(const std::string & text, int fontSize)
    {
        return CanRenderWithTTF(text, fontSize) ? MeasureTTFWidth(text, fontSize) : Utf8Length(text) * fontSize;
    }

    std::vector<std::string> WrapText(const std::string & text, int fontSize, int maxWidth)
    {
        std::vector<std::string> lines;
        std::string line;
        size_t start = 0;
        while(start < text.size())
        {
            size_t end = text.find(' ', start);
            if(end == std::string::npos)
                end = text.size();
            const std::string word = text.substr(start, end - start);
            const std::string candidate = line.empty() ? word : line + " " + word;
            if(!line.empty() && TextWidth(candidate, fontSize) > maxWidth)
            {
                lines.push_back(line);
                line = word;
            }
            else
                line = candidate;
            start = end + 1;
        }
        if(!line.empty())
            lines.push_back(line);
        return lines;
    }

    std::string ControllerLabel(const std::string & deviceName)
    {
        const std::string marker = "controller";
        const size_t at = deviceName.rfind(marker);
        if(at == std::string::npos || at + marker.size() >= deviceName.size())
            return deviceName;
        return Translate("GP_CONTROLLER") + " " + deviceName.substr(at + marker.size());
    }

    bool IsAxisField(const std::string & field)
    {
        const std::string stock = GamepadMapping::Defaults().Get(field);
        return !stock.empty() && stock[0] == 'a';
    }

    std::string BaseDbPath()
    {
        return std::ifstream(StockDbPath).good() ? StockDbPath : ActiveDbPath;
    }
}

RemapperApp::RemapperApp(std::string optionsLocation)
    : optionsLocation_(std::move(optionsLocation))
    , customDbPath_(optionsLocation_ + "inputs/sdl2/gamecontrollerdb.txt")
    , saved_(LoadActiveMapping(ActiveDbPath))
    , mapping_(saved_)
{
    sdlContext_.reset(new SDL_Context(std::chrono::milliseconds(33), false));
    renderer_ = sdlContext_->renderer;
    controller_.reset(new Controller(1));
    sdlCapture_.reset(new InputCapture());
    capture_ = sdlCapture_.get();
    SetDrawColor(renderer_, UiTheme::Bg);

    gearIcon_ = Texture(optionsLocation_ + UiTheme::AssetGear, renderer_, UiTheme::GearX, UiTheme::GearY);
    appTitleText_ = Texture(Translate("GP_TITLE"), UiTheme::TitleFontSize, renderer_, UiTheme::TitleX, UiTheme::TitleY, false, ToAbgr(UiTheme::Text), true);
    appTitleText_.rect.y -= appTitleText_.rect.h / 2;
    appVersionText_ = Texture(MOD_VERSION, UiTheme::VersionFontSize, renderer_, appTitleText_.rect.x + appTitleText_.rect.w + UiTheme::VersionGap, UiTheme::TitleY, false, ToAbgr(UiTheme::Text), true);
    appVersionText_.rect.y -= appVersionText_.rect.h / 2;
    sectionTitle_ = Texture(Translate("GP_BUTTONS"), UiTheme::SectionTitleFontSize, renderer_, UiTheme::SectionTitleX, UiTheme::SectionTitleY, false, ToAbgr(UiTheme::Text), true);
    creditText_ = Texture("Gamepad Remapping Tool - by DefKorns", CreditFontSize, renderer_, UiTheme::CreditX, UiTheme::CreditY, false, ToAbgr(UiTheme::Text), true);

    badges_.reset(new BadgePainter(optionsLocation_, renderer_));
    badgeA_ = badges_->Make("A", Translate("HINT_SELECT"), UiTheme::BadgeADark, UiTheme::BadgeA);
    badgeB_ = badges_->Make("B", Translate("HINT_BACK"), UiTheme::BadgeBDark, UiTheme::BadgeB);
    exitDialog_.reset(new ConfirmDialog(renderer_, *badges_, Translate("GP_UNSAVED"),
                                        badges_->Make("A", Translate("GP_DISCARD"), UiTheme::BadgeADark, UiTheme::BadgeA),
                                        badges_->Make("B", Translate("GP_STAY"), UiTheme::BadgeBDark, UiTheme::BadgeB)));

    const char * const actionKeys[ActionRowCount] = { "GP_CONTROLLER", "GP_MAP_ALL", "GP_SAVE", "GP_RESTORE" };
    const RowKind actionKinds[ActionRowCount] = { RowKind::Controller, RowKind::MapAll, RowKind::Save, RowKind::Restore };
    for(int i = 0; i < ActionRowCount; ++i)
        rows_.push_back(Row{ actionKinds[i], -1, Texture(Translate(actionKeys[i]), RowGlyphSize, renderer_, UiTheme::RowTextX, 0, false, ToAbgr(UiTheme::Text), true) });
    const std::vector<MappingTarget> & targets = MappingTargets();
    for(int i = 0; i < static_cast<int>(targets.size()); ++i)
        rows_.push_back(Row{ RowKind::Target, i, Texture(TargetLabel(i), RowGlyphSize, renderer_, UiTheme::RowTextX, 0, false, ToAbgr(UiTheme::Text), true) });

    for(int i = 0; i < static_cast<int>(targets.size()); ++i)
        if(!targets[i].extra)
            wizardOrder_.push_back(i);

    rowPitch_ = std::max(UiTheme::RowPitch, GetTTFLineHeight(RowGlyphSize));
    visibleRows_ = std::max(1, (UiTheme::FooterDividerY - UiTheme::ListBottomMargin - UiTheme::RowFirstY) / rowPitch_);
    SelectController(0);
}

RemapperApp::~RemapperApp()
{
    SetMapperPaused(false);
}

std::string RemapperApp::ControllerName(int index) const
{
    if(index <= 0 || index > static_cast<int>(usbPads_.size()))
        return Translate("GP_FRONT_PORTS");
    return usbPads_[index - 1].name;
}

void RemapperApp::SetMapperPaused(bool paused) const
{
    if(paused)
        std::ofstream(PadMapperPauseFlag).put('1');
    else
        std::remove(PadMapperPauseFlag);
}

void RemapperApp::BuildSideNotice()
{
    sharedNotice_.clear();
    int noticeY = UiTheme::RowFirstY;
    for(const std::string & line : WrapText(Translate(UsbMode() ? "GP_USB_NOTICE" : "GP_SHARED_NOTICE"), NoticeFontSize, UiTheme::DetailW))
    {
        sharedNotice_.push_back(Texture(line, NoticeFontSize, renderer_, UiTheme::DetailX, noticeY, false, ToAbgr(UiTheme::TextDim), true));
        noticeY += NoticeLinePitch;
    }
}

void RemapperApp::SelectController(int index)
{
    usbPads_ = UsbPads(ListInputNodes());
    const int count = static_cast<int>(usbPads_.size()) + 1;
    controllerIndex_ = ((index % count) + count) % count;
    rawCapture_.reset();
    saved_ = GamepadMapping();
    if(UsbMode())
    {
        SetMapperPaused(true);
        rawCapture_.reset(new RawCapture(usbPads_[controllerIndex_ - 1]));
        capture_ = rawCapture_.get();
        LoadPadMap(PadMapPath(usbPads_[controllerIndex_ - 1]), saved_);
    }
    else
    {
        SetMapperPaused(false);
        capture_ = sdlCapture_.get();
        saved_ = LoadActiveMapping(ActiveDbPath);
    }
    mapping_ = saved_;
    rows_[ControllerRowIndex].label = Texture(Translate("GP_CONTROLLER") + ":  < " + ControllerName(controllerIndex_) + " >", RowGlyphSize, renderer_, UiTheme::RowTextX, 0, false, ToAbgr(UiTheme::Text), true);
    BuildSideNotice();
    RefreshValues();
}

std::string RemapperApp::TargetLabel(int target) const
{
    const MappingTarget & t = MappingTargets()[target];
    return t.translateLabel ? Translate(t.label) : t.label;
}

void RemapperApp::RefreshValues()
{
    const std::vector<MappingTarget> & targets = MappingTargets();
    values_.clear();
    for(const MappingTarget & t : targets)
    {
        const std::string value = mapping_.Get(t.field);
        const Color color = value != saved_.Get(t.field) ? UiTheme::Accent : UiTheme::TextDim;
        values_.push_back(Texture(value.empty() ? "-" : value, RowGlyphSize, renderer_, 0, 0, false, ToAbgr(color), true));
    }
    const Color saveColor = mapping_ != saved_ ? UiTheme::Text : UiTheme::TextDim;
    rows_[SaveRowIndex].label = Texture(Translate("GP_SAVE"), RowGlyphSize, renderer_, UiTheme::RowTextX, 0, false, ToAbgr(saveColor), true);
}

void RemapperApp::ScrollTo(int row)
{
    if(row < topRow_)
        topRow_ = row;
    else if(row >= topRow_ + visibleRows_)
        topRow_ = row - visibleRows_ + 1;
    topRow_ = std::max(0, std::min(topRow_, static_cast<int>(rows_.size()) - visibleRows_));
}

void RemapperApp::ShowNotice(const std::string & key)
{
    noticeText_ = Texture(Translate(key), PanelSmallSize, renderer_, 0, 0, false, ToAbgr(UiTheme::Accent), true);
    noticeText_.rect.x = UiTheme::ListContentRightX - noticeText_.rect.w;
    noticeText_.rect.y = sectionTitle_.rect.y + (sectionTitle_.rect.h - noticeText_.rect.h) / 2;
    noticeUntil_ = SDL_GetTicks() + NoticeMs;
}

RemapperApp::FrameEvent RemapperApp::PollFrameEvents()
{
    sdlContext_->StartFrame();
    controller_->Update();
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        if(event.type == SDL_QUIT)
            return FrameEvent::Quit;
        sdlCapture_->HandleEvent(event);
    }
    capture_->Update();
    if(sdlContext_->powerwatch->buttonPress())
        return FrameEvent::PowerButtonPressed;
    return FrameEvent::Continue;
}

void RemapperApp::StartCapture(int target)
{
    if(!capture_->HasDevice())
    {
        mode_ = Mode::Browse;
        ShowNotice("GP_NO_CONTROLLER");
        return;
    }
    captureTarget_ = target;
    ScrollTo(ActionRowCount + target);
    capture_->Begin();
    captureStartedAt_ = SDL_GetTicks();
}

void RemapperApp::StartWizardStep()
{
    if(wizardStep_ >= wizardOrder_.size())
    {
        if(mode_ == Mode::Wizard)
            StartExtrasGate();
        else
            EndCapture();
        return;
    }
    StartCapture(wizardOrder_[wizardStep_]);
}

void RemapperApp::StartExtrasGate()
{
    mode_ = Mode::ExtrasGate;
    captureTarget_ = -1;
    capture_->Begin();
    captureStartedAt_ = SDL_GetTicks();
}

void RemapperApp::EndCapture()
{
    capture_->Cancel();
    captureTarget_ = -1;
    mode_ = Mode::Browse;
    controller_->Reset();
}

void RemapperApp::HandleCapture()
{
    std::string binding;
    const bool captured = capture_->TakeResult(binding);
    const bool timedOut = !captured && capture_->Waiting() && SDL_GetTicks() - captureStartedAt_ >= CaptureTimeoutMs;
    if(!captured && !timedOut)
        return;

    if(mode_ == Mode::ExtrasGate)
    {
        if(!captured || binding != mapping_.Get("a"))
        {
            EndCapture();
            return;
        }
        wizardOrder_.clear();
        for(int i = 0; i < static_cast<int>(MappingTargets().size()); ++i)
            if(MappingTargets()[i].extra)
                wizardOrder_.push_back(i);
        wizardStep_ = 0;
        mode_ = Mode::WizardExtras;
        StartWizardStep();
        return;
    }

    if(captured)
    {
        const std::string & field = MappingTargets()[captureTarget_].field;
        if(UsbMode() && IsAxisField(field) && !binding.empty() && (binding.back() == '+' || binding.back() == '-'))
            binding.pop_back();
        mapping_.Set(field, binding);
        RefreshValues();
        lastCapture_ = Texture(Translate("GP_LAST") + ": " + binding + " (" + ControllerLabel(capture_->ResultSource()) + ")", PanelSmallSize, renderer_, 0, 0, false, ToAbgr(UiTheme::TextDim), true);
    }
    if(mode_ == Mode::CaptureOne)
    {
        EndCapture();
        return;
    }
    ++wizardStep_;
    StartWizardStep();
}

void RemapperApp::Activate(const Row & row)
{
    switch(row.kind)
    {
    case RowKind::Controller:
        if(mapping_ != saved_)
            ShowNotice("GP_SAVE_FIRST");
        else
            SelectController(controllerIndex_ + 1);
        break;
    case RowKind::MapAll:
        lastCapture_ = Texture();
        wizardOrder_.clear();
        for(int i = 0; i < static_cast<int>(MappingTargets().size()); ++i)
            if(!MappingTargets()[i].extra)
                wizardOrder_.push_back(i);
        wizardStep_ = 0;
        mode_ = Mode::Wizard;
        StartWizardStep();
        break;
    case RowKind::Save:
        Save();
        break;
    case RowKind::Restore:
        Restore();
        break;
    case RowKind::Target:
        lastCapture_ = Texture();
        mode_ = Mode::CaptureOne;
        StartCapture(row.target);
        break;
    }
}

void RemapperApp::Save()
{
    if(mapping_ == saved_)
        return;
    if(UsbMode())
    {
        if(!SavePadMap(PadMapPath(usbPads_[controllerIndex_ - 1]), mapping_))
        {
            ShowNotice("GP_SAVE_FAILED");
            return;
        }
        saved_ = mapping_;
        RefreshValues();
        ShowNotice("GP_SAVED");
        return;
    }
    mkdir((optionsLocation_ + "inputs/sdl2").c_str(), 0755);
    if(!WriteMappingDb(BaseDbPath(), customDbPath_, mapping_))
    {
        ShowNotice("GP_SAVE_FAILED");
        return;
    }
    std::system(("sh " + optionsLocation_ + "inputs/scripts/gamepad_apply apply").c_str());
    saved_ = LoadActiveMapping(ActiveDbPath);
    mapping_ = saved_;
    RefreshValues();
    ShowNotice("GP_SAVED");
}

void RemapperApp::Restore()
{
    if(UsbMode())
    {
        std::remove(PadMapPath(usbPads_[controllerIndex_ - 1]).c_str());
        saved_ = GamepadMapping();
        mapping_ = saved_;
        RefreshValues();
        ShowNotice("GP_RESTORED");
        return;
    }
    std::system(("sh " + optionsLocation_ + "inputs/scripts/gamepad_apply restore").c_str());
    saved_ = LoadActiveMapping(ActiveDbPath);
    mapping_ = saved_;
    RefreshValues();
    ShowNotice("GP_RESTORED");
}

void RemapperApp::ExitToMenu() const
{
    SetMapperPaused(false);
    std::system(BuildReturnCommand(optionsLocation_).c_str());
}

void RemapperApp::ResumeUnderlyingUi() const
{
    SetMapperPaused(false);
    std::system(("/bin/sh " + optionsLocation_ + "scripts/ResumeUI.sh").c_str());
}

bool RemapperApp::HandleBrowseInput()
{
    const int rowCount = static_cast<int>(rows_.size());
    if(controller_->GetButtonStatus(A) || controller_->GetButtonStatus(START))
        Activate(rows_[selected_]);
    else if(selected_ == ControllerRowIndex && (controller_->HeldRepeat(LEFT) || controller_->HeldRepeat(RIGHT)))
    {
        if(mapping_ != saved_)
            ShowNotice("GP_SAVE_FIRST");
        else
            SelectController(controllerIndex_ + (controller_->PeekButtonStatus(LEFT) ? -1 : 1));
    }
    else if(controller_->HeldRepeat(UP))
        selected_ = (selected_ - 1 + rowCount) % rowCount;
    else if(controller_->HeldRepeat(DOWN))
        selected_ = (selected_ + 1) % rowCount;
    else if(controller_->GetButtonStatus(B))
    {
        if(mapping_ == saved_)
            return true;
        mode_ = Mode::ConfirmExit;
    }
    ScrollTo(selected_);
    return false;
}

void RemapperApp::DrawChrome()
{
    DrawStrokeRect(renderer_, UiTheme::OuterRect, UiTheme::Border, UiTheme::BorderWidth, UiTheme::BorderRadius);
    gearIcon_.Draw(renderer_);
    appTitleText_.Draw(renderer_);
    appVersionText_.Draw(renderer_);
    DrawHLine(renderer_, UiTheme::HeaderDividerX, UiTheme::HeaderDividerX + UiTheme::HeaderDividerW, UiTheme::HeaderDividerY, UiTheme::Border, UiTheme::BorderWidth);
    DrawHLine(renderer_, UiTheme::OuterRect.x, UiTheme::OuterRect.x + UiTheme::OuterRect.w, UiTheme::FooterDividerY, UiTheme::Border, UiTheme::BorderWidth);
    creditText_.Draw(renderer_);
    if(mode_ == Mode::Browse)
        badges_->Draw(badgeB_, badges_->Draw(badgeA_, UiTheme::BadgeClusterRightX, UiTheme::BadgeBandY), UiTheme::BadgeBandY);

    const int accentBarY = sectionTitle_.rect.y + (sectionTitle_.rect.h - UiTheme::SectionAccentBarH) / 2;
    DrawFillRect(renderer_, { UiTheme::SectionTitleX - UiTheme::SectionAccentBarW - SectionAccentGap, accentBarY, UiTheme::SectionAccentBarW, UiTheme::SectionAccentBarH }, UiTheme::Accent);
    sectionTitle_.Draw(renderer_);
    if(SDL_GetTicks() < noticeUntil_)
        noticeText_.Draw(renderer_);
    for(Texture & line : sharedNotice_)
        line.Draw(renderer_);
}

void RemapperApp::DrawRows()
{
    const int lastRow = std::min(static_cast<int>(rows_.size()), topRow_ + visibleRows_);
    const int highlighted = captureTarget_ >= 0 ? ActionRowCount + captureTarget_ : selected_;
    int y = UiTheme::RowFirstY;
    for(int i = topRow_; i < lastRow; ++i, y += rowPitch_)
    {
        Row & row = rows_[i];
        row.label.rect.y = y + (rowPitch_ - row.label.rect.h) / 2 + UiTheme::RowTextYNudge;
        if(i == highlighted && mode_ != Mode::ExtrasGate)
        {
            const SDL_Rect box{ UiTheme::ListX, row.label.rect.y - SelectionPad, UiTheme::ListContentRightX - UiTheme::ListX, row.label.rect.h + 2 * SelectionPad };
            DrawRoundedFillRect(renderer_, box, UiTheme::SelectedRowBg, UiTheme::BoxRadius);
            DrawStrokeRect(renderer_, box, UiTheme::Accent, SelectionBorderW, UiTheme::BoxRadius);
        }
        else if(i != lastRow - 1 && i != ActionRowCount - 1)
            DrawHLine(renderer_, UiTheme::ListX, UiTheme::ListContentRightX, row.label.rect.y + row.label.rect.h + DividerOffset, UiTheme::Border);
        row.label.Draw(renderer_);
        if(row.kind == RowKind::Target)
        {
            Texture & value = values_[row.target];
            value.rect.x = UiTheme::RowControlRightX - value.rect.w;
            value.rect.y = row.label.rect.y;
            value.Draw(renderer_);
        }
        if(i == ActionRowCount - 1)
            DrawHLine(renderer_, UiTheme::ListX, UiTheme::ListContentRightX, row.label.rect.y + row.label.rect.h + DividerOffset, UiTheme::Accent, SelectionBorderW);
    }
}

void RemapperApp::DrawCapturePanel()
{
    const SDL_Rect panel = Dialog::PanelRect(UiTheme::FrameW - 2 * PanelMarginX, PanelH);
    Dialog::DrawBackdrop(renderer_);
    Dialog::DrawPanel(renderer_, panel);
    const int centerX = panel.x + panel.w / 2;

    const bool gate = mode_ == Mode::ExtrasGate;
    Texture prompt(Translate(gate ? "GP_EXTRAS" : "GP_PRESS"), PanelPromptSize, renderer_, centerX, panel.y + PanelPromptY, true, ToAbgr(UiTheme::Text), true);
    prompt.Draw(renderer_);
    Texture target(gate ? Translate("GP_EXTRAS_CONTINUE") : TargetLabel(captureTarget_), gate ? PanelPromptSize : PanelTargetSize, renderer_, centerX, panel.y + PanelTargetY, true, ToAbgr(UiTheme::Accent), true);
    target.Draw(renderer_);
    if(mode_ == Mode::Wizard || mode_ == Mode::WizardExtras)
    {
        Texture progress(std::to_string(wizardStep_ + 1) + " / " + std::to_string(wizardOrder_.size()), PanelSmallSize, renderer_, centerX, panel.y + PanelProgressY, true, ToAbgr(UiTheme::TextDim), true);
        progress.Draw(renderer_);
    }
    lastCapture_.rect.x = centerX - lastCapture_.rect.w / 2;
    lastCapture_.rect.y = panel.y + PanelLastCaptureY;
    lastCapture_.Draw(renderer_);

    const Uint32 elapsed = std::min(CaptureTimeoutMs, SDL_GetTicks() - captureStartedAt_);
    const int barW = panel.w - 2 * TimerBarInset;
    const int remainingW = capture_->Waiting() ? static_cast<int>(barW * (CaptureTimeoutMs - elapsed) / CaptureTimeoutMs) : 0;
    DrawFillRect(renderer_, { panel.x + TimerBarInset, panel.y + PanelH - PanelTimerBottom, barW, TimerBarH }, UiTheme::Border);
    DrawFillRect(renderer_, { panel.x + TimerBarInset, panel.y + PanelH - PanelTimerBottom, remainingW, TimerBarH }, UiTheme::Accent);
    Texture hint(Translate(gate ? "GP_WAIT_TO_FINISH" : "GP_WAIT_TO_SKIP"), PanelSmallSize, renderer_, centerX, panel.y + PanelH - PanelHintBottom, true, ToAbgr(UiTheme::TextDim), true);
    hint.Draw(renderer_);
}

int RemapperApp::Run()
{
    for(;;)
    {
        const FrameEvent frameEvent = PollFrameEvents();
        if(frameEvent == FrameEvent::Quit)
            return 0;
        if(frameEvent == FrameEvent::PowerButtonPressed)
        {
            ResumeUnderlyingUi();
            return 0;
        }

        switch(mode_)
        {
        case Mode::Browse:
            if(HandleBrowseInput())
            {
                ExitToMenu();
                return 0;
            }
            break;
        case Mode::ConfirmExit:
            if(controller_->GetButtonStatus(A))
            {
                ExitToMenu();
                return 0;
            }
            if(controller_->GetButtonStatus(B))
                mode_ = Mode::Browse;
            break;
        default:
            HandleCapture();
            break;
        }

        DrawChrome();
        DrawRows();
        if(mode_ == Mode::ConfirmExit)
            exitDialog_->Draw();
        else if(mode_ != Mode::Browse)
            DrawCapturePanel();
        SetDrawColor(renderer_, UiTheme::Bg);
        sdlContext_->EndFrame();
    }
}
