/********************************************************************************************//**
 * @file iot.c
 * 
 * pdp-8 IOT instructions
 ************************************************************************************************/

#include "iot.h"

#include "sys.h"
#include "tty.h"

#include "pdp8sim.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>

#define        IOT_DEV_CNT     64              	/* # of Devices (2**6) */

// Device table
static iot_dev dev_tbl[IOT_DEV_CNT];			// Device Table

static fd_set  read_fds;                       	// Read device ready flags
static fd_set  write_fds;                      	// Write device ready flags

/********************************************************************************************//**
 * Utilities
 ************************************************************************************************/

// iops(ops)?
bool iop1(unsigned ops)	{	return ops & 1;	}

// iop2(ops)?
bool iop2(unsigned ops)	{	return ops & 2; }

// iop3?
bool iop4(unsigned ops)	{	return ops & 4; }

/********************************************************************************************//**
 * Unsupported Device(s)
 ************************************************************************************************/

// Unsupported Device init
static void unsupported_init(iot_dev* pdev)	{
	assert(pdev != NULL);

	pdev->rdy = false;				// Unused
	pdev->rdy_pend = false;			// Unused
}

// Unsupported Device iot
static void unsupported_iot(iot_dev* pdev, unsigned ops) {
	assert(pdev != NULL);

#ifndef	NDEBUG
	fprintf(stderr, "%s: IOT on unsupported device: %03o %03o!\n", progName, pdev->id, ops);
#endif
}

// Unsupported Device update
static void unsupported_update(iot_dev* pdev)	{	assert(pdev != NULL);	}

// Unsupported Device make ready
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
	p.runtime_s += 4.5e-6 - 1.5e-6;			// Adjust for IOT
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

	// maybe use selectp to deal with SIGINT?

	const int r = select(STDOUT_FILENO+1, &read_fds, &write_fds, NULL, &timeout);
	if (r > 0) {
		if (FD_ISSET(STDIN_FILENO, &read_fds)) {
			iot_dev* pdev = &dev_tbl[3];
			pdev->mk_rdy(pdev);
		}

		if (FD_ISSET(STDOUT_FILENO, &write_fds)) {
			iot_dev* pdev = &dev_tbl[4];
			pdev->mk_rdy(pdev);
		}

		// Other devices here...
	}

	for (iot_dev* pdev = dev_tbl; pdev < &dev_tbl[IOT_DEV_CNT]; ++pdev)
		pdev->update(pdev);
}

/********************************************************************************************//**
 * Initialize the IOT subsystem
 ************************************************************************************************/
void iot_init(void) {
	// SYS device
	dev_tbl[0].id		= 0;
	dev_tbl[0].init 	= sys_init;
	dev_tbl[0].iot		= sys_iot;
	dev_tbl[0].update	= sys_update;
	dev_tbl[0].mk_rdy 	= sys_ready;

	// Highspeed Perforated Tape Reader PR8/I
	dev_tbl[1].id		= 1;
	dev_tbl[1].init		= unsupported_init;
	dev_tbl[1].iot		= unsupported_iot;
	dev_tbl[1].update	= unsupported_update;
	dev_tbl[1].mk_rdy	= unsupported_update;

	// Highspeed Perforated Tape Punch  PR8/I
	dev_tbl[2].id		= 2;
	dev_tbl[2].init		= unsupported_init;
	dev_tbl[2].iot		= unsupported_iot;
	dev_tbl[2].update	= unsupported_update;
	dev_tbl[2].mk_rdy	= unsupported_ready;

	// TTY keyboard
	dev_tbl[3].id		= 3;
	dev_tbl[3].init		= tty_kb_init;
	dev_tbl[3].iot		= tty_kb_iot;
	dev_tbl[3].update	= tty_kb_update;
	dev_tbl[3].mk_rdy	= tty_kb_ready;

	// TTY printer
	dev_tbl[4].id		= 4;
	dev_tbl[4].init		= tty_pr_init;
	dev_tbl[4].iot		= tty_pr_iot;
	dev_tbl[4].update	= tty_pr_update;
	dev_tbl[4].mk_rdy	= tty_pr_ready;

	// The remainder are undefined
	for (unsigned id = 5; id < IOT_DEV_CNT; ++id) {

		iot_dev* pdev = &dev_tbl[id];
		pdev->id		= id;
		pdev->init		= unsupported_init;
		pdev->iot		= unsupported_iot;
		pdev->update	= unsupported_update;
		pdev->mk_rdy	= unsupported_update;
	}

	// Now, initialize all of the devices
	for (iot_dev* pdev = dev_tbl; pdev < &dev_tbl[IOT_DEV_CNT]; ++pdev)
		pdev->init(pdev);
}

/********************************************************************************************//**
 * Check for pending interrupts
 ************************************************************************************************/
bool iot_int_pending(void) {
	for (iot_dev* pdev = dev_tbl; pdev < &dev_tbl[IOT_DEV_CNT]; ++pdev)
		if (pdev->rdy)
			return true;

	return false;
}

