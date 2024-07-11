#include <stdlib.h>
#include <conio.h>
#include <dos.h>

#include "tool.h"

/**
 * Reference to precompiled bootsector.
 */
extern unsigned int boot_boot_bin_len;
extern unsigned char boot_boot_bin[];

extern hdd_data_block_t resident_hdd_data; 
extern uint16_t tsr_data_paragraphs;


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
  printf("  /d # Drive to operate on.\n");
  printf("  /r   Dump the loaded boot sector as hex.\n");
  printf("  /p   Output current partition table.\n");
  printf("  /w   Overwrite the boot sector with our active version.\n");
  printf("       By default, this preserves the existing partition\n");
  printf("       table.\n");
  printf("  /n   Overwrite the existing partition table with a default\n");
  printf("       one. Only makes sense with /w.\n");
  printf("  /t   Load the TSR version of the bios patch and replace the\n");
  printf("       memory inefficient boot sector patch if it's loaded.\n");
  printf("\n");

  return 1;
}

int parse_arguments(int argc, char **argv, config_t *config) {
  int parsed = 0;
  int c;

  // See: https://open-watcom.github.io/open-watcom-v2-wikidocs/clib.html#getopt
  while((c = getopt(argc, argv, "d:rpwnt")) != -1) {
    parsed++;
    switch(c) {
      case 'd':
        // Drive can only be a byte value.
        config->drive = atoi(optarg) & 0xFF;
        break;

      case 'r':
        // Read the boot sector and dump it as raw hex
        config->dump = true;
        break;

      case 'n':
        config->ignore_partition_table = true;
        break;
      
      case 'p':
        config->output_partition_table = true;
        break;

      case 'w':
        config->write = true;
        break;

      case 't':
        config->load_tsr = true;
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
 * Check to see if the boot in the buffer has our signature.
 */
bool is_drive_patched(mbr_t *sector_buf) {
  return sector_buf->mbr.signature == TSR_SIGNATURE;
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
  if (!config->ignore_partition_table) {
    printf("Found existing partition table, copying.\n");
    memcpy(
        write_buf.mbr.partition_table,
        (original->mbr.partition_table),
        sizeof(partition_entry_t) * 4);

  }

  printf("Using partition table:\n");
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
  far_ptr_t mbr_expected;
  config_t config;
  mbr_t sector_buf;
  uint8_t status;

  tsr_data_paragraphs = (0x100 + FP_OFF(&tsr_data_paragraphs) + 15) >> 4;

  memset((void *)&config, 0, sizeof(config_t));

  // setup defaults
  config.mbr_active = is_boot_sector_patch_active();
  config.tsr_active = is_tsr_patch_active(); 

  // zero out the sector buffer
  memset(&sector_buf, 0, sizeof(sector_buf));

  mbr_expected = get_boot_sector_patch_ptr();
  
  if (parse_arguments(argc, argv, &config)) {
    return usage();  
  }

  printf("Boot sector patch expected at %04X:%04X -> %s\n",
      mbr_expected.vector.segment, mbr_expected.vector.offset,
      config.mbr_active ? "loaded" : "not loaded");

  printf("TSR patch size %0u bytes.\n", tsr_data_paragraphs * 16);
  printf("TSR patch -> %s\n", config.tsr_active ? "loaded" : "not loaded");

  // Check to see if we can read from sector 0, 0, 0 on the primary
  // hard drive.
  printf("Reading boot sector from fixed disk %u -> ", config.drive);
 
  status = read_boot_sector(config.drive, &sector_buf.buffer);

  if (status != 0) {
    printf("Error %02X -> %s\n", status, translate_error(status));
    printf("Aborting!\n");
    
    // If we can't read the disk, there's nothing we can do.
    return 1;
  } else if (is_drive_patched(&sector_buf)) {
    printf("patched\n");
  } else {
    printf("not patched\n");  
  }

  if (config.output_partition_table) {
    print_partition_table(sector_buf.mbr.partition_table);
  } 

  if (config.dump) { 
    dump_sector(&sector_buf.buffer);
  }

  // If we're loading the TSR, this will not return.
  if (config.load_tsr) {
    // Copy data from MBR
    if (config.mbr_active) {
      printf("Repatching with MBR data.\n");
      load_tsr(&config, &sector_buf.mbr.hdd_data);
    } 

    printf("Loading default patch.\n");
    load_tsr(&config, &(((mbr_t *)boot_boot_bin)->mbr.hdd_data));
  } 

  if (config.write) {
    status = write_mbr(&config, &sector_buf);

    if (status) {
      return 1; 
    } 
  }

  return 0;
}
