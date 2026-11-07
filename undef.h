/********************************************************************************************//**
 * @file undef.h
 * 
 * pdp-8 SYS device
 ************************************************************************************************/

#ifndef	UNDEF_H
#define	UNDEF_H

#include "iot.h"

void undef_init(iot_dev* pdev);
void undef_iot(iot_dev* pdev, unsigned ops);
void undef_mk_rdy(iot_dev* pdev);

#endif

