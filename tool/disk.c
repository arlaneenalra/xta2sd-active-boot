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
    printf("---------------------------------------------------------\n");
  }
}
