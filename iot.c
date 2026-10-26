/********************************************************************************************//**
 * @file iot.c
 * 
 * PDP-8 IOT instructions
 ************************************************************************************************/

#include "iot.h"
#include "pdp8sim.h"

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>

fd_set	read_fds;						// Read device ready flags
fd_set	write_fds;						// Write device ready flags

/********************************************************************************************//**
 * Simulate an IOT instruction.
 *
 * Note that ION is delayed by ION_DELAY instructions, including the ION instruction. This allows
 * a return from interrupt to work correctly, e.g.,
 *
 *     	ION		/ Enable interrupts AFTER the following instruction
 *		JMS I 0	/ Return from interrupt
 ************************************************************************************************/
void iot() {
	const	unsigned 	dev	= (p.regs.md & IOT_DEV_SEL) >> IOT_DEV_SHIFT;
	const	unsigned	ops	= p.regs.md & IOT_OP;
			bool		rdy	= false;	// Capture before IOP2 clears the device flag.

	switch (dev) {
	case 0:								// System
		if (ops == 1) {					// ION, enable interrupts
			if (!p.flags.int_en)
				p.ion_delay = ION_DELAY;
		} else if (ops == 2) {			// IOF, disable interrupts
			p.ion_delay = 0;
			p.flags.int_en = false;
		} else {
			fprintf(stderr, "Unknown IOT System operation: %03o!\n", ops);
			assert(false);
		}
		break;

	case 1:							// Highspeed Perforated Tape Reader PR8/I
		fprintf(stderr, "RSF/RRB/RFC s not implemented, ignored.\n");
		break;

	case 2:							// Highspeed Perforated Tape Punch  PR8/I
		fprintf(stderr,  "PSF/PCF/PPC/PLS is not implemented, ignored.\n");

	case 3: 						// Teletype Keyboard Reader
		rdy = FD_ISSET(STDIN_FILENO, &read_fds);
		if (ops & 1) {				//	KSF
			if (rdy)
				++p.regs.pc;
		}

		if (ops & 2)				//	KCC
			FD_CLR(STDIN_FILENO, &read_fds);

		if (ops & 4) {				//	KRS
			int c = 0;
			int n = read(STDIN_FILENO, &c, 1);
			if (n < 0) {
				fprintf(stderr, "%s: read() error\n", progName);
				c = 0;
			}
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
		fprintf(stderr, "Unknown or unsupported IOT device: %0o.\n", dev);
		assert(false);
	}

	p.runtime += 4.5 - 1.5;			// Adjust for IOT
}

/********************************************************************************************//**
 * Disassemble instr, assuming its a valid IOT instruction.
 ************************************************************************************************/
void iot_disasm(unsigned instr) {
	const unsigned dev	= (instr & IOT_DEV_SEL) >> IOT_DEV_SHIFT;
	const unsigned ops	= instr & IOT_OP;
	printf("IOT %0o %0o", dev, ops);
}

/********************************************************************************************//**
 * Bring the IO device ready flags up to date
 ************************************************************************************************/
void iot_update_devs() {
	struct timeval timeout = { 0, 0};

	FD_ZERO(&read_fds);
	FD_SET(STDIN_FILENO, &read_fds);

	FD_ZERO(&write_fds);
	FD_SET(STDOUT_FILENO, &write_fds);

	if (FD_ISSET(STDIN_FILENO, &read_fds) || FD_ISSET(STDOUT_FILENO, &write_fds))
		p.flags.int_pend = true;

	/*
	 * Select can intercept Ctrl-C (SIGINT) if it is the very first character,
	 * so treat select() errors as HALT.
	 */

	if (select(STDOUT_FILENO+1, &read_fds, &write_fds, NULL, &timeout) < 0)
		signal_handler(SIGINT); 
	else
		; // set interrupt pending here.
}

