/********************************************************************************************//**
 * @file iot.c
 * 
 * pdp-8 iot instructions
 ************************************************************************************************/

#include "iot.h"
#include "pdp8sim.h"

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>

#define	MPARITY			0200		// Set bit 7 for mark parity
#define	MPARITY_MASK	0177		// Clear bit 7 to clear mark parity

static fd_set	read_fds;			// Read device ready flags
static fd_set	write_fds;			// Write device ready flags

/********************************************************************************************//**
 * An IOT Device
 ************************************************************************************************/
typedef struct iot_dev {			// IO Device interface
	unsigned	id;					//	Device ID
	bool		rdy;				//	Device ready flag
									//	Initialize the device
	void		(*init)(struct iot_dev* pdev);
									//	Interact with the device
	void		(*iot)(struct iot_dev* pdev, unsigned ops);
									//	Update the device
	void		(*update)(struct iot_dev* pdev);
									//	Set the device ready
	void		(*ready)(struct iot_dev* pdev);

} iot_dev;

#define	IOT_DEV_CNT	64				/* # of Devices (2**6) */

// Device table
static iot_dev dev_tbl[IOT_DEV_CNT];

/********************************************************************************************//**
 * System Device (SYS, device id 0)
 ************************************************************************************************/

// init
static void sys_init(iot_dev* pdev) {
	assert(pdev != NULL);

	pdev->rdy = false;			// Unused by the SYS device
}

// iot
static void sys_iot(iot_dev* pdev, unsigned ops) {
	assert(pdev != NULL);

	const bool iop1 = ops & 1;
	const bool iop2 = ops & 2;
	const bool iop4 = ops & 4;

	if (iop1)					// ION
		p.ion_delay = ION_DELAY;

	if (iop2) {					// IOF
		p.ion_delay = 0;
		p.flags.int_en = false;
	}

	if (iop4 || (iop1 && iop2)) {
		fprintf(stderr, "unknown iot system operation: %03o %03o!\n", pdev->id, ops);
		assert(false);
	}
}

// update
static void sys_update(iot_dev* pdev) {	assert(pdev != NULL);	}

// make ready
static void sys_ready(iot_dev* pdev) {	assert(pdev != NULL);	}

/********************************************************************************************//**
 * TTY Keyboard/paper-tape punch device
 ************************************************************************************************/

// init
static void tty_kb_init(iot_dev* pdev) {
	assert(pdev != NULL);

	pdev->rdy = false;
}

// iot
static void tty_kb_iot(iot_dev* pdev, unsigned ops) {
	assert(pdev != NULL);

	const bool iop1 = ops & 1;
	const bool iop2 = ops & 2;
	const bool iop4 = ops & 4;

	if (iop1) {				//	KSF
		if (pdev->rdy)
			++p.regs.pc;
	}

	if (iop2)				//	KCC
		pdev->rdy = false;

	if (iop4) {				//	KRS
		int c = 0;
		int n = read(STDIN_FILENO, &c, 1);
		if (n < 0) {		// Some other error
			const int error = errno;
			fprintf(stderr, "%s: read() error: %d, %03o %03o\n", progName, error, pdev->id, ops);
			c = 0;
		}
		p.regs.ac = c | MPARITY;
	}
}

// update
static void tty_kb_update(iot_dev* pdev) {
	assert(pdev != NULL);
}

// make ready
static void tty_kb_ready(iot_dev* pdev) {
	assert(pdev != NULL);

	p.flags.int_pend = pdev->rdy = true;
}

/********************************************************************************************//**
 * TTY Printer/Punch Device
 ************************************************************************************************/

// init
static void tty_pr_init(iot_dev* pdev) {
	assert(pdev != NULL);

	pdev->rdy = false;
}

// iot
static void tty_pr_iot(iot_dev* pdev, unsigned ops) {
	assert(pdev != NULL);

	const bool iop1 = ops & 1;
	const bool iop2 = ops & 2;
	const bool iop4 = ops & 4;

	if (iop1 && pdev->rdy)		// TSF
		++p.regs.pc;

	if (iop2)					//	TCF
		pdev->rdy = false;
			
	if (iop4) {					// 	TPC
		if (p.regs.ac !=0) {
			int c = p.regs.ac & MPARITY_MASK;
			write(STDOUT_FILENO, &c, 1);
		}
	}
}

// update
static void tty_pr_update(iot_dev* pdev) {
	assert(pdev != NULL);
}

// make ready
static void tty_pr_ready(iot_dev* pdev) {
	p.flags.int_pend = pdev->rdy = true;

	assert(pdev != NULL);
}

