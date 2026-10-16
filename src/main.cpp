/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */

#include "remapper_app.h"
#include "framework/sdl_helper.h"
#include "localization.h"

#include <string>

int main()
{
    const std::string optionsLocation = "/etc/options_menu/";
    LoadLanguageFromConfig(optionsLocation);
    SetTTFFontPath(optionsLocation);
    UiTheme::LoadThemeConfig(optionsLocation);

    RemapperApp app(optionsLocation);
    return app.Run();
}
