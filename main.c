/********************************************************************************************//**
 * @file main.c
 * 
 * A PDP-8 Simulator Front End
 ************************************************************************************************/

#include "pdp8sim.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const char* progName = "pdp8sim";

typedef enum BIN_State { Leader, OriginMSB, OriginLSB, DataMSB, DataLSB, Trailer } BIN_State;

/********************************************************************************************//**
 * Load a BIN file into memory
 ************************************************************************************************/
static bool load_BIN (const char* filename) {
	FILE* ifile = NULL;
	if ((ifile = fopen(filename, "r")) == NULL) { 
		fprintf(stderr, "%s: can't open '%s'!\n", progName, filename);
		return false;
	}

	const unsigned BIN_LEADER 		= 00200;
	const unsigned BIN_ORG_Mask		= 00100;
	const unsigned BIN_DATA_Mask	= 00077;
	const unsigned BIN_MSB_Shift	= 6;

	char		c;
	unsigned	data = 0;
	BIN_State	bstate = DataMSB;

	while ((c = getc(ifile)) != EOF) {
		const unsigned byte = c;
		if (byte == BIN_LEADER)
			continue;

		if ((byte & BIN_ORG_Mask) == BIN_ORG_Mask)
			bstate = OriginMSB;

		switch (bstate) {
			case Leader:
				assert(false);
				break;

			case OriginMSB:
				p.regs.pc = (byte & BIN_DATA_Mask) << BIN_MSB_Shift;
				bstate = OriginLSB;
				break;

			case OriginLSB:
				p.regs.pc |= byte;
				bstate = DataMSB;
				break;

			case DataMSB:
				data = (byte & BIN_DATA_Mask) << BIN_MSB_Shift;
				bstate = DataLSB;
				break;

			case DataLSB:
				data |= byte;
				p.mem[p.regs.pc++] = data;
				bstate = DataMSB;
				break;

			default:
				assert(false);
		}
	}
	fclose(ifile);

	return true;
}

/********************************************************************************************//**
 * Write an help diagnostic to standard error.
 ************************************************************************************************/
static void help() {
	fprintf(stderr, "Usage: %s [options... | filenames...]\n", progName);
	fprintf(stderr, "Where options is zero or more of:\n");
	fprintf(stderr, "-h|?     -- print this message, and return 1\n");
	fprintf(stderr, "-v       -- print the version, and return 1\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "And where filenames is zero or more program file names to load in BIN format\n");
}

/********************************************************************************************//**
 * The PDP8 simulator
 ************************************************************************************************/
int main (int argc, char** argv) {
	if (pdp8_init() != 0)
		return 1;

	// Scan the command line...
	for (int argn = 1; argn < argc; ++argn) {
		const char* arg = argv[argn];

		const size_t len = strlen(arg);

		if (len == 0)
			continue;

		if (strcmp(arg, "-") == 0)
			fprintf(stderr, "%s: unknown option '-'\n", progName);

		if (arg[0] == '-') {
			for (unsigned i = 0; i < len; ++i) {
				char c = arg[i];

				switch(c) {
					case '?': case 'h': help();			return 1;
					case 'v': printf("version 0.9\n");	return 1;
					default:
						fprintf(stderr, "%s: unknown option '%c'!\n", progName, c);
						return 1;
				}
			}

		} else if (!load_BIN(arg))			// Load a program file...
			return 1;
	}

	return process();						// Run the simulation...
}

