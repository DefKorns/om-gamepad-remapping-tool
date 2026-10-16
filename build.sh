#!/usr/bin/env bash
set -euo pipefail

[ -f vendor/hmod-build/docker-make.sh ] || git submodule update --init vendor/hmod-build
[ -f vendor/OptionsMenu/src/framework/sdl_context.cpp ] || git submodule update --init --recursive vendor/OptionsMenu

sh vendor/hmod-build/docker-make.sh "$@"

ls -la mod/etc/options_menu/inputs/gamepad_remapper out/*.hmod 2>/dev/null
