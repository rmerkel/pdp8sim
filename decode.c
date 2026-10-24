/********************************************************************************************//**
 * @file decode.c
 * 
 * class Decode
 ************************************************************************************************/

#include "decode.h"

const unsigned Page_Mask    	= 07600;	///< PC Address Page address mask
const unsigned Op_Mask			= 07000;	///< OpCode mask
const unsigned Op_Shift			= 9;		///< OpCode shift
const unsigned I_Mask			= 00400;	///< Indirect bit
const unsigned P_Mask			= 00200;	///< Page bit
const unsigned Addr_Mask		= 00177;	///< Address/page offset mask

/********************************************************************************************//**
 * Decode a instruction
 ************************************************************************************************/
void decode(unsigned instr, unsigned ppc, Decode* d) {
	d->op		= (OpCode) ((instr & Op_Mask) >> Op_Shift);
	d->i		= (instr & I_Mask) == I_Mask;
	d->p		= (instr & P_Mask) == P_Mask;

	const unsigned addr = instr & Addr_Mask;

	d->eaddr	= d->p ? ppc & Page_Mask : 0;
	d->eaddr	|=addr;
	d->bits		= instr & 00777;
}

