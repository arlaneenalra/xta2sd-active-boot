#ifndef TOOL_H
#define TOOL_H

#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct config_type {

  uint8_t drive;

  bool dump;
  bool output_partition_table;
  bool ignore_partition_table;

  // Actions  to take
  bool write;
  bool load_tsr;

  // Some basic state values
  bool mbr_active; 
  bool tsr_active;
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

#pragma pack(__push, 1)
typedef struct hdd_data_block_type {
  uint16_t cylinders;
  uint8_t heads;
  uint16_t reduced_write_cylinder;
  uint16_t write_precomp_cylinder;
  uint8_t max_ecc_burst;
  uint8_t control_byte;
  uint8_t timeout;
  uint8_t format_timeout;
  uint8_t check_timeout;
  uint16_t landing_zone_cylinder;
  uint8_t sectors_per_track;  // On the Tandy 1110HD this is hard set to 17 and the value is always 0.
  uint8_t reserved;
} hdd_data_block_t;

/**
 * Data structure to represent the resident TSR part of the application when loaded.
 */
typedef struct tsr_resident_type {
  hdd_data_block_t hdd_data;
  uint32_t signature; // used to tell if the TSR is actually loaded or not.
} tsr_resident_t;


/**
 * Way over simplified value used to identify the TSR in memory.
 */
#define TSR_SIGNATURE 0xAABBCCDD
/**
 * The offset into the MBR that we find the actuall hdd_data_table.
 */
#define MBR_TABLE_OFFSET 0x0004

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

  hdd_data_block_t hdd_data; // (16 bytes) The hdd data table for our patched boot sector.
  uint32_t signature; 

  /**
   * This should be the raw code of the boot sector. The partition table should
   * exist at 0x1BE, to find it correctly we need to add place holder dummy bytes.
   */
  uint8_t __code[0x1BE - sizeof(far_ptr_t) - sizeof(hdd_data_block_t) - sizeof(uint32_t)];

  /**
   * There are four slots for partitions in the partition table.
   */
  partition_entry_t partition_table[4]; // (64 bytes)

  /**
   * Boot sector signature. For a valid boot sector, this should always
   * be 0xAA55
   */
  uint16_t mbr_signature;
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

#pragma pack(__pop)

int usage();
void nasty_warning();
int parse_arguments(int argc, char **argv, config_t *config);
void dump_sector(uint8_t __far *buf);

uint8_t read_boot_sector(uint8_t drive, void __far *buf);
uint8_t write_boot_sector(uint8_t drive, void __far *buf);
const char *translate_error(uint8_t err_code);

void print_partition_table(partition_entry_t *table);

uint8_t write_mbr(config_t *config, mbr_t *original);

far_ptr_t get_boot_sector_patch_ptr();
bool is_boot_sector_patch_active();
bool is_tsr_patch_active();
bool is_drive_patched();

void load_tsr(config_t *config, hdd_data_block_t *hdd_data);

#define DRIVE_0 0x80
#define DRIVE_1 0x81

#endif
