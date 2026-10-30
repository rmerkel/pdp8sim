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

#define	MPARITY			0200		// Set bit 7 for Mark parity
#define	MPARITY_MASK	0177		// Clear bit 7 to clear Mark parity

fd_set	read_fds;					// Read device ready flags
fd_set	write_fds;					// Write device ready flags

#if 	1
bool	ready[64] = { false };		// Device ready flags
#endif

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
			bool*		pRdy = &ready[dev];

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
		if (ops & 1) {				//	KSF
			if (*pRdy)
				++p.regs.pc;
		}

		if (ops & 2)				//	KCC
			*pRdy = false;

		if (ops & 4) {				//	KRS
			int c = 0;
			int n = read(STDIN_FILENO, &c, 1);
			if (n < 0) {
				fprintf(stderr, "%s: read() error\n", progName);
				c = 0;
			}
			p.regs.ac = c | MPARITY;
		}
		break;

	case 4:							// Teletype Teleprinter/Punch
		if (ops & 1) {				//	TSF
			if (*pRdy)
				++p.regs.pc;
		}

		if (ops & 2)				//	TCF
			*pRdy = false;
			
		if (ops & 4) {				// 	TPC
			if (p.regs.ac !=0) {
				int c = p.regs.ac & MPARITY_MASK;
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

#if 1
	if (FD_ISSET(STDIN_FILENO, &read_fds))
		ready[3] = true;
	if (FD_ISSET(STDOUT_FILENO, &write_fds))
		ready[4] = true;
#endif

	/*
	 * Select can intercept Ctrl-C (SIGINT) if it is the very first character,
	 * so treat select() errors as HALT.
	 */

	const int r = select(STDOUT_FILENO+1, &read_fds, &write_fds, NULL, &timeout);
	if (r > 0)
		p.flags.int_pend = true;

#if 1
	fprintf(stderr, "%d, %d: %b, %b\n", ready[3], ready[4], p.flags.int_en, p.flags.int_pend);
#endif
}

