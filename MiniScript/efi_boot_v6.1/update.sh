#!/bin/bash

if [ $USER != "root" ]; then
   echo "Error ==> This script must be run as root"
   exit
fi

if [ ! -e "/Volumes/EFI" ]; then
     echo " Error ==> /Volumes/EFI does not exist"
     echo " Error ==> Your EFI partition must be mounted for this script to work"
fi

echo "Updating EFI boot cache"
mv /Volumes/EFI/System/Booter/Extensions.mkext /Volumes/EFI/System/Booter/Extensions.mkext.previous
chmod -R 644 /Volumes/EFI/Extensions
chown -R root:wheel /Volumes/EFI/Extensions
kextcache -a i386 -m /Volumes/EFI/System/Booter/Extensions.mkext /Volumes/EFI/Extensions > update.log 2>&1
chmod 644 /Volumes/EFI/System/Booter/Extensions.mkext
chown root:wheel /Volumes/EFI/System/Booter/Extensions.mkext

echo "done."
