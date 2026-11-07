/********************************************************************************************//**
 * @file iot.c
 * 
 * pdp-8 IOT instructions
 ************************************************************************************************/

#include "iot.h"

#include "sys.h"
#include "tty.h"
#include "undef.h"

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

/************************************************************************************************
 * Utilities
 ************************************************************************************************/

// Return true if iop1 is set in ops.
bool iop1(unsigned ops)	{	return ops & 1;	}

// Return true if iop2 is set in ops.
bool iop2(unsigned ops)	{	return ops & 2; }

// Return true if iop3 is set in ops.
bool iop4(unsigned ops)	{	return ops & 4; }

/********************************************************************************************//**
 * Initialize the IOT subsystem
 ************************************************************************************************/
void iot_init(void) {
	// SYS device
	dev_tbl[0].id		= 0;
	dev_tbl[0].init 	= sys_init;
	dev_tbl[0].iot		= sys_iot;
	dev_tbl[0].mk_rdy 	= sys_ready;

	// Highspeed Perforated Tape Reader PR8/I
	dev_tbl[1].id		= 1;
	dev_tbl[1].init		= undef_init;
	dev_tbl[1].iot		= undef_iot;
	dev_tbl[1].mk_rdy	= undef_mk_rdy;

	// Highspeed Perforated Tape Punch  PR8/I
	dev_tbl[2].id		= 2;
	dev_tbl[2].init		= undef_init;
	dev_tbl[2].iot		= undef_iot;
	dev_tbl[2].mk_rdy	= undef_mk_rdy;

	// TTY keyboard
	dev_tbl[3].id		= 3;
	dev_tbl[3].init		= tty_kb_init;
	dev_tbl[3].iot		= tty_kb_iot;
	dev_tbl[3].mk_rdy	= tty_kb_ready;

	// TTY printer
	dev_tbl[4].id		= 4;
	dev_tbl[4].init		= tty_pr_init;
	dev_tbl[4].iot		= tty_pr_iot;
	dev_tbl[4].mk_rdy	= tty_pr_ready;

	// The remainder are undefined
	for (unsigned id = 5; id < IOT_DEV_CNT; ++id) {

		iot_dev* pdev = &dev_tbl[id];
		pdev->id		= id;
		pdev->init		= undef_init;
		pdev->iot		= undef_iot;
		pdev->mk_rdy	= undef_mk_rdy;
	}

	// Now, initialize all of the devices
	for (iot_dev* pdev = dev_tbl; pdev < &dev_tbl[IOT_DEV_CNT]; ++pdev)
		pdev->init(pdev);
}

/********************************************************************************************//**
 * Simulate an IOT instruction.
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

	// @tbd - add some common IOT instructions such as KSF, TLS...

	printf("IOT %03o %03o", dev, ops);
}

/********************************************************************************************//**
 * Bring the IO device ready flags up to date
 ************************************************************************************************/
void iot_update() {
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
	}
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

