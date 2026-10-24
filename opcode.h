/********************************************************************************************//**
 * @file opcode.h
 * 
 * A PDP-8 Simulator: enum OpCode
 ************************************************************************************************/

#ifndef	OPCODE_H
#define	OPCODE_H

#ifdef	_cplusplus
extern "C" {
#endif

#include <ostream>

/********************************************************************************************//**
 * PDP-8 Operation codes for the basic instructions
 ************************************************************************************************/
typedef enum OpCode {
    AND = 0,
    TAD = 1,
    ISZ = 2,
    DCA = 3,
    JMS = 4,
    JMP = 5,
    IOT = 6,
    OPR = 7
} OpCode;

const char* OpCode2Str(const OpCode* op);

#ifdef	_cplusplus
}
#endif

#endif