/********************************************************************************************//**
 * Unsupported Device
 ************************************************************************************************/

// init
static void unsupported_init(iot_dev* pdev)	{
	assert(pdev != NULL);

	pdev->rdy = false;				// Unused
}

// iot
static void unsupported_iot(iot_dev* pdev, unsigned ops) {
	assert(pdev != NULL);

#ifndef	NDEBUG
	fprintf(stderr, "%s: IOT on unsupported device: %03o %03o!\n", progName, pdev->id, ops);
#endif
}

// update
static void unsupported_update(iot_dev* pdev)	{	assert(pdev != NULL);	}

// make ready
static void unsupported_ready(iot_dev* pdev) {
	assert(pdev != NULL);

#ifndef	NDEBUG
	fprintf(stderr, "%s: setting an unsupported device ready: %03o!\n", progName, pdev->id);
#endif
}

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

	assert(dev < IOT_DEV_CNT);

	iot_dev* pdev = dev_tbl + dev;
	pdev->iot(pdev, ops);
	p.runtime += 4.5 - 1.5;			// Adjust for IOT
}

/********************************************************************************************//**
 * Disassemble instr, assuming its a valid IOT instruction.
 ************************************************************************************************/
void iot_disasm(unsigned instr) {
	const unsigned dev	= (instr & IOT_DEV_SEL) >> IOT_DEV_SHIFT;
	const unsigned ops	= instr & IOT_OP;
	printf("IOT %03o %03o", dev, ops);
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

	p.flags.int_pend = false;

	/*
	 * Select can intercept Ctrl-C (SIGINT) if it is the very first character,
	 * so treat select() errors as HALT.
	 */

	const int r = select(STDOUT_FILENO+1, &read_fds, &write_fds, NULL, &timeout);
	if (r > 0) {
#if 1	// Just support the keyboard for now...
		if (FD_ISSET(STDIN_FILENO, &read_fds)) {
			dev_tbl[3].ready(&dev_tbl[3]);

#	if 0
			p.flags.run = false;
			fprintf(stderr, "%b, %b: %b\n", dev_tbl[3].rdy, p.flags.int_en, p.flags.int_pend);
#	endif
		}
#else
		// @tbd Possible to have a fds bit translate to IO device ID here.

		if (FD_ISSET(STDIN_FILENO, &read_fds))
			dev_tbl[&dev_tbl[3]].ready();

		if (FD_ISSET(STDOUT_FILENO, &write_fds))
			dev_tbl[&dev_tbl[4]].ready();
#endif
	}

	for (iot_dev* pdev = dev_tbl; pdev < &dev_tbl[IOT_DEV_CNT]; ++pdev)
		pdev->update(pdev);
}

/********************************************************************************************//**
 * Initialize the IOT subsystem
 ************************************************************************************************/
void iot_init(void) {
	// SYS device
	dev_tbl[0].init 	= sys_init;
	dev_tbl[0].iot		= sys_iot;
	dev_tbl[0].update	= sys_update;
	dev_tbl[0].ready 	= sys_ready;

	// Highspeed Perforated Tape Reader PR8/I
	dev_tbl[1].init		= unsupported_init;
	dev_tbl[1].iot		= unsupported_iot;
	dev_tbl[1].update	= unsupported_update;
	dev_tbl[1].ready	= unsupported_update;

	// Highspeed Perforated Tape Punch  PR8/I
	dev_tbl[2].init		= unsupported_init;
	dev_tbl[2].iot		= unsupported_iot;
	dev_tbl[2].update	= unsupported_update;
	dev_tbl[2].ready	= unsupported_ready;

	// TTY keyboard
	dev_tbl[3].init		= tty_kb_init;
	dev_tbl[3].iot		= tty_kb_iot;
	dev_tbl[3].update	= tty_kb_update;
	dev_tbl[3].ready	= tty_kb_ready;

	// TTY printer
	dev_tbl[4].init		= tty_pr_init;
	dev_tbl[4].iot		= tty_pr_iot;
	dev_tbl[4].update	= tty_pr_update;
	dev_tbl[4].ready	= tty_pr_ready;

	// The remainder are undefined
	for (iot_dev* pdev = &dev_tbl[5]; pdev < &dev_tbl[IOT_DEV_CNT]; ++pdev) {
		pdev->init		= unsupported_init;
		pdev->iot		= unsupported_iot;
		pdev->update	= unsupported_update;
		pdev->ready		= unsupported_update;
	}

	// Now, initialize all of the devices
	for (iot_dev* pdev = dev_tbl; pdev < &dev_tbl[IOT_DEV_CNT]; ++pdev)
		pdev->init(pdev);
}

