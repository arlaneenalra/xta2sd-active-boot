#include <dos.h>
#include <stdlib.h>
#include <env.h>

#include "tool.h"

/**
 * Pointer to the interrupt vector table. We need this
 * to see if we're using the active boot sector code
 * right now.
 */
uint16_t __far *memory_size = (uint16_t __far *)0x00000413;
far_ptr_t __far *hdd_table_ptr = (far_ptr_t __far *)0x00000104;

extern tsr_resident_t tsr_data;
extern uint16_t tsr_data_paragraphs;

/**
 * Predict boot sector patch address.
 */
far_ptr_t get_boot_sector_patch_ptr() {
far_ptr_t predicted;

  uint32_t ram_bytes = *memory_size;
  ram_bytes *= 0x400;

  predicted.vector.segment = ((ram_bytes & 0xFFFF0) >> 4);
  predicted.vector.offset = MBR_TABLE_OFFSET;

  return predicted;
}

/**
 * Determine if the boot sector patch is active was used to boot this machine.
 */
bool is_boot_sector_patch_active() {
  far_ptr_t predicted_boot_sector = get_boot_sector_patch_ptr();

  return ((tsr_resident_t __far *)predicted_boot_sector.ptr)
    ->signature == TSR_SIGNATURE; 
}

/**
 * Determine if the TSR version of the application is loded.
 */
bool is_tsr_patch_active() {
  far_ptr_t predicted_boot_sector = get_boot_sector_patch_ptr();

  // If the interrupt vector points to the boot sector patch,
  // our tsr cannot be active.
  if (predicted_boot_sector.ptr == hdd_table_ptr->ptr) {
    return false;
  }

  printf("TSR Signature is %04X:%04X -> %08lX\n", 
    hdd_table_ptr->vector.segment,
    hdd_table_ptr->vector.offset,
    ((tsr_resident_t __far *)hdd_table_ptr->ptr)->signature);

  return ((tsr_resident_t __far *)hdd_table_ptr->ptr)
     ->signature == TSR_SIGNATURE;
}

/**
 *  Load the TSR into memory and replace the boot sector patch if it was
 *  previously loaded.
 */
void load_tsr(config_t *config, hdd_data_block_t *hdd_data) {
  far_ptr_t env_block;

  env_block.vector.segment = _psp;
  env_block.vector.offset = 0x002C;

  if (config->tsr_active) {
    printf("TSR is already loaded, nothing to do.\n");
    return;
  }    

  // Copy the data from tsr_in to the tsr_data structure.
  memcpy(
      &(tsr_data.hdd_data),
      hdd_data,
      sizeof(hdd_data_block_t));

  // Make sure we set the signature first.
  tsr_data.signature = TSR_SIGNATURE;

  // Setup the hdd data table pointer
  hdd_table_ptr->ptr = &(tsr_data);

  // Clear the MBR patch if it was active
  if (config->mbr_active) {
    printf("Unloading MBR patch.\n");

    // Give the claimed memory back
    *memory_size += 1;
  }

  // Free the environment block
  if (_dos_freemem(*((uint16_t __far *)env_block.ptr))) {
    printf("Error freeing environment block: %i\n", errno);
  }

  _dos_keep(0, tsr_data_paragraphs); 
}

