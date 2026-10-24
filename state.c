/********************************************************************************************//**
 * @file state.c
 * 
 * A PDP-8 Simulator: enum State
 ************************************************************************************************/

#include "state.h"

/********************************************************************************************//**
 * Return state as a string
 ************************************************************************************************/
const char* State2Str(const State* state) {
    switch (*state) {
    case Fetch:     return "F";		break;
    case Defer:     return "D";		break;
    case Execute:   return "E";		break;
    case Break:		return "B";		break;
	case WordCount:	return "WC";	break;
	case CurrAddr:	return "CA";	break;
    default:
		return "Unknown State!";
    }
}

