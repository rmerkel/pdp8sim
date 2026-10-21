/********************************************************************************************//**
 * @file iot.cc
 * 
 * PDP-8 IOT instructions
 ************************************************************************************************/

#include "iot.h"
#include "pdp8sim.h"

#include <cassert>
#include <csignal>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sys/select.h>
#include <unistd.h>

using namespace std;

fd_set	read_fds;						// Read device ready flags
fd_set	write_fds;						// Write device ready flags

/********************************************************************************************//**
 * Simulate an IOT instruction.
 ************************************************************************************************/
void iot() {
	const	unsigned 	dev	= (p.regs.md & IOT_DEV_SEL) >> IOT_DEV_SHIFT;
	const	unsigned	ops	= p.regs.md & IOT_OP;
			bool		rdy	= false;	// Capture before IOP2 clears the device flag.

	switch (dev) {
	case 0:							// System
		if (ops == 1)				// ION, unmask interrupts, with delay so that ION takes effet at the end of the following instuction
			cerr << "ION is not implemented, ignored." << endl;
		else if (ops == 2)			// IOF, mask interrupts
			cerr << "IOF is not implemented, ignoredj." << endl;
		else {
			cerr << "Unknown IOT System operation: " << ops << endl;
			assert(false);
		}
		break;

	case 1:							// Highspeed Perforated Tape Reader PR8/I
		cerr << "RSF/RRB/RFC is not implemented, ignored." << endl;
		break;

	case 2:							// Highspeed Perforated Tape Punch  PR8/I
		cerr << "PSF/PCF/PPC/PLS not implemented, ignored." << endl;

	case 3: 						// Teletype Keyboard Reader
		rdy = FD_ISSET(STDIN_FILENO, &read_fds);
		if (ops & 1) {				//	KSF
			if (rdy)
				++p.regs.ac;
		}

		if (ops & 2)				//	KCC
			FD_CLR(STDIN_FILENO, &read_fds);

		if (ops & 4) {				//	KRS
			int c;
			read(STDIN_FILENO, &c, 1);
			p.regs.ac = c;
		}
		break;

	case 4:							// Teletype Teleprinter/Punch
		rdy = FD_ISSET(STDOUT_FILENO, &write_fds);
		if (ops & 1) {				//	TSF
			if (rdy)
				++p.regs.pc;
		}

		if (ops & 2)				//	TCF
			FD_CLR(STDOUT_FILENO, &write_fds);
			
		if (ops & 4) {				// 	TPC
			if (p.regs.ac !=0) {
				int c = p.regs.ac & 0177;
				write(STDOUT_FILENO, &c, 1);
			}
		}
		break;

	default: 						// Not implemented/supported!
		cerr << "Unknown or unsupported IOT device: " << dev << endl;
		assert(false);
	}

	p.runtime += 4.5 - 1.5;			// Adjust for IOT
}

/********************************************************************************************//**
 * Disassemble instr, assuming its a valid IOT instruction.
 ************************************************************************************************/
void iot_disasm(unsigned instr) {
	cout	<< "IOT ";
	const unsigned dev	= (instr & IOT_DEV_SEL) >> IOT_DEV_SHIFT;
	const unsigned ops	= instr & IOT_OP;
	cout << setw(3)	<< dev  << ' '
		 << setw(1) << ops;
}

/********************************************************************************************//**
 * Bring the IO device ready flags up to date
 ************************************************************************************************/
void iot_update_devs() {
	timeval		timeout = { 0, 0};

	FD_ZERO(&read_fds);
	FD_SET(STDIN_FILENO, &read_fds);

	FD_ZERO(&write_fds);
	FD_SET(STDOUT_FILENO, &write_fds);

	/*
	 * Select can intercept Ctrl-C (SIGINT) if it is the very first character,
	 * so treat select() errors as HALT.
	 */

	if (select(STDOUT_FILENO+1, &read_fds, &write_fds, nullptr, &timeout) < 0)
		signal_handler(SIGINT); 
	else
		; // set interrupt pending here.
}

