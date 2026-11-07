/********************************************************************************************//**
 * @file tty.c
 * 
 * pdp-8 TTY device
 ************************************************************************************************/

#include "tty.h"

#include "pdp8sim.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#define	TTY_CHAR_RATE	10					/* in characters/second		*/
#define	TTY_RDY_DELAY	(1.0/TTY_CHAR_RATE)	/* Delay in seconds			*/

/************************************************************************************************
 * TTY Keyboard/Paper-Tape Reader, device id 3.
 ************************************************************************************************/

// Initialize the TTY Keyboard
void tty_kb_init(iot_dev* pdev) {
	assert(pdev != NULL);

	pdev->rdy = false;

	/*
	 * The keyboard starts life ready-pending becuase it only needs the arrival
	 * of a new character to become ready. Thus ready-delay is one TTY delay in
	 * the past.
	 */

	pdev->rdy_pend = true;
	pdev->rdy_delay_s = p.runtime_s - TTY_RDY_DELAY;
}

// Process a IOT instruction for the TTY Keyboard
void tty_kb_iot(iot_dev* pdev, unsigned ops) {
	assert(pdev != NULL);

	if (iop1(ops))			//	KSF
		++p.regs.pc;

	if (iop2(ops)) 			//	KCC
		pdev->rdy = false;

	if (iop4(ops)) {		//	KRS
		int c = 0;
		if (read(STDOUT_FILENO, &c, 1) == -1)
			perror("read()");
		if (c == EOF)
			c = 0;
		else
			p.regs.ac = c | MPARITY;

		// Start the TTY rate/delay timer...

		pdev->rdy_pend = true;
		pdev->rdy_delay_s = p.runtime_s;
	}
}

// Make the TTY Keyboard ready
void tty_kb_ready(iot_dev* pdev) {
	assert(pdev != NULL);

	if (pdev->rdy_pend) {
		if (p.runtime_s - pdev->rdy_delay_s > TTY_RDY_DELAY) {
			pdev->rdy_pend = false;
			pdev->rdy = true;
		}
	}
}

/************************************************************************************************
 * TTY Printer/Paper-Tape Punch, device id #4.
 ************************************************************************************************/

// Initialize the TTY Printer/Paper-Tape Punch
void tty_pr_init(iot_dev* pdev) {
	assert(pdev != NULL);

	pdev->rdy = false;

	/*
	 * The printer first becomes ready once the first character is printed. Thus
	 * ready-pending is false.
     */
	
	pdev->rdy_pend = false;
	pdev->rdy_delay_s = p.runtime_s;
}

// Process a IOT instruction for the TTY Printer/Paper-Tape Punch
void tty_pr_iot(iot_dev* pdev, unsigned ops) {
	assert(pdev != NULL);

	if (iop1(ops) && pdev->rdy)	// TSF
		++p.regs.pc;

	if (iop2(ops)) 				//	TCF
		pdev->rdy = false;
			
	if (iop4(ops)) {			// 	TPC
		int c = p.regs.ac & MPARITY_MASK;
		if (write(STDOUT_FILENO, &c, 1) == -1)
			perror("write()");

		// Start the TTY rate/delay timer...

		pdev->rdy_pend = true;
		pdev->rdy_delay_s = p.runtime_s;
	}
}

// Make the TTY Printer/Paper-Tape Punch Ready.
void tty_pr_ready(iot_dev* pdev) {
	assert(pdev != NULL);

	if (pdev->rdy_pend) {
		if (p.runtime_s - pdev->rdy_delay_s > TTY_RDY_DELAY) {
			pdev->rdy_pend = false;
			pdev->rdy = true;
		}
	}
}

