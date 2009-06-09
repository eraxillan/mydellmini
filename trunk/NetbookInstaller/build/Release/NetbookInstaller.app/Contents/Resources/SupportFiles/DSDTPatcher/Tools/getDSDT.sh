#!/bin/sh
ioreg -lw0 | grep DSDT > /tmp/dsdt/ioreg.txt
ioregdump=$(cat /tmp/dsdt/ioreg.txt)
modified1=${ioregdump#*'DSDT'}
modified2=${modified1#*'<'}
modified3=${modified2%%'>'*}
echo $modified3 > /tmp/dsdt/dsdt.txt
xxd -r -p /tmp/dsdt/dsdt.txt > /tmp/dsdt/dsdt.dat
rm /tmp/dsdt/dsdt.txt
rm /tmp/dsdt/ioreg.txt
