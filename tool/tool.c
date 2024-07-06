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
 * Warns the user that taking this action could result in data loss gives
 * them a chance to back out. Does not return if the user says no.
 */
void nasty_warning() {
  int c;

  printf("Warning! Warning! Warning!\n");
  printf("  This operation has may cause loss of data on the target drive!\n");
  printf("  Make sure you have a back up prior to completing this operaton.\n");
  printf("Warning! Warning! Warning!\n");
  printf("\n");
  printf("Do you wish to proceed? (press 'Y' to continue and any other key to abort.)\n");

  c = getch();
  if (c != 'y' && c != 'Y') {
    printf("You pressed %c, aborting.\n", c);
    exit(1);
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
  printf("  /p   Output current partition table.\n");
  printf("  /w   Overwrite the boot sector with our active version.\n");
  printf("       By default, this preserves the existing partition\n");
  printf("       table.");
  printf("\n");

  return 1;
}

int parse_arguments(int argc, char **argv, config_t *config) {
  int parsed = 0;
  int c;

  // See: https://open-watcom.github.io/open-watcom-v2-wikidocs/clib.html#getopt
  while((c = getopt(argc, argv, "fd:rpw")) != -1) {
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
      
      case 'p':
        config->output_partition_table = 1;
        break;

      case 'w':
        config->write = 1;
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

/**
 * Updates the on disk MBR with our active one preserving the
 * existing partition table.
 */
uint8_t write_mbr(config_t *config, mbr_t *original) {
  uint8_t status;
  mbr_t write_buf;

  if (sizeof(mbr_t) != boot_boot_bin_len) {
    printf("FATAL ERROR: MBR structure and Boot Sector are not the same size!!!\n");
    printf("MBR Struct %u Boot Sector %u\n", sizeof(mbr_t), boot_boot_bin_len);
  }
  
  memcpy(write_buf.buffer, boot_boot_bin, sizeof(mbr_t));
 
  // Copy the partition table over from the original boot sector.
  memcpy(
      write_buf.mbr.partition_table,
      (original->mbr.partition_table),
      sizeof(partition_entry_t) * 4);


  printf("Found existing partition table:\n");
  print_partition_table(write_buf.mbr.partition_table);

  // Give users a chance to back out before nuking their drive. 
  nasty_warning();

  printf("Writing active boot sector...\n");

  // Actually write the boot sector. 
  status = write_boot_sector(config->drive, &write_buf);

  if (status) {
    printf("Error writing boot sector %02X -> %s\n", status, translate_error(status));
  }

  printf("Done!\n");

  printf("A reboot is necessary for these changes to take effect.\n");

  return status;
}
 
int main(int argc, char **argv) {
  config_t config = {
    0x00, // set drive to point at the first hard drive. 
    0x00,  // default to not dumping the sector.
    0x00 
  };

  mbr_t sector_buf;
  uint8_t status;

  // zero out the sector buffer
  memset(&sector_buf, 0, sizeof(sector_buf));
  
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
  printf("Loading boot sector from fixed disk %u: ", config.drive);
 
  status = read_boot_sector(config.drive, &sector_buf.buffer);

  if (status != 0) {
    printf("Error %02X -> %s\n", status, translate_error(status));
    printf("Aborting!\n");
    
    // If we can't read the disk, there's nothing we can do.
    return 1;
  } else {
    printf("Success!\n");
  }

  if (config.output_partition_table) {
    print_partition_table(sector_buf.mbr.partition_table);
  } 

  if (config.dump) { 
    dump_sector(&sector_buf.buffer);
  }

  if (config.write) {
    status = write_mbr(&config, &sector_buf);

    if (status) {
      return 1; 
    } 
  }

  return 0;
}
