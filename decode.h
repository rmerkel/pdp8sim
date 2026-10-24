/********************************************************************************************//**
 * @file decode.h
 * 
 * class Decode
 ************************************************************************************************/

#ifndef	DECODE_H
#define	DECODE_H

#ifdef	_cplusplus
extern "C" {
#endif

#include <cstdint>

#include "opcode.h"

/********************************************************************************************//**
 * Decoded instructon
 ************************************************************************************************/
typedef struct Decode {
	OpCode		op;							///< Opcode
	bool		i;							///< Indirect?
	bool		p;							///< Current page?
	uint16_t	eaddr;						///< Effective addr
	uint16_t	bits;						///< bits 3-11 of the instruction (for OPR, IOT)

} Decode;

void decode(unsigned instr, unsigned pc, Decode* decoded);

#ifdef	_cplusplus
}
#endif

#endif
