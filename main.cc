/********************************************************************************************//**
 * @file main.cc
 * 
 * A PDP-8 Simulator Front End
 ************************************************************************************************/

#include "pdp8sim.h"

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>

using namespace std;

const char* progName = "pdp8sim";

/********************************************************************************************//**
 * Load a BIN file into memory
 ************************************************************************************************/
static bool load_BIN (const string& filename) {
	enum class BIN_State { Leader, OriginMSB, OriginLSB, DataMSB, DataLSB, Trailer };

	ifstream ifs{filename};
	if (!ifs) {
		cerr << progName << ": can't open '" << filename << "'!\n";
		return false;
	}

	const unsigned BIN_LEADER 		= 00200;
	const unsigned BIN_ORG_Mask		= 00100;
	const unsigned BIN_DATA_Mask	= 00077;
	const unsigned BIN_MSB_Shift	= 6;

	unsigned	data = 0;
	uint8_t		byte;
	char		c;
	BIN_State	bstate = BIN_State::DataMSB;

	while (ifs.get(c)) {
		byte = static_cast<uint8_t>(c);

		if (byte == BIN_LEADER)
			continue;

		if ((byte & BIN_ORG_Mask) == BIN_ORG_Mask)
			bstate = BIN_State::OriginMSB;

		switch (bstate) {
			case BIN_State::Leader:
				assert(false);
				break;

			case BIN_State::OriginMSB:
				p.regs.pc = (byte & BIN_DATA_Mask) << BIN_MSB_Shift;
				bstate = BIN_State::OriginLSB;
				break;

			case BIN_State::OriginLSB:
				p.regs.pc |= byte;
				bstate = BIN_State::DataMSB;
				break;

			case BIN_State::DataMSB:
				data = (byte & BIN_DATA_Mask) << BIN_MSB_Shift;
				bstate = BIN_State::DataLSB;
				break;

			case BIN_State::DataLSB:
				data |= byte;
				p.mem[p.regs.pc++] = data;
				bstate = BIN_State::DataMSB;
				break;

			default:
				assert(false);
		}
	}

	return true;
}

/********************************************************************************************//**
 * Write an help diagnostic to standard error.
 ************************************************************************************************/
static void help() {
	cerr	<< "Usage: " << progName << " [options... | filenames...]\n"
			<< "Where options is zero or more of:\n"
			<< "-h|?     -- print this message, and return 1\n"
			<< "-v       -- print the version, and return 1\n"
			<< '\n'
			<< "And where filenames is zero or more program file names to load in BIN format\n";
}

/********************************************************************************************//**
 * The PDP8 simulator
 ************************************************************************************************/
int main (int argc, char** argv) {
	if (pdp8_init() != 0)
		return 1;

	// Scan the command line...
	for (int argn = 1; argn < argc; ++argn) {
		const string arg = argv[argn];

		if (arg == "")
			continue;

		if (arg == "-")
			cerr << progName << ": unknown option '-',\n";

		if (arg[0] == '-') {
			for (auto i = arg.begin() + 1; i != arg.end(); ++i) {
				char c = *i;

				switch(c) {
					case '?': case 'h': help();			return 1;
					case 'v': cout << "version 0.9\n";	return 1;
					default:
						cerr << progName << ": unknown option '" << c << "'.\n";
						return 1;
				}
			}

		} else if (!load_BIN(arg))			// Load a program file...
			return 1;
	}

	return process();						// Run the simulation...
}

