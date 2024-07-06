#include <stdio.h>
#include <unistd.h>
#include <stdint.h>


extern unsigned int boot_boot_bin_len;

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

/**
 * Pointer to the interrupt vector table. We need this
 * to see if we're using the active boot sector code
 * right now.
 */
uint16_t __far *memory_size = (uint16_t __far *)0x00000413;
far_ptr_t __far *hdd_table_ptr = (far_ptr_t *)0x00000104;


/**
 * Output a usage statment and exit.
 */
int usage() {
	printf("Active boot sector patching tool.\n");
	printf("Usage:\n");
	printf("  tool [/f]\n");
	printf("\n");
	printf("  /f Force the issue.");
	printf("\n");

	return 1;
}


int parse_arguments(int argc, char **argv) {
	int parsed = 0;
	int c;

	// See: https://open-watcom.github.io/open-watcom-v2-wikidocs/clib.html#getopt
	while((c = getopt(argc, argv, "f")) != -1) {
		parsed++;
		switch(c) {
			case 'f':
				printf("Forcing the issue.\n");
				break;
			case '?':
				printf("Unknown argument '%c'\n", optopt);
				return 1; 
		}
	}

	// We have to have at least one argument.	
	if (parsed == 0) {
		return 1; 
	}

	return 0;
}
 
int main(int argc, char **argv) {
		
	if (!parse_arguments(argc, argv)) {
		return usage();	
	}

	printf("Current hdd_table located at %4x:%4x -> %8lx\n",
			hdd_table_ptr->vector.segment,
			hdd_table_ptr->vector.offset,
			hdd_table_ptr->ptr);

	printf("Bios is currently reporting %uK conventional memory free.\n",
			*memory_size);

  return 0;
}
