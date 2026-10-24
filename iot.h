/********************************************************************************************//**
 * @file iot.h
 * 
 * PDP-8 IOT instructions
 ************************************************************************************************/

#ifndef	IOT_H
#define	IOT_H

#ifdef	_cplusplus
extern "C" {
#endif

void iot();
void iot_disasm(unsigned instr);
void iot_update_devs();

#ifdef	_cplusplus
}
#endif

#endif
