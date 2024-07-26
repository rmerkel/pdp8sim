/********************************************************************************************//**
 * @file state.cc
 * 
 * A PDP-8 Simulator: enum State
 ************************************************************************************************/

#include "state.h"

using namespace std;

/********************************************************************************************//**
 ************************************************************************************************/
std::ostream& operator<< (std::ostream& os, const State& state) {
    switch (state) {
    case State::Fetch:      os << "Fetch";		break;
    case State::Defer:      os << "Defer";		break;
    case State::Execute:    os << "Execute";	break;
    case State::Break:      os << "Break";		break;
    default:                os << "Unknown State!";
    }

	return os;
}

