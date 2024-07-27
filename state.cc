/********************************************************************************************//**
 * @file state.cc
 * 
 * A PDP-8 Simulator: enum State
 ************************************************************************************************/

#include <cassert>

#include "state.h"

using namespace std;

/********************************************************************************************//**
 ************************************************************************************************/
std::ostream& operator<< (std::ostream& os, const State& state) {
    switch (state) {
    case State::Fetch:      os << " F";	break;
    case State::Defer:      os << " D";	break;
    case State::Execute:    os << " E";	break;
    case State::Break:      os << " B";	break;
    default:
		os << "Unknown State!";
		assert(false);
    }

	return os;
}

