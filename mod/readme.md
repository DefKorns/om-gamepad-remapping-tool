# Options Menu - Gamepad Remapping Tool

**Requires [my Options Menu fork](https://github.com/DefKorns/OptionsMenu/releases) as a base — not compatible with any other UI.**

![Front ports](https://raw.githubusercontent.com/DefKorns/om-gamepad-remapping-tool/om_version/docs/remapper-front-ports.png)

## What is it?

Remap the buttons that Canoe and Kachikachi (the console's SNES and NES emulators) see, from the console itself. Map the front ports, or give each USB controller model — a DualShock 4, for example — a mapping of its own.

Open the Options Menu (hold **L+R** on a SNES/Super Famicom, **B+Down** on a NES/Famicom) and go to **Advanced Options → Controller → Gamepad Remapping**.

## Features

- **Every button in one list** with what it's mapped to; select a row and press the new button to remap just that one
- **Map all buttons** walks through them one by one — wait 5 seconds to keep the current one — and then offers ZL, ZR, Home and the analog sticks
- **Pick the controller** on the first row: the front ports, or any USB controller by its real name
- **Save** applies right away, no reboot; **Restore defaults** undoes it
- **Down+Select** (the Home Combo) and the controller's own Home/PS button take you back to the menu on USB controllers too
- **Ready-made profiles:** a DualShock 4 works as soon as you plug it in, with the SNES layout (Circle = A, Cross = B, Triangle = X, Square = Y), and so does the Krom Kumite arcade stick. There's also a profile for the Xbox 360 controller, not tested yet
- Buttons are shown by their names — Cross, L1, Share, D-Pad Up — instead of raw codes

## Front ports and USB controllers

- **Front ports:** one mapping, shared by both ports and by any USB controller that has no mapping of its own. The console sees every front-port controller as the same type, so they can't be told apart.
- **USB controllers:** each model gets its own mapping, and the front ports stay as they are. A USB controller plays as the first free player: player 2 with a controller in port 1, player 1 when port 1 is empty.
- Plug the front controller in before the USB one. If you plug it in later, unplug the USB controller and plug it back in.

## Requirements

- [Hakchi2 CE](https://github.com/TeamShinkansen/hakchi2/releases/latest)
- [My Options Menu fork](https://github.com/DefKorns/OptionsMenu/releases)

## Credits

- [DefKorns](https://github.com/DefKorns)
- Based on [advokaten's](https://github.com/advokaten) [Remap-Canoe-Controller](https://github.com/advokaten/Remap-Canoe-Controller)

## Thanks

- [CompCom](https://github.com/CompCom) — original Options Menu
- [ModMyClassic](https://modmyclassic.com/)
- RetroKane
