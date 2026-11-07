/********************************************************************************************//**
 * @file sys.h
 * 
 * pdp-8 SYS device
 ************************************************************************************************/

#ifndef	SYS_H
#define	SYS_H

#include "iot.h"

void sys_init(iot_dev* pdev);
void sys_iot(iot_dev* pdev, unsigned ops);
void sys_ready(iot_dev* pdev);

#endif

