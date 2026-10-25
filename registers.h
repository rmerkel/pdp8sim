/********************************************************************************************//**
 * @file registers.h
 * 
 * PDP-8 Registers
 ************************************************************************************************/

#ifndef	REGISTERS_H
#define REGISTERS_H

#include "opcode.h"

#ifdef	_cplusplus
extern "C" {
#endif

/********************************************************************************************//**
 * PDP8 Registers
 ************************************************************************************************/
typedef struct Registers {
	unsigned	pc	: 12;       		///< Program Counter - may expand to include df and if
	unsigned	ac	: 12;				///< ACcumulator register
	unsigned	 l	:  1;				///< Link register
	unsigned	ma	: 12;       		///< Memory address register
	unsigned	md	: 12;				///< Memory data register
	unsigned	sr	: 12;				///< Switch register
	OpCode		ir;
} Registers;

#ifdef	_cplusplus
}
#endif

#endif
