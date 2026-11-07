/********************************************************************************************//**
 * @file sys.c
 * 
 * pdp-8 SYStem device
 ************************************************************************************************/

#include "iot.h"

#include "pdp8sim.h"
#include "sys.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

/************************************************************************************************
 * System Device (SYS), device id 0)
 *
 * Note that ION is delayed by ION_DELAY instructions, including the ION instruction. This allows
 * a return from interrupt to work correctly, e.g.,
 *
 *     	ION		/ Enable interrupts AFTER the following instruction
 *		JMS I 0	/ Return from interrupt
 ************************************************************************************************/

// Initialize the SYS device
void sys_init(iot_dev* pdev) {
	assert(pdev != NULL);

	pdev->rdy = false;			// Unused by the SYS device
	pdev->rdy_delay_s = 0.0;
}

// Process an IOT instruction on the SYS device
void sys_iot(iot_dev* pdev, unsigned ops) {
	assert(pdev != NULL);

	if (ops == 1)				// ION
		p.ion_delay = ION_DELAY;

	else if (ops == 2) {		// ION
		p.ion_delay = 0;
		p.flags.int_en = false;

	} else {
		fprintf(stderr, "unknown iot system operation: %03o %03o!\n", pdev->id, ops);
		assert(false);
	}
}

// Make the SYS device ready
void sys_ready(iot_dev* pdev)	{	assert(pdev != NULL);	}

