/********************************************************************************************//**
 * @file decode.cc
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
 * Default constructor
 ************************************************************************************************/
Decode::Decode() : op{OpCode::AND}, i {false}, p {false}, eaddr {0}, bits {0} {}

/********************************************************************************************//**
 * Decode a instruction
 ************************************************************************************************/
Decode::Decode(unsigned instr, unsigned pc) {
	op		= static_cast<OpCode>((instr & Op_Mask)	>> Op_Shift);
	i		= (instr & I_Mask) == I_Mask;
	p		= (instr & P_Mask) == P_Mask;

	const unsigned addr = instr & Addr_Mask;

	eaddr	= p ? pc & Page_Mask : 0;
	eaddr	|=addr;
	bits	= instr & 00777;
}

