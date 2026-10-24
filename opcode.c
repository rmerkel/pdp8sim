/********************************************************************************************//**
 * @file opcode.c
 * 
 * PDP8 Simulator: OpCode
 ************************************************************************************************/

#include "opcode.h"

/********************************************************************************************//**
 * Return op as a string.
 ************************************************************************************************/
const char* OpCode2Str(const OpCode op) {
    switch (op) {
    case AND:   return "AND";	break;
    case TAD:   return "TAD";	break;
    case ISZ:   return "ISZ";	break;
    case DCA:   return "DCA";	break;
    case JMS:   return "JMS";	break;
    case JMP:   return "JMP";	break;
    case IOT:   return "IOT";	break;
    case OPR:   return "OPR";	break;
    default:	return "Undefined OpCode!";
    }
}

