/********************************************************************************************//**
 * @file opcode.cc
 * 
 * PDP8 Simulator: OpCode
 ************************************************************************************************/

#include "opcode.h"

using namespace std;

/********************************************************************************************//**
 ************************************************************************************************/
ostream& operator<< (ostream& os, const OpCode& op) {
    switch (op) {
    case OpCode::AND:   os << "AND";	break;
    case OpCode::TAD:   os << "TAD";	break;
    case OpCode::ISZ:   os << "ISZ";	break;
    case OpCode::DCA:   os << "DCA";	break;
    case OpCode::JMS:   os << "JMS";	break;
    case OpCode::JMP:   os << "JMP";	break;
    case OpCode::IOT:   os << "IOT";	break;
    case OpCode::OPR:   os << "OPR";	break;
    default:            os << "Undefined OpCode!";
    }

	return os;
}

