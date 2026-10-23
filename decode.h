/********************************************************************************************//**
 * @file decode.h
 * 
 * class Decode
 ************************************************************************************************/

#ifndef	DECODE_H
#define	DECODE_H

#include <cstdint>

#include "opcode.h"

/********************************************************************************************//**
 * Decode instructon
 ************************************************************************************************/
struct Decode {
	OpCode		op;							///< Opcode
	bool		i;							///< Indirect?
	bool		p;							///< Current page?
	uint16_t	eaddr;						///< Effective addr
	uint16_t	bits;						///< bits 3-11 of the instruction (for OPR, IOT, tbd)

	Decode();
	Decode(unsigned instr, unsigned pc);
};

#endif
