#!/bin/bash
#
#
#    Copyright 2012 user890104
#
#
#    This file is part of emCORE.
#
#    emCORE is free software: you can redistribute it and/or
#    modify it under the terms of the GNU General Public License as
#    published by the Free Software Foundation, either version 2 of the
#    License, or (at your option) any later version.
#
#    emCORE is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#    See the GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with emCORE.  If not, see <http://www.gnu.org/licenses/>.
#
#

# Quick README:
# 1. place this script in your home dir
# 2. checkout our SVN to $HOME/freemyipod
# 3. build the tree (or use precompiled binaries)
# 4. adjust the paths below
# 5. plug your iPod and enter DFU mode
# 6. run the script
# 7. enjoy!

# works for both initial installs and updating

####################################
# IMPORTANT!!! EDIT THESE BELOW!!! #
####################################

if [ -z "${1}" ]
then
	FMI_TARGET_DEVICE='/dev/sdc'
else
	FMI_TARGET_DEVICE="${1}"
fi

FMI_IPODDFU="${HOME}/freemyipod/tools/ipoddfu/ipoddfu.py"
FMI_BOOTSTRAP="${HOME}/freemyipod/apps/installer-ipodclassic/build/bootstrap-ipodclassic.dfu"
#FMI_BOOTSTRAP="${HOME}/dfu-images/bootstrap-ipodclassic-r708-20110424.dfu"
#FMI_INSTALLER="${HOME}/freemyipod/emcore/trunk/build/ipodclassic/emcore.bin"
FMI_INSTALLER="${HOME}/freemyipod/apps/installer-ipodclassic/build/installer-ipodclassic.ubi"
#FMI_INSTALLER="${HOME}/installers/installer-ipodclassic-r708-20110424.ubi"

########################
# DO NOT EDIT BELOW!!! #
########################

FMI_MOUNTPOINT="$(mktemp --directory --tmpdir freemyipod.XXXXXXXX)"/
trap "echo; rm -vrf '${FMI_MOUNTPOINT}'" EXIT

python3 "${FMI_IPODDFU}" "${FMI_BOOTSTRAP}" && \

( \
	echo -n 'Please wait'

	while [ ! -e "${FMI_TARGET_DEVICE}" ]
	do
		echo -n .
		sleep 1
	done

	echo
) && \
sudo mount "${FMI_TARGET_DEVICE}" "${FMI_MOUNTPOINT}" && \
sudo cp "${FMI_INSTALLER}" "${FMI_MOUNTPOINT}"/init.ubi && \
sudo eject "${FMI_TARGET_DEVICE}" && \
rm -vrf "${FMI_MOUNTPOINT}" && \
echo 'Installer has been launched!'
