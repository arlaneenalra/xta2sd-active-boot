#include <stdlib.h>
#include <conio.h>
#include <dos.h>

#include "tool.h"
#include "version.h"

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
  printf("  /d # Drive to operate on. This is the fixed disk number rather.\n");
  printf("  /r   Dump the loaded boot sector as hex.\n");
  printf("  /p   Output current partition table.\n");
  printf("  /w   Overwrite the boot sector with our active version.\n");
  printf("       By default, this preserves the existing partition\n");
  printf("       table.\n");
  printf("  /n   Overwrite the existing partition table with a default\n");
  printf("       one. Only makes sense with /w.\n");
  printf("  /t   Load the TSR version of the bios patch and replace the\n");
  printf("       memory inefficient boot sector patch if it's loaded.\n");
  printf("  /c # Set the number of cylinders the drive has. Defaults to\n");
  printf("       900 (0x384)\n"); 
  printf("  /h # Set the number of heads the drive has. Defaults to\n");
  printf("       15 (0x0E)\n");
  printf("\n");

  return 1;
}

int parse_arguments(int argc, char **argv, config_t *config) {
  int parsed = 0;
  int c;

  // See: https://open-watcom.github.io/open-watcom-v2-wikidocs/clib.html#getopt
  while((c = getopt(argc, argv, "d:rpwntc:h:")) != -1) {
    switch(c) {
      case 'd':
        // Drive can only be a byte value.
        config->drive = atoi(optarg) & 0xFF;
        break;

      case 'c':
        parsed = atoi(optarg);

        if (parsed > 1023 || parsed <= 0) {
          printf("Cylinders must be between 1 and 1023.\n");
          exit(1);
        }

        config->cylinders = (uint16_t)parsed;
        config->ch_override = true;
        break;

      case 'h':
        parsed = atoi(optarg);

        if (parsed > 15 || parsed <= 0) {
          printf("Heads must be between 1 and 15\n");
          exit(1);
        }

        config->heads = (uint16_t)parsed;
        config->ch_override = true;
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
 * Handles setting up the patch and mbr data that would be used
 * by the TSR or written to disk.
 */
void prepare_hdd_data(config_t *config, mbr_t *sector_buf) {
  mbr_t write_buf;

  memcpy(write_buf.buffer, boot_boot_bin, sizeof(mbr_t));

  // Copy the partition table over from the original boot sector.
  if (!config->ignore_partition_table) {
    memcpy(
        write_buf.mbr.partition_table,
        (sector_buf->mbr.partition_table),
        sizeof(partition_entry_t) * 4);
  }

  // Update the data table if we have an override value or are writing
  // to the disk.
  if (config->write || config->ch_override) {
    write_buf.mbr.hdd_data.cylinders = config->cylinders;
    write_buf.mbr.hdd_data.heads = config->heads;
    write_buf.mbr.hdd_data.reduced_write_cylinder = config->cylinders;
  }


  // Copy the data back to the sector buffer.
  memcpy(sector_buf->buffer, write_buf.buffer, sizeof(mbr_t));
}

/**
 * Updates the on disk MBR with our active one preserving the
 * existing partition table.
 */
uint8_t write_mbr(config_t *config, mbr_t *write_buf) {
  uint8_t status;

  // Give users a chance to back out before nuking their drive. 
  nasty_warning();

  printf("Writing active boot sector...\n");

  // Actually write the boot sector. 
  status = write_boot_sector(config->drive, write_buf);

  if (status) {
    printf("Error writing boot sector %02X -> %s\n", status, translate_error(status));
  }

  printf("Done!\n");
  
  printf("Please run FDISK to verify the new drive geometry and update\n");
  printf("your partition table.\n");
  printf("\n");
  printf("If the drive geometry has changed, any existing partitons will be\n");
  printf("invalid!\n");

  return status;
}
 
int main(int argc, char **argv) {
  far_ptr_t mbr_expected;
  config_t config;
  mbr_t sector_buf;
  uint8_t status;

  printf("Active Boot Sector Version: %s\n", VERSION);
  printf("Copyright (c) 2024 Chris Salch\n");
  printf("\n");
  printf("This program comes with ABSOLUTELY NO WARRANTY!\n");
  printf("This is free software, and you are welcome to redistribute it\n");
  printf("under certain conditions.  For details see the accompanying\n");
  printf("LICENSE.TXT file or visit https://www.gnu.org/licenses/gpl-3.0.html.\n");
  printf("--------------------------------------------------------------------\n");

  tsr_data_paragraphs = (0x100 + FP_OFF(&tsr_data_paragraphs) + 15) >> 4;

  memset((void *)&config, 0, sizeof(config_t));

  // Setup defaults
  config.cylinders = 900;
  config.heads = 15;

  // Check system state. 
  config.mbr_active = is_boot_sector_patch_active();
  config.tsr_active = is_tsr_patch_active(); 
  
  // zero out the sector buffer
  memset(&sector_buf, 0, sizeof(sector_buf));

  mbr_expected = get_boot_sector_patch_ptr();
  
  if (parse_arguments(argc, argv, &config)) {
    return usage();  
  }

  // Nothing will work if this is true.
  if (sizeof(mbr_t) != boot_boot_bin_len) {
    printf("FATAL ERROR: MBR structure and Boot Sector are not the same size!!!\n");
    printf("MBR Struct %u Boot Sector %u\n", sizeof(mbr_t), boot_boot_bin_len);
  }

  print_current_geometry();

  printf("Boot sector patch expected at %04X:%04X -> %s\n",
      mbr_expected.vector.segment, mbr_expected.vector.offset,
      config.mbr_active ? "loaded" : "not loaded");

    // Check to see if we can read from sector 0, 0, 0 on the primary
  // hard drive.
  printf("Boot sector from fixed disk %u -> ", config.drive);
 
  status = read_boot_sector(config.drive, &sector_buf.buffer);

  if (status != 0) {
    printf("Error %02X -> %s\n", status, translate_error(status));
    printf("Aborting!\n");
    
    // If we can't read the disk, there's nothing we can do.
    return 1;
  } else if (is_drive_patched(&sector_buf)) {
    printf("patched Geometry: Cylinders=%i Heads=%i\n",
        sector_buf.mbr.hdd_data.cylinders,
        sector_buf.mbr.hdd_data.heads);
  } else {
    printf("not patched\n");  
  }

  printf("TSR patch -> %s\n", config.tsr_active ? "loaded" : "not loaded");

  if (config.output_partition_table) {
    print_partition_table(sector_buf.mbr.partition_table);
  } 

  if (config.dump) { 
    dump_sector(&sector_buf.buffer);
  }

  // Prep data for TSR/MDR overwrite.
  prepare_hdd_data(&config, &sector_buf);

  // The geometry can be different that what was reported above.
  if (config.load_tsr || config.write) {
    printf("Using Geometry: Cylinders=%i Heads=%i\n",
      sector_buf.mbr.hdd_data.cylinders,
      sector_buf.mbr.hdd_data.heads);
  }

  // If we're loading the TSR, this will not return.
  if (config.load_tsr) {
    load_tsr(&config, &sector_buf.mbr.hdd_data);
  }

  if (config.write) {
    if (config.tsr_active) {
      printf("TSR is already loaded, updating to match geometry.\n");
      update_tsr(&sector_buf);
    } 

    return write_mbr(&config, &sector_buf);
  }

  return 0;
}
