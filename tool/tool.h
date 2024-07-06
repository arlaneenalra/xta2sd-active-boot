#ifndef TOOL_H
#define TOOL_H

#include <unistd.h>
#include <stdint.h>


int usage();
int parse_arguments(int argc, char **argv);


/**
 * Allows accessing the segment and offset addresses of an interrupt vector.
 * They are reall just far pointers in the end.
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
	uint8_t	max_ecc_burst;
	uint8_t control_byte;
	uint8_t timeout;
	uint8_t format_timeout;
	uint8_t check_timeout;
	uint16_t landing_zone_cylinder;
	uint8_t sectors_per_track;  // On the Tandy 1110HD this is hard set to 17 and the value is always 0.
	uint8_t reserved;
} hdd_data_block_t;


#endif
