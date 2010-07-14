README
------
Put here the stuff to work with the RT7550

FIRST STEPS - Connect to the RT7750 with serial cable
=====================================================

* Install minicom
* Run sudo minicom -s
* Set in Serial Port Setup

  * Serial Device: /dev/ttyUSB0 (or /dev/ttyUSB1)

  * Bps: 115200 8N1

  * Hardware Flow Control: No


Documentation URLs
======================
In here put the cites with documentation for the little arms:

* http://tech.groups.yahoo.com/group/ts-7000/

Sites to discover what chipset is in what wireless card

* http://wireless.kernel.org/en/users/Drivers/
* http://linux-wless.passys.nl/

Establishing AD-HOC Communication
=================================

PC1 - LGs laptops from INESC
PC2 - TS-7550s

Run in PC1
##########
* ifconfig eth1 down
* ifconfig eth1 up
* iwconfig eth1 essid teste mode Ad-Hoc channel 1 ap 02:0c:f1:b5:8f:01 key off
* ifconfig eth1 192.168.0.1

Run in PC2
#########
* Edit the file /etc/modprobe.d/blacklist and add rt73usb
* Copy the file "rt73_ts7500.ko" to "/lib/modules/2.6.24.4/kernel/drivers/net/wireless/rt2x00". You may find the rt_ts7500.ko file in this zip file: "ftp://ftp.embeddedarm.com/ts-arm-sbc/ts-7500-linux/binaries/wifi-g-usb-2_rt2501usb-binaries.tar.gz"
* Run depmod -a
* Restart (don't really now if it is necessary)
* ifconfig rausbwifi up
* iwconfig rausbwifi essid teste mode Ad-Hoc channel 1 ap 02:0C:F1:B5:CC:5D key off
* ifconfig rausbwifi 192.168.0.25

Autostart interfaces in the TS7550
##################################
I added the following lines to /etc/network/interfaces:

auto rausbwifi
iface rausbwifi inet static
    address 192.168.0.25
    netmask 255.255.255.0
    pre-up /root/adhoc.sh

Note that the /root/adhoc.sh is a script with the previous instructions (don't forget to markit executable)

Placa Wireless IOGEAR (Can't make it work in adhoc)
====================================================

*Modulos:*
zd1211 -> Old module developed by the company (available in sourceforge)
zd1211rw -> New module developed by the community (available since 2.6.18-rc1)

*Referencias:*
Site of the module:
http://wiki.debian.org/zd1211rw

Site to compile the module + arguments of the iwconfig:
https://docs.google.com/viewer?url=http://www.linuxowl.com/ffs/DocsSoftware/SWMULZ-5400-Linux-UserGuide.pdf

Automatically boot into Debian linux
====================================
In linux busy-box in the root directory issue:
ln -sf linuxrc-sdroot linuxrc; save

Translation table
=================
card |        ip       | Register
-------------------------------
  1  | 192.168.0.1     | MV-25
  2  | 192.168.0.2     | MV-26
  3  | 192.168.0.3     | MV-27

How to compile kernel in the TS-7500 node
=========================================

* Download the kernel from: ftp://ftp.embeddedarm.com/ts-arm-sbc/ts-7500-linux/sources/linux-2.6.24-ts-src-aug102009.tar.gz
 (or fetch it from: http://github.com/joninvski/ts_7500_kernel )

* Download the crosstool chain: ftp://ftp.embeddedarm.com/ts-arm-sbc/ts-7500-linux/cross-toolchains/crosstool-linux-arm-uclibc-3.4.6.tar.gz
 (or fetch it from: http://github.com/joninvski/arm-uclibc-3.4.6 )

* Download the module for the wireless card
 (or fetch it from: http://github.com/joninvski/USB_Wifi_RT2501_TS-7500 )

First compile the kernel
------------------------

* In the 2.6.24.4-cavium directory change the Makefile pointing it to the correct path. In my case:

  * CROSS_COMPILE	?= /home/workspace/plaquinhas/kernel/arm-uclibc-3.4.6/bin/arm-linux-

* Put the crosstoll chain in the path

* Run: $> make ts7500_defconfig

* Run: $> make menuconfig
(If there is any error compiling menuconfig just install the package libncurses-dev)

* Go to networking and select all the modules necessary for iptables/netfilter
(The .config present in the git repository contains this information)

* Run: $> make modules; make modules_install
(in here i did a litlle trick: chmod a+w /lib/modules to be able to install modules whitout being root)

Copy the kernel to the sd card
------------------------------

* Put the sdcard in the computer (let's assume sdb)

* Run: dd if=arch/arm/boot/zImage of=/dev/sdb2\

* Mount /dev/sdb4

* Copy the modules present in /lib/modules/2.6.24.4/ to the card 4th partition (to the same directory)

Compile the usb wifi card driver
--------------------------------

* Go the the directory of the usb wifi source code.

* In the Makefile change the cross tools path and the target to 7500
(you can find these changes in the git repository)

* make

* Copy the ts73.ko file to the /lib/modules/2.6.24.4/kernel/drivers/net/wireless/rt2x00/rt73_ts7500.ko (note this is in the forth partition of the sd-card)o


Run the kernel from the sd-card
-------------------------------

* Put the jumpers in the development board: JP1 = ON; JP2 = OFF

* Do a depmod -a to do all module dependencies

Copy the kernel and initrd to the flash in the arm
--------------------------------------------------

* On my pc (I cannot to this in the card) I copy the sdb2 and sdb3 partitions to two files and then use those files to copy to the flash. This is how to do it.

* Put the sd-card on the pc

* dd if=/dev/nbd2 of=/tmp/zImage
* dd if=/dev/nbd3 of=/tmp/initrd
* Copy both these files to the /dev/ndb4 file system (mount it!!!!!!)

* Put the sd-card on the arm and then turn it up

* In the arm do:

  * spiflashctl -W 4095 -z 512 -k part1 -i /whatever/zImage
  * spiflashctl -W 32 -z 65536 -k part2 -i /whaterver/initrd
  * sync

Note that you probably can do the first two dd's in the card, but I always get an error doing it. It create enormous files.

Run from the flash
------------------

* Put both jumpers off

* Hope for the best
