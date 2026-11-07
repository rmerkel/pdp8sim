/********************************************************************************************//**
 * @file iot.h
 * 
 * PDP-8 IOT instructions
 ************************************************************************************************/

#ifndef	IOT_H
#define	IOT_H

#include "iot.h"
#include <stdbool.h>

#ifdef	_cplusplus
extern "C" {
#endif

/********************************************************************************************//**
 * A PDP-8 IOT Device
 ************************************************************************************************/
typedef struct iot_dev {			// IO Device interface
	unsigned	id;					//	Device ID
	bool		rdy;				//	Device ready flag
	bool		rdy_pend;			//	Device ready pending
	double		rdy_delay_s;		//	Last time ready timestamp (secs)

									//	Initialize pdev
	void		(*init)(struct iot_dev* pdev);
									//	Interact with pdev with contents of ops
	void		(*iot)(struct iot_dev* pdev, unsigned ops);
									//	Make pdev ready
	void		(*mk_rdy)(struct iot_dev* pdev);

} iot_dev;

// device utilites
bool iop1(unsigned ops);
bool iop2(unsigned ops);
bool iop4(unsigned ops);

void iot_init(void);
bool iot_int_pending(void);
void iot(void);
void iot_disasm(unsigned instr);
void iot_update(void);

#ifdef	_cplusplus
}
#endif

#endif
