PART_BOOTABLE     equ 80h
PART_NOT_BOOTABLE equ 00h

PART_FAT12        equ 01h
PART_FAT16			  equ 06h
PART_FAT32        equ 0Bh

;;;;;;;;
;; Converts CHS values to 3 byte partition table values.
;; 
%macro chs 3
	db %2  ; heads
	db (%3 + ((%1 & 300h) >> 2)) 
	db (%1 & 0FFh)
%endmacro

;;;;;;;;
;; Calculates the LBA sector for a CHS value 
;;
%define chs_sector(c, h, s) ((c * HEADS * SECTORS) + ((h) * SECTORS) + (s - 1))

;;;;;;;;
;; Macro to define a partition table entry
;;
;; - Bootable 
;; - Starting C,H,S
;; - Partition Type
;; - Ending C,H,S
;; - Starting Sector Absolute (dd)
;; - Partition Size in Secotrs (dd)
%macro part_entry 8
	db %1                    ; Bootable
	chs %2, %3, %4           ; Starting CHS
	db %5                    ; Partition Type
	chs %6, %7, %8           ; Ending CHS
	dd chs_sector(%2, %3, %4) ; Starting Sector Absolute
	dd (chs_sector(%6, %7, %8) - chs_sector(%2, %3, %4) + 1)
%endmacro


