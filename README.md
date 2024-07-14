# Tandy 1110HD Active Boot Sector

## Warning

This is alpha quality software that is provided with no warranty or guarantee.
It has only been tested on a Tandy 1110HD.

## Overview 

The Tandy 1110HD is in interesting little machine V20 based laptop produced in the early 90s.
It's not the most capable or most interesting machine out there, but it does hold a bit of nostalgia value for those of use who actually used them.
Unfortunately, these machines used Connor CP2024 20Mb hard drives with an XTA (also know as 8-bit IDE) interface and a known failure mode as they age.
There's a bumper inside the hard drive that keeps the head arm from slamming into a magnet that degrades until and allows the arm to become permanently stuck.
XTA drives are rare to begin with and the Tandy 1110HD is "bios locked" to a specific configuration of 20Mb drive.

Thankfully, there is an alternative in the [XTA2SD device](https://forum.vcfed.org/index.php?threads/8-bit-ide-xta-replacement-project.1224016/post-1393231).
It works quite nicely with a 20Mb image, but you're still stuck with that fixed 20Mb geometry. 
That's where this little toy comes into play.

## Theory of Operation

I started a tear down of a Tandy 1110HD and its bios chip in a [series of blog posts](https://chrissalch.com/tags/tandy-1110hd/).
The bios rom for the 1110HD contains a drive parameter table at F000:D827 that is pointed to by INT 41h.
So, if you check address 0000:0104 after the machine is booted with a stock bios, you should find that address and a data table that is 16 bytes in length.
This table contains the CHS style definition of the Connor CP2024 20Mb hard drive minus the number of sectors per cylinder/track.
For this drive, that value is locked to 17 and cannot be changed without deeper modifications.
The nice part of this is that the table can be replaced *after boot* and the bios routines work just fine using the new definitions.

## The Boot Sector

That's where the boot sector part of this tool comes into play.
Even with the incorrect definition, the bios routines are capable of reading the very first sector on the drive.
They could probably read the entire first cylinder and head if we really needed to, but it was possible to pack everything into one sector.
The boot sector in this repo, checks to see if its internal data table matches the one in bios and does nothing if it does.
This allows those of us with a spare eprom and burner to patch the bios chip and avoid playing software games.
If not, the bios loads itself into the last available kilobyte of memory and subtracts it from the reported bios values.
One kilobyte is more than we really need (16 bytes or so plus a signature flag) but it's all the minimum we can protect this early in the boot cycle.

After this point, the boot process can proceed normally with a standard partition table.
In fact, initial testing with DOS 6.22 showed that standard fdisk should be safe to use.

The assembly code for the boot sector is pre-configured with a 112Mb drive definition and single partition that takes up the whole drive.
I have not provided a pre-built image, but if you start with a 112Mb image configured with 900 cylinders, 15 heads, and 17 tracks (384h, 0Eh, 11h in hex respectively) creating your own is pretty straight forward:

* Run `dd if=<image file> bs=512 skip=1 outfile=boot.img` to strip the original boot/partition table off.
* Run `make boot.bin` to build the boot sector
* Run `cp boot.bin boot.img > real_image.img` to produce the bootable image.

The commands above should work on just about any unix like system and boil down to cutting off the first 512 bytes of the image and replacing it with our custom boot sector.
My testing was accomplished with an image provided by the Peter who originally created the XTA2SD device. (Many thanks for this amazing little piece of technology!).

## The DOS TSR and Tool

All well and good if you can boot from the XTA2SD, but what if you can't?
That's where the *tool.exe* comes into play.
*Tool.exe* is completely self contained and provides a patching tool as well as a TSR that can be used to produce the same effect as the boot sector.
Since it's a little more memory efficient, it is recommended to use the TSR *after* booting with the patched boot sector to free up that last kilobyte of ram.
The TSR version burns about 288 bytes of memory vs the kilobyte used by the boot sector hack.

# How to Use Tool.exe

## Checking Status

If you run *tool.exe* on its own with no arguments you get a copyright message followed by some basic status information:

```
Active Boot Sector Version: local-build
Copyright (c) 2024 Chris Salch
This program comes with ABSOLUTELY NO WARRANTY!
This is free software, and you are welcome to redistribute it
under certain conditions.  For details see the accompanying
LICENSE.TXT file or visit https://www.gnu.org/licenses/gpl-3.0.html.
--------------------------------------------------------------------
Boot sector patch expected at A000:0004 -> not loaded
TSR patch size 288 bytes.
TSR patch -> not loaded
Reading boot sector from fixed disk 0 -> not patched
```

It defaults to reading the boot sector from the first fixed disk and checks to see if:

* The boot sector is patched?
* Did the machine boot from a patched boot sector (i.e. is the boot sector data table hack in place)?
* Is the TSR currently loaded?

This should always be safe to do.

## Usage Info

Running `tool /?` will output a usage message with a description of each of the available command line flags.

## Dumping the Current Boot Sector and Partition Table

In most cases, you probably don't need to use these operations.
They are primarily included for debugging purposes.

### Dumping the Current Boot Sector

If you run `tool /r` it will output a hex representation of the current boot sector contents on the selected drive along with the status information.

### Dumping the Partition Table

If you run `tool /p` it will output the current partition table definitions in CHS for.
This will always output all for slots in the table, even of a partition is not defined.
Each entry takes two rows, with the first having the CHS version and the second listing absolute sector numbers.

## Selecting a Different Drive

Setting `/d #` will allow you to select a different drive.
The Tandy 1110HD is physically limited to one drive thanks to its internal hardware.
This flag is mostly useful if you're building an image with a tool like [dosbox-x](https://dosbox-x.com/) or on another system.

## Patching the Boot Sector

### Keeping Existing Partitions

WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING! WARNING!  WARNING!  WARNING!  WARNING!

Please be aware this is an inherently dangerous operation.
While it is extremely unlikely that you will damage hardware doing this, it is entirely possible to lose data.
Make sure you have backups of any images and data prior to attempting to use this tool.

WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING! WARNING!  WARNING!  WARNING!  WARNING!

Be sure to follow the directions provided with the XTA2SD to modify its internal image definition in LOWLEVEL.HEX.
Currently, this tool only patches to the 112Mb image definiton configured with 900 cylinders, 15 heads, and 17 tracks (384h, 0Eh, 11h in hex respectively).
You will need to update the LOWLEVEL.HEX file on the XTA2SD sdcard first.

Second, you will need to be able to boot from a floppy/Gotek/floppy emulator use something like [dosbox-x](https://dosbox-x.com/) to install the boot sector patch on an image.
I would recommend the `dd` based method described in the theory of operation section to avoid a chicken and egg problem setting things up.

To apply the patched boot sector to a drive use `tool /w`.
By default, this will preserve any existing partitions on the drive.
If you need to setup and partition an image on machine, I would recommend booting from a floppy and loading the TSR patch prior to partitioning the drive.

Note: This tool does not change the underlying geometry of the drive or emulate drive.
It only updates the computers understanding of that geometry.
If the drive already has partitions/data on it that was written by the computer, changing this understanding with the patch will likely make the existing data unreadable to the computer.

### Default 112Mb Partition

WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING! WARNING!  WARNING!  WARNING!  WARNING!

Please be aware this is an inherently dangerous operation.
While it is extremely unlikely that you will damage hardware doing this, it is entirely possible to lose data.
Make sure you have backups of any images and data prior to attempting to use this tool.

WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING! WARNING!  WARNING!  WARNING!  WARNING!

As with the pre-built version of the boot sector, the patching tool can create a single 112Mb partition when it patches boot sector.
Use `tool /w /n` to do this.

## Loading the TSR

To load the TSR version of the tool, use `tool /t`.
If the boot sector patch is loaded, this will remove that patch and replace it with the TSR version, freeing the 1k of allocated memory.
The TSR patch can also be used from a boot floppy to allow reading a patched drive when booted from floppy instead of through the patched boot sector.

If you are booting from a patched drive, it is recommended that you add `tool /t` to your AUTOEXEC.BAT file and load the TSR patch once booted.

## Removing the Boot Sector Patch

To remove the boot sector patch use `fdisk /mbr` on a DOS system or similar tool to rewrite the boot sector.
Again this may result in data loss so make sure to have a backup.
 
# Future Plans
* Adding flags to adjust the patched geometry to something other than a fixed 112Mb drive.
* Add partitioning logic?
* Tool to modify images on a modern machine?

# Building From Source

## Prerequisites

I use a modern Mac for most of my development work.
Though a Linux system should work as well.
You will need

* xxd
* docker or locally installed Open Watcom V2.
* nasm
* GNU make

## Building Using Docker

This uses a [docker image](https://hub.docker.com/r/arlaneenalra/watcom-docker) for the Open Watcom V2 compiler but requries `nasm` and `make` be installed on the build machine.

Only Open Watcom V2 in docker:
```
make all
```

To build completely in docker:
```
docker run --rm -it -v $(pwd):/src arlaneenalra/watcom-docker make WATCOM_DOCKER= all
```

This will build the *boot.bin* sector and *tool.exe*.

## Building Without Docker

If you have Open Watcom V2 setup locally, you can skip the docker based build by running:

```
make WATCOM_DOCKER= all
```

# FAQ

## I found a bug!

Feel free to open an issue. This project is presently managed by a single developer so that may take time.

## I have a suggestion...

Send over a PR!
I'll make no guarantees about my ability to review and merge a PR or the chances of a particular change being accepted but, it can't really hurt.

