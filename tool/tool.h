#ifndef TOOL_H
#define TOOL_H

#include <unistd.h>
#include <stdint.h>

typedef struct config_type {
  uint8_t drive;
  uint8_t dump;
  uint8_t output_partition_table;
  uint8_t write;
} config_t;

/**
 * Allows accessing the segment and offset addresses of an interrupt vector.
 * They are reall just far pointers in the end. There's probably a better way
 * to do this, but this does work.
 */
typedef struct segment_offset_type {
  uint16_t offset;
  uint16_t segment;
} segment_offset_t;


typedef union far_ptr_type {
  segment_offset_t vector;
  void __far *ptr;
} far_ptr_t;

typedef struct hdd_data_block_type {
  uint16_t cylinders;
  uint8_t heads;
  uint16_t reduced_write_cylinder;
  uint16_t write_precomp_cylinder;
  uint8_t  max_ecc_burst;
  uint8_t control_byte;
  uint8_t timeout;
  uint8_t format_timeout;
  uint8_t check_timeout;
  uint16_t landing_zone_cylinder;
  uint8_t sectors_per_track;  // On the Tandy 1110HD this is hard set to 17 and the value is always 0.
  uint8_t reserved;
} hdd_data_block_t;

#define BOOTABLE 0x80
#define NOT_BOOTABLE 0x00
#define MBR_SIGNATURE 0xAA55

/**
 * A partition table entry in raw, on disk form.
 */
typedef struct partition_entry_type {
  uint8_t bootable;
  
  /**
   * Cyliners/heads/sectors value for this partion in raw on disk form.
   */
  uint8_t starting_chs[3];

  uint8_t partition_type;

  /**
   * Cyliners/heads/sectors value for this partion in raw on disk form.
   */
  uint8_t ending_chs[3];

  uint32_t starting_sector; // Starting sector in absolute form.
  uint32_t ending_sector; // Ending sector in abolute form. 

} partition_entry_t;

/**
 * A container type that allows access to specific parts of the boot sector.
 */ 
typedef struct boot_sector_type {
  /**
   * When loaded from disk this points to a jump instruction and a 2 byte buffer.
   * In memory, these are replaced with the original value of the int 41h vector.
   */
  far_ptr_t original_vector; // (4 bytes)

  hdd_data_block_t hdd_data; // (17 bytes) The hdd data table for our patched boot sector.

  /**
   * This should be the raw code of the boot sector. The partition table should
   * exist at 0x1BE, to find it correctly we need to add place holder dummy bytes.
   */
  uint8_t __code[0x1BE - sizeof(far_ptr_t) - sizeof(hdd_data_block_t)];

  /**
   * There are four slots for partitions in the partition table.
   */
  partition_entry_t partition_table[4]; // (64 bytes)

  /**
   * Boot sector signature. For a valid boot sector, this should always
   * be 0xAA55
   */
  uint16_t signature;
} boot_sector_t;

/**
 * Macros for reading CHS values from on disk form.
 */
#define CHS_HEAD(chs) chs[0]
#define CHS_SECTOR(chs) (chs[1] & 0x3F)
#define CHS_CYLINDER(chs) ((((uint16_t)chs[1] & 0xC0) << 2) + chs[2])

/**
 * Union type for dealing with the raw boot sector.
 */
typedef union mbr_buffer_type {
  boot_sector_t mbr;
  uint8_t buffer[512];
} mbr_t;

int usage();
void nasty_warning();
int parse_arguments(int argc, char **argv, config_t *config);

uint8_t read_boot_sector(uint8_t drive, void __far *buf);
uint8_t write_boot_sector(uint8_t drive, void __far *buf);
const char *translate_error(uint8_t err_code);

void print_partition_table(partition_entry_t *table);

uint8_t write_mbr(config_t *config, mbr_t *original);


#define DRIVE_0 0x80
#define DRIVE_1 0x81

#endif
