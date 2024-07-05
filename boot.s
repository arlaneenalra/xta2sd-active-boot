;; A small boot sector that allows overriding the hard drive size on a 
;; Tandy 1110HD. This will require sacrificing roughly 1k of memory to
;; to make work.
;;
;; A large portion of this comes from https://thestarman.pcministry.com/asm/mbr/STDMBR.htm 

CYLINDERS        equ 0384h
HEADS            equ 0Fh
SECTORS          equ 11h   ; This is fixed on the Tandy 1110HD and most similar driveis

START            equ 0000h
PART_TABLE       equ START + 01BEh
END              equ START + 01FEh

BOOT_SHADOW      equ START 
BIOS_LOADED      equ 7C00h ; The bios actually loads us at this address

FREE_MEM         equ 0413h ; Location where the amount of free memory in kilobytes is stored

HDD_DATA_OFFSET  equ 0104h ; Address of interrupt vector that points to HDD data block
HDD_DATA_SEGMENT equ 0106h

DEBUG equ 4h


  cpu 8086
  bits 16
  org START 

;;;;;;;;
;; Display a message using bios iterrupts.
;;
%macro write_message 1
  mov si, %1
  call output_message
%endmacro

%include "debug.inc"
%include "macro.inc"
%include "partition.inc"

main:
  ;; We're relying on the jmp being 2 bytes
  jmp init_entry ; Jump to the real entry point
  dw 0000h

  ;; The bios data block used to set hardisk parameters
  ;; See: https://fd.lod.bz/rbil/interrup/bios/41.html
