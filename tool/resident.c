#include "tool.h"

/**
 * Setup a resident data block for our TSR version.
 * See: https://forum.vcfed.org/index.php?threads/tsr-programs-with-open-watcom.23586/ for details.
 */
//#pragma data_seg("BEGTEXT", "CODE")
//#pragma code_seg("BEGTEXT", "CODE")
#pragma data_seg("ResidentData", "RDATA")
#pragma code_seg("ResidentCode", "RDATA")

/**
 * The only part of the TSR that need to stay in memory.
 */
tsr_resident_t tsr_data;

/**
 * Figure out how many 'paragraphs' we our data takes up.
 * We add 0x100 to account for the PSP and 15 to avoid a parital paragraph.
 * 
 * Must be the last entry in this file.
 */
uint16_t tsr_data_paragraphs;


