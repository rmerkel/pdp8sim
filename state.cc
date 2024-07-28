/********************************************************************************************//**
 * @file state.cc
 * 
 * A PDP-8 Simulator: enum State
 ************************************************************************************************/

#include <cassert>

#include "state.h"

using namespace std;

/********************************************************************************************//**
 * Write the "short" name for state on to the output stream (os).
 ************************************************************************************************/
std::ostream& operator<< (std::ostream& os, const State& state) {
    switch (state) {
    case State::Fetch:      os << "F";	break;
    case State::Defer:      os << "D";	break;
    case State::Execute:    os << "E";	break;
    case State::Break:      os << "B";	break;
	case State::WordCount:	os << "WC";	break;
	case State::CurrAddr:	os << "CA"; break;
    default:
		os << "Unknown State!";
		assert(false);
    }

	return os;
}