hdd_data_block:
  dw 0384h      ; Number of cylinders
  db 0Fh        ; Number of heads (This might max out at 0Fh
  dw 0384h      ; Starting reduced write current cylinder
  dw 012Ch      ; Starting write precompensation cylinder number
  db 0Bh        ; Maximum ECC burst length
  db 05h        ; Control byte - 00h -> 3ms, 04h -> 200ms, 05h -> 70ms, 06h -> 3ms, 07h -> 3ms
  db 63h        ; Standard timeout
  db 0FFh       ; Fromatting timeout
  db 73h        ; Timeout for checking drive
  dw 0000h      ; Cylinder number for landing zone
  db 0h         ; Number of sectors per track (AT only locked at 17 on Tandy 1110HD)

; Testing values for dosbox-x 
;  dw 0383h      ; Number of cylinders
;  db 0Fh        ; Number of heads (This might max out at 0Fh
;  dw 0000h      ; Starting reduced write current cylinder
;  dw 0FFFFh     ; Starting write precompensation cylinder number
;  db 00h        ; Maximum ECC burst length
;  db 0C8h       ; Control byte - 00h -> 3ms, 04h -> 200ms, 05h -> 70ms, 06h -> 3ms, 07h -> 3ms
;  db 00h        ; Standard timeout
;  db 00h        ; Fromatting timeout
;  db 00h        ; Timeout for checking drive
;  dw 0383h      ; Cylinder number for landing zone
;  db 11h        ; Number of sectors per track (AT only locked at 17 on Tandy 1110HD)

HDD_DATA_BLOCK_SIZE equ ($ - hdd_data_block)

  ;; Setup and copy the boot sector from 7C00h to the traditional
  ;; 0600h

init_entry:

  cli                  ; Disable interrupts
  xor ax, ax           ; Zero ax
  mov ss, ax           ; Set the stack segment to zero
  mov sp, BIOS_LOADED  ; Set the stack pointer to directly above where bios loaded us.
  mov si, sp           ; Set the source index to where bios loaded us.
  mov ds, ax           ; Setup ds

  ;; Now we need to calculate where we're going to store the bios block
  mov ax, [FREE_MEM]   ; Load the number of free kilobyes of memory.
  sub ax, 1h           ; Subtract 1k, we'll officially steal it later if we need it. 

  mov dx, 0400h        ; 1k
  mul dx               ; Get the value in the number of bytes and store it in dx:ax

  mov cl, 0Ch          ; We need to shift dx left 12 bits
  shl dx, cl
  mov cl, 04h          ; We need to shift ax right 4 bits
  shr ax, cl
  
  or ax, dx            ; This gives us the segment address for the last 1k of ram

  mov es, ax           ; Setup es
  sti                  ; Enable interrupts.

  ;; Sacrifice the jmp and padding at the start of this code to get a far pointer for the
  ;; jump. We need to do this before the copy, because it's very tricky to get the address
  ;; right.
  mov [BIOS_LOADED + 2 ], ax ; should point to main
  mov dx, patch_bios
  mov [BIOS_LOADED], dx
  
  cld
  mov DI, BOOT_SHADOW
  mov cx, 0100h ; copy the whole boot sector (We're copying words so this is 2 bytes at a time.)
  rep movsw

  ; Jump to the copied code.
  ; Note: this address shows up incorrectly in the lst file for some reason.
  jmp far [ES:0000h]
  

patch_bios:
  write_message loaded_msg

  ;; Save of the original hdd data vector
  mov ax, [ss:HDD_DATA_OFFSET]   ; Load the original hdd table offset.
  mov [cs:0000h], ax             ; Preserve it in case we need to restore it later.
  mov di, ax                     ; Setup di for the compare.
  mov ax, [ss:HDD_DATA_SEGMENT]  ; Load the original hdd table segment.
  mov [cs:00002h], ax            ; Preserve it in case we need to restore it later.
  mov es, ax                     ; Setup es for the compare.

  ;; Check if the bios already mathces our specs.
  ;; We're going to go byte by byte and compare hdd_data_block
  ;; to the bios table. If they match, we're good, otherwise,
  ;; we need to patch the bios.

  mov cx, HDD_DATA_BLOCK_SIZE ; Get the size of the data block

  ;; Setup the bios data block for compare
;  mov di, [ss:HDD_DATA_OFFSET]
;  mov ax, [ss:HDD_DATA_SEGMENT]
;  mov es, ax


  ;; Setup our hdd data block for compare
  mov bx, cs
  mov ds, bx
  mov si, hdd_data_block

  cld
  repe cmpsb
  jcxz .real_boot_notify
  jmp .apply_patch
  

.real_boot_notify:
  write_message not_patched_msg
  
  jmp .real_boot

.apply_patch: 
  ;; This should protect the patch data from being overwritten by the OS.

  mov ax, [ss:FREE_MEM]   ; Load the number of free kilobyes of memory.
  sub ax, 1h           ; Take 1k for this code
  mov [ss:FREE_MEM], ax   ; Save the updated value back to bios data so we don't get clobbered

  ;; Update the interrupt vector
  mov ax, hdd_data_block
  mov [ss:HDD_DATA_OFFSET], ax
  mov ax, cs
  mov [ss:HDD_DATA_SEGMENT], ax

  write_message patched_msg


.real_boot:
  xor ax, ax            ; reset ds to 0000h
  mov es, ax
  mov ds, ax 

;; Now we attempt to actually boot a partition
  mov si, part_table    ; Address of the partition table
  mov bl, 04h           ; There are only 4 partition table entries.


.find_part:
  cmp byte [cs:si], PART_BOOTABLE ; Check to see if the partition is bootable.
  jz attempt_boot

  cmp byte [cs:si], PART_NOT_BOOTABLE ; Is this a valid partition entry?
  jnz invalid_partition

  add si, 0Ah           ; Increment to the next partition table entry
  dec bl

  jnz .find_part
  int 18h               ; Follow the same pattern as the stock bootsector, call rom basic interrupt.

  jmp $

attempt_boot:
  ;; if we get here, we've found a seemingly valid partiton to boot.
  mov dx, [cs:si]       ; This should put the drive in dh and the head for the start
                        ; of the partition in dl 
  mov cx, [cs:si + 02h] ; This should put the starting cylinder in ch and
                        ; first sector in cl
  mov bp, si            ; Save offset of the Active partition table entry to pass to the
                        ; Volume Boot Sector.

  mov di, 0005h         ; We'll try booting 5 times

.retry_loop:
  mov bx, BIOS_LOADED   ; Put the boot sector where the bios was supposed to.
  mov ax, 0201h         ; load one sector.

  push di
  int 13h               ; try to load the boot sector
  pop di

  jnc .loaded_fine      ; If the sector loaded, don't attempt to reset the drive.

  ;; If we get here, we couldn't load the os from disk
  write_message error_loading_msg

  xor ax, ax
  int 13h               ; Reset the drive
  
  dec di
  jnz .retry_loop 

  ;; Failed loading the os.
  write_message no_more_retries
  jmp $ 

.loaded_fine:
  mov di, BIOS_LOADED + 01FEh   ; Point to the laster word of the boot sector
  cmp word [di], 0AA55h          ; Check for a boot sector signature.
  jnz missing_os


  mov si, bp                    ; This is used by the OS Boot code to find
                                ; the offset of the Active Partition Entry
  jmp 0000h:BIOS_LOADED         ; Jump to the OS boot sector.

missing_os:
  write_message missing_os_msg
  jmp $

invalid_partition:
  write_message invalid_msg
  jmp $

output_message:
  push ds
  mov ax, cs
  mov ds, ax
  mov ah, 0Eh
.loop:
  lodsb
  or al, al
  jz .done 
  int 10h
  jmp .loop

.done:
  pop ds
  ret

loaded_msg:
  db "HDD Patch: "
  db 00h

patched_msg:
  db "Loaded."
  db 0Dh, 0Ah, 00h

not_patched_msg:
  db "Not needed."
  db 0Dh, 0Ah, 00h

invalid_msg:
  db "Invalid partition table."
  db 0Dh, 0Ah, 00h

error_loading_msg:
  db "Error loading operating system."
  db 0Dh, 0Ah, 00h

no_more_retries:
  db "Fatal disk error."
  db 0Dh, 0Ah, 00h

missing_os_msg:
  db "Missing OS!"
  db 0Dh, 0Ah, 00h

  setloc PART_TABLE
part_table:
  ;; Define a partition table.
  part_entry PART_BOOTABLE, 00h, 01h, 01h, PART_FAT16, 382h, 0Eh, 11h

  setloc END
boot_label:
  dw 0AA55h    ; Required for bios to see this as a boot sector.

