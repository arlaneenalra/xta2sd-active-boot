#include <i86.h>

#include "tool.h"

/**
 * Attempt to read the boot sector from the given drive.
 */
uint8_t read_boot_sector(uint8_t drive, void __far *buf) {
  far_ptr_t buf_ptr;
  union REGS regs;
  struct SREGS sregs;

  // makes it a bit easier to access the segment and offset.
  buf_ptr.ptr = buf;

  segread(&sregs);

  regs.h.ah = 0x02; // Function 0x02 - Read sector.
  regs.h.al = 0x01; // Number of sectors read.
  regs.h.ch = 0x00; // Cylinder (0 indexed);
  regs.h.cl = 0x01; // Sector (1 indexed);
  regs.h.dh = 0x00; // Head (0 indexed);

  // Drive to read. Hard drives are always above 0x80
  regs.h.dl = 0x80 + drive; 
 
  // 
  regs.w.bx = buf_ptr.vector.offset;
  sregs.es = buf_ptr.vector.segment;

  int86x(0x13, &regs, &regs, &sregs);

  // The status value is in ah
  return regs.h.ah; 
}


