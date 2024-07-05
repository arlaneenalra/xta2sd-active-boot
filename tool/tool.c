#include <stdio.h>
#include <unistd.h>

extern unsigned int boot_boot_bin_len;

int usage();
int parse_arguments(int argc, char **argv);

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
				return usage();
		}
	}

	if (parsed == 0) {
		return usage();
	}

	return 0;
}
 
int main(int argc, char **argv) {
		
	parse_arguments(argc, argv);

  return 0;
}
