#!/bin/sh
#
#  Copyright 2019 DefKorns (https://gitlab.com/advokaten/remap-canoekachikachi/LICENSE)
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
source $mountpoint/etc/options_menu/inputs/scripts/om_vars
script_init
# If controller 2 file exists, activate Bind P2 option.
# And if Controller 1 also exist, activate Bind All. Otherwise will disable Bind P2 and Bind All
if [ -e "$ctrl_p2" ]; then
	rename "$disableBindP2" "$enableBindP2"
	[ -e "$ctrl_p1" ] && rename "$disableBindAll" "$enableBindAll"
else
	# waits to Controller 1 to be connected before disable
	while [ ! -e "$ctrl_p1" ]; do usleep 100; done
	rename "$enableBindP2" "$disableBindP2"
	rename "$enableBindAll" "$disableBindAll"
fi

usleep 50000 && $optionsMenu/options --commandPath $omModCommands/ --scriptPath $omModScripts --title "$title" &
