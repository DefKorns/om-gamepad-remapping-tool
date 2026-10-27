# Changelog

## v2.0.0

A full rewrite: the old shell scripts are gone, replaced by a native screen and a background service. Requires [Options Menu](https://github.com/DefKorns/OptionsMenu/releases) v2.0.0-beta2 or newer.

- **Native remapper** built on the Options Menu engine: every button in one list, a "Map all buttons" wizard, Save and Restore defaults, in all the Options Menu languages
- **Each USB controller model gets its own mapping**, handled by a new background service (`pad_mapper`). The front ports keep one shared mapping
- **Ready-made profiles** for the DualShock 4, the PlayStation Classic controller, the wired Xbox 360 controller and the Krom Kumite arcade stick
- Buttons are shown by their names (Cross, L1, Share…) instead of raw codes
- A USB controller keeps navigating the menu while you remap it

## v1.2.3

- Map PlayStation pads by label: Cross = A, Circle = B

## v1.2.2

- Add PlayStation Classic and tested Xbox 360 profiles

## v1.2.1

- Add a Krom Kumite profile and drop stale USB translators

## v1.2.0

- Ship USB pad profiles and show buttons by name

## v1.1.1

- Map USB pads on their own with a `pad_mapper` service
- Rewrite the readmes for the native remapper and USB pads

## v1.1.0

- Add a GitHub Actions workflow to build the tool
