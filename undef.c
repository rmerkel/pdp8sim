/********************************************************************************************//**
 * @file undef.c
 * 
 * Undefined pdp-8 device
 ************************************************************************************************/

#include "undef.h"

#include "pdp8sim.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

// Unsupported Device init
void undef_init(iot_dev* pdev)	{
	assert(pdev != NULL);

	pdev->rdy = false;				// Unused
	pdev->rdy_pend = false;			// Unused
}

// Unsupported Device iot
void undef_iot(iot_dev* pdev, unsigned ops) {
	assert(pdev != NULL);

#ifndef	NDEBUG
	fprintf(stderr, "%s: IOT on unsupported device: %03o %03o!\n", progName, pdev->id, ops);
#endif
}

// Unsupported Device make ready
void undef_mk_rdy(iot_dev* pdev) {
	assert(pdev != NULL);

#ifndef	NDEBUG
	fprintf(stderr, "%s: setting an unsupported device ready: %03o!\n", progName, pdev->id);
#endif
}

