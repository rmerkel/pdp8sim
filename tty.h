/********************************************************************************************//**
 * @file tty.h
 * 
 * pdp-8 TTY device
 ************************************************************************************************/

#ifndef	TTY_H
#define	TTY_H

#include "iot.h"

#define	MPARITY			0200		// Set bit 7 for mark parity
#define	MPARITY_MASK	0177		// Clear bit 7 to clear mark parity

/********************************************************************************************//**
 * TTY Keyboard/Paper-Reader Device
 ************************************************************************************************/
void tty_kb_init(iot_dev* pdev);
void tty_kb_iot(iot_dev* pdev, unsigned ops);
void tty_kb_update(iot_dev* pdev);
void tty_kb_ready(iot_dev* pdev);

/********************************************************************************************//**
 * TTY Printer/Paper-Punch Device
 ************************************************************************************************/

void tty_pr_init(iot_dev* pdev);
void tty_pr_iot(iot_dev* pdev, unsigned ops);
void tty_pr_update(iot_dev* pdev);
void tty_pr_ready(iot_dev* pdev);

#endif

