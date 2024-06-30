A boot sector that is designed to work with a Tandy 1110HD and an xta2sd card to allow it to boot a 112mb hdd image on the stock rom.

PRE-Alpha use at your own risk!

To build an image take an existing image of the correct size, right now this works with 900, 15, 17 CHS defined image (384h, 0Eh, 11h). For the moment, this image should have a bootable FAT16 partition at (0, 1, 1) that extends to (898, 14, 17). i.e. Single DOS partition that covers the whole disk.


* Run `dd if=<image file> bs=512 skip=1 outfile=boot.img` to strip the original boot/partition table off.
* Run `make boot.bin` to build the boot sector
* Run `cp boot.bin boot.img > real_image.img` to produce the bootable image.

This should work so long as you have NASM installed and are on a unix like system.

