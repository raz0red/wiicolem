#!/bin/bash

#---------------------------------------------------------------------------#
#   __      __.__.___________        .__                                    #
#  /  \    /  \__|__\_   ___ \  ____ |  |   ____   _____                    #
#  \   \/\/   /  |  /    \  \/ /  _ \|  | _/ __ \ /     \                   #
#   \        /|  |  \     \___(  <_> )  |_\  ___/|  Y Y  \                  #
#    \__/\  / |__|__|\______  /\____/|____/\___  >__|_|  /                  #
#         \/                \/                 \/      \/                   #
#     WiiColem by raz0red                                                   #
#     Port of the ColEm emulator by Marat Fayzullin                         #
#                                                                           #
#     [github.com/raz0red/wiicolem]                                         #
#                                                                           #
#---------------------------------------------------------------------------#
#                                                                           #
#  Copyright (C) 2019 raz0red                                               #
#                                                                           #
#  The license for ColEm as indicated by Marat Fayzullin, the author of     #
#  ColEm is detailed below:                                                 #
#                                                                           #
#  ColEm sources are available under three conditions:                      #
#                                                                           #
#  1) You are not using them for a commercial project.                      #
#  2) You provide a proper reference to Marat Fayzullin as the author of    #
#     the original source code.                                             #
#  3) You provide a link to http://fms.komkon.org/ColEm/                    #
#                                                                           #
#---------------------------------------------------------------------------#

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
DATE="$( date '+%Y%m%d%H%S' )"
DIST_DIR=$SCRIPTPATH/dist
LAYOUT_DIR=$SCRIPTPATH/res/layout/
BOOT_DOL_SRC=$SCRIPTPATH/boot.dol
BOOT_DOL_DEST=$DIST_DIR/apps/wiicolem
BOOT_ELF_SRC=$SCRIPTPATH/boot.elf
BOOT_ELF_DEST=$DIST_DIR
META_FILE=$DIST_DIR/apps/wiicolem/meta.xml
EMUCOMMON_DIR=$SCRIPTPATH/wii-emucommon

#
# Function that is invoked when the script fails.
#
# $1 - The message to display prior to exiting.
#
function fail() {
    echo $1
    echo "Exiting."
    exit 1
}

#
# Build in directory specified (if applicable)
#
if [ ! -z $1 ]; then
    BUILD_DIR=$1
    echo "Building in: $BUILD_DIR..."
    rm -rf $BUILD_DIR \
        || { fail 'Unable to delete build directory.'; }
    cp -R $SCRIPTPATH $BUILD_DIR \
        || { fail 'Unable to copy files to build directory.'; }
    find $BUILD_DIR -type f \
        \( -name "*.o" -or -name "*.a" -or -name "*.dol" -or -name "*.elf" \) \
        -exec rm {} + \
        || { fail 'Unable to delete previous build output files.'; }
    $BUILD_DIR/dist.sh \
        || { fail 'Error running dist script.'; }
    rm -rf $DIST_DIR \
        || { fail 'Unable to remove dist directory.'; }
    cp -R $BUILD_DIR/dist $DIST_DIR  \
        || { fail 'Unable to copy files to final dist directory.'; }
    exit 0
fi

# Build wii-emucommon
echo "Building wii-emucommon..."
$EMUCOMMON_DIR/dist.sh || { fail 'Error building wii-emucommon.'; }

# Change to script directory
echo "Changing to script directory..."
cd $SCRIPTPATH || { fail 'Error changing to script directory.'; }

# Build WiiColem
echo "Building WiiColem..."
make || { fail 'Error building WiiColem.'; }

# Clear dist directory
if [ -d $DIST_DIR ]; then
    echo "Clearing dist directory..."
    rm -rf $DIST_DIR || { fail 'Error clearing dist directory.'; }
fi

# Copy layout
echo "Copy layout..."
cp -R $LAYOUT_DIR $DIST_DIR || { fail 'Error copying layout.'; }

# Remove .gitignore
echo "Cleaning layout..."
find $DIST_DIR -name .gitignore -type f -delete \
    || { fail 'Error cleaning layout.'; }

# Copy boot files
echo "Copying boot files..."
cp $BOOT_DOL_SRC $BOOT_DOL_DEST || { fail 'Error copying boot.dol.'; }

# Update date in meta file
echo "Setting date in meta file..."
sed -i "s,000000000000,$DATE,g" $META_FILE \
    || { fail 'Error setting date in meta file.'; }

# Update version in meta-file (if SNAPSHOT)
echo "Updating version in meta file..."
sed -i "s,-SNAPSHOT,-SNAPSHOT-$DATE,g" $META_FILE \
    || { fail 'Error setting version in meta file.'; }
    
# Create the distribution (zip)    
echo "Creating distribution..."
VERSION=$( sed -ne "s/.*version>\(.*\)<\/version.*/\1/p" $META_FILE )
VERSION_NO_DOTS="${VERSION//./_}"
DIST_FILE=wiicolem-$VERSION_NO_DOTS.zip
cd $DIST_DIR || { fail 'Error changing to distribution directory.'; }
zip -r $DIST_FILE . || { fail 'Error creating zip file.'; }
rm -rf $DIST_DIR/wiicolem \
    || { fail 'Error deleting wiicolem directory in dist.'; }
rm -rf $DIST_DIR/apps \
    || { fail 'Error deleting apps directory in dist.'; }
cp $BOOT_ELF_SRC $BOOT_ELF_DEST || { fail 'Error copying boot.elf.'; }
