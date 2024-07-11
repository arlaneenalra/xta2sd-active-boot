#include <i86.h>

#include "tool.h"

/**
 * Translates the error code reported by bios.
 */
const char *translate_error(uint8_t err_code) {
  const char *unknown = "Unknown error!";

  switch (err_code) {
    case 0x00:
      return "Success";
      break;

    case 0x01:
      return "Invalid Command";
      break;

    case 0x02:
      return "Cannot Find Address Mark";
      break;
    
    case 0x03:
      return "Attempted Write On Write Protected Disk";
      break;
  
    case 0x04:
      return "Sector Not Found";
      break;

    case 0x05:
      return "Reset Failed";
      break;

    case 0x06:
      return "Disk change line 'active'";
      break;

    case 0x07:
      return "Drive parameter activity failed";
      break;

    case 0x08:
      return "DMA overrun";
      break;

    case 0x09:
      return "Attempt to DMA over 64kb boundary";
      break;

    case 0x0A:
      return "Bad sector detected";
      break;

    case 0x0B:
      return "Bad cylinder (track) detected";
      break;

    case 0x0C:
      return "Media type not found";
      break;

    case 0x0D:
      return "Invalid number of sectors";
      break;

    case 0x0E:
      return "Control data address mark detected";
      break;

    case 0x0F:
      return "DMA out of range";
      break;

    case 0x10:
      return "CRC/ECC data error";
      break;

    case 0x11:
      return "ECC corrected data error";
      break;

    case 0x20:
      return "Controller failure";
      break;

    case 0x40:
      return "Seek failure";
      break;

    case 0x80:
      return "Drive timed out, assumed not ready";
      break;

    case 0xAA:
      return "Drive not ready";
      break;

    case 0xBB:
      return "Undefined error";
      break;
    
    case 0xCC:
      return "Write fault";
      break;

    case 0xE0:
      return "Status error";
      break;

    case 0xFF:
      return "Sense operation failed";
      break;
  }

  return unknown;
}

/**
 * Read the boot sector from the given drive.
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
 
  regs.w.bx = buf_ptr.vector.offset;
  sregs.es = buf_ptr.vector.segment;

  int86x(0x13, &regs, &regs, &sregs);

  // The status value is in ah
  return regs.h.ah; 
}

/**
 * Write the boot sector form the in memory buffer.
 */
uint8_t write_boot_sector(uint8_t drive, void __far *buf) {
  far_ptr_t buf_ptr;
  union REGS regs;
  struct SREGS sregs;

  // makes it a bit easier to access the segment and offset.
  buf_ptr.ptr = buf;

  segread(&sregs);

  regs.h.ah = 0x03; // Function 0x03 - Write sector.
  regs.h.al = 0x01; // Number of sectors write.
  regs.h.ch = 0x00; // Cylinder (0 indexed);
  regs.h.cl = 0x01; // Sector (1 indexed);
  regs.h.dh = 0x00; // Head (0 indexed);

  // Drive to read. Hard drives are always above 0x80
  regs.h.dl = 0x80 + drive; 
 
  
  regs.w.bx = buf_ptr.vector.offset;
  sregs.es = buf_ptr.vector.segment;

  int86x(0x13, &regs, &regs, &sregs);

  // The status value is in ah
  return regs.h.ah; 
}

/**
 * Output the current partition table.
 */
void print_partition_table(partition_entry_t *table) {
  int i;

  printf("---------------------------------------------------------\n");
  printf("| # | Boot | Type | Starting (CHS)   |  Ending (CHS)    |\n");
  printf("---------------------------------------------------------\n");

  for (i = 0; i < 4; i++) {
    printf("| %u |  %02X  |  %02X  | (%4.u, %3.u, %3.u) | (%4.u, %3.u, %3.u) |\n",
        i,
        table[i].bootable,
        table[i].partition_type,
        CHS_CYLINDER(table[i].starting_chs),
        CHS_HEAD(table[i].starting_chs),
        CHS_SECTOR(table[i].starting_chs),

        CHS_CYLINDER(table[i].ending_chs),
        CHS_HEAD(table[i].ending_chs),
        CHS_SECTOR(table[i].ending_chs));

    printf("|=================|         %8.lu |         %8.lu |\n",
        table[i].starting_sector,
        table[i].ending_sector); 
  }

  printf("---------------------------------------------------------\n");
}
