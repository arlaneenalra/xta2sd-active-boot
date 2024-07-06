#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tool.h"

/**
 * Reference to precompiled bootsector.
 */
extern unsigned int boot_boot_bin_len;
extern unsigned char boot_boot_bin[];

/**
 * Pointer to the interrupt vector table. We need this
 * to see if we're using the active boot sector code
 * right now.
 */
uint16_t __far *memory_size = (uint16_t __far *)0x00000413;
far_ptr_t __far *hdd_table_ptr = (far_ptr_t *)0x00000104;

/**
 * Output the raw contents of the the booiit sector.
 */
void dump_sector(uint8_t __far *buf) {
  int i, j;

  printf("Raw Sector:\n");

  for (i = 0; i < 0x200; i += 0x10) {
    printf("%04X:", i);

    for (j = 0; j < 0x10; j++) {
      printf(" %02X", buf[i + j]);
    }

    printf("\n");
  }
}

/**
 * Output a usage statment and exit.
 */
int usage() {
  printf("Active boot sector patching tool.\n");
  printf("Usage:\n");
  printf("  tool [/f] [/d #] [/r]\n");
  printf("\n");
  printf("  /f   Force the issue.\n");
  printf("  /d # Drive to operate on.\n");
  printf("  /r   Dump the loaded boot sector as hex.\n"); 
  printf("\n");

  return 1;
}

int parse_arguments(int argc, char **argv, config_t *config) {
  int parsed = 0;
  int c;

  // See: https://open-watcom.github.io/open-watcom-v2-wikidocs/clib.html#getopt
  while((c = getopt(argc, argv, "fd:r")) != -1) {
    parsed++;
    switch(c) {
      case 'f':
        printf("Forcing the issue.\n");
        break;

      case 'd':
        // Drive can only be a byte value.
        config->drive = atoi(optarg) & 0xFF;
        break;

      case 'r':
        // Read the boot sector and dump it as raw hex
        config->dump = 1;
        break;

      case ':':
        printf("%c requires and argument.\n", optopt);
        return 1;
        break;

      case '?':
      default:
        printf("Unknown argument '%c'\n", optopt);
        return 1; 
    }
  }

  return 0;
}
 
int main(int argc, char **argv) {
  config_t config = {
    0x00, // set drive to point at the first hard drive. 
    0x00  // default to not dumping the sector.
  };
  uint8_t sector_buf[1024];
  uint8_t status;

  // zero out the sector buffer
  memset(sector_buf, 0, sizeof(sector_buf));
  
  if (parse_arguments(argc, argv, &config)) {
    return usage();  
  }

  printf("Current hdd_table located at %04X:%04X -> %08lX\n",
      hdd_table_ptr->vector.segment,
      hdd_table_ptr->vector.offset,
      hdd_table_ptr->ptr);

  printf("Bios is currently reporting %uK conventional memory free.\n",
      *memory_size);

 
  // Check to see if we can read from sector 0, 0, 0 on the primary
  // hard drive.
  printf("Loading boot sector from %u:\n", config.drive);
 

  status = read_boot_sector(config.drive, sector_buf);

  if (status != 0) {
    printf("Error reading existing boot sector: %02X\n", status);
  }

  if (config.dump) { 
    dump_sector(sector_buf);
  }

  return 0;
}
