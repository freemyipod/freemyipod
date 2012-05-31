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

# Builds the installers
# Adjust the path below, it needs to point to the root of our svn tree
# You also need a rockbox tree at the same level as our svn tree, that
# includes builds for ipodnano2g and ipod6g, in build-ipodnano2g and
# build-ipod6g, respectively.

ROOT_PATH='/home/venci/freemyipod/'

# what to build? builds the classic installer by default

DIRS=(\
	'tools/ucl' \
	'tools/elf2emcoreapp' \
	'apps/installer-ipodclassic' \
)
#	'apps/installer-ipodnano2g' \

# end of configuration

cd "${ROOT_PATH}" || exit 1

for i in $(seq 0 $[${#DIRS[@]} - 1])
do
        echo '== [begin] '"${DIRS[${i}]}"': make clean =='

        cd "${ROOT_PATH}${DIRS[${i}]}" && \
	\
        if [ 'apps/installer-ipodnano2g' = "${DIRS[${i}]}" ] \
        || [ 'apps/installer-ipodclassic' = "${DIRS[${i}]}" ]
        then
		echo
                #rm -vf 'flashfiles.built' && \
                #find flashfiles -maxdepth 1 -type f |xargs rm -vrf
	elif [ 'tools/elf2emcoreapp' = "${DIRS[${i}]}" ]
	then
		rm -vf '../arm-elf-eabi-elf2emcoreapp'
	fi

        echo '== [end] '"${DIRS[${i}]}"': make clean =='
done

cd "${ROOT_PATH}" || exit 2

if [ -n "${1}" ]
then
	if [ '0' = "${1}" ]
	then
		exit
	fi

	svn -r "${1}" update || exit 3
else
	svn update || exit 3
fi

rm -vf "${ROOT_PATH}emcore/trunk/build/version.h"

for i in $(seq 0 $[${#DIRS[@]} - 1])
do
        echo '== [begin] '"${DIRS[${i}]}"': make =='

        cd "${ROOT_PATH}${DIRS[${i}]}" && \

        if [ 'apps/installer-ipodnano2g' = "${DIRS[${i}]}" ]
	then
                make flashfiles && \
		'../../tools/ucl2e10singleblk' '../../../rockbox/build-ipodnano2g/rockbox.ipod' 'flashfiles/rockbox.ipod.ucl' && \
		make
        elif [ 'apps/installer-ipodclassic' = "${DIRS[${i}]}" ]
        then
                make flashfiles && \
		'../../tools/ucl2e10singleblk' '../../../rockbox/build-ipod6g/rockbox.ipod' 'flashfiles/rockbox.ipod.ucl' && \
		make
        elif [ 'tools/elf2emcoreapp' = "${DIRS[${i}]}" ]
	then
		#make
		ln -vs 'elf2emcoreapp/arm-elf-eabi-elf2emcoreapp' '../'
	else
                make
        fi

        STATUSES[${i}]=$?

        echo '== [end] '"${DIRS[${i}]}"': make =='
done

for i in $(seq 0 $[${#DIRS[@]} - 1])
do
        echo -n "${DIRS[${i}]}"': '

        if [ -n "{STATUSES[${i}]}" ] && [ '0' = "${STATUSES[${i}]}" ]
        then
                echo 'SUCCESS!'
        else
                echo 'ERROR!'
        fi
done
