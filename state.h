/********************************************************************************************//**
 * @file state.h
 * 
 * A PDP-8 Simulator: enum State - Major PDP-8 states
 ************************************************************************************************/

#ifndef	STATE_H
#define	STATE_H

#ifdef	_cplusplus
extern "C" {
#endif

/********************************************************************************************//**
 * Major Memory states
 ************************************************************************************************/
typedef enum State {
	Fetch,
	Defer,
	Execute,
	Break,
	WordCount,
	CurrAddr
} State;

const char* State2Str(const State* state);

#ifdef	_cplusplus
}
#endif

#endif

