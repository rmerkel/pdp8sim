/********************************************************************************************//**
 * @file pdp8sim.h
 * 
 * A PDP-8 Simulator
 ************************************************************************************************/

#ifndef	PDP8SIM_H
#define	PDP8SIM_H

#ifdef	_cplusplus
extern "C" {
#endif

#include "registers.h"
#include "state.h"

#include <stdbool.h>

extern const char* progName;

/************************************************************************************************
 * Constants
 ************************************************************************************************/

#define UINT12_MAX		07777
#define	INT12_MAX		+2047
#define	INT12_MIN		-2048

/************************************************************************************************
 * Bit masks
 ************************************************************************************************/

#define Sign_Mask		04000	///< 2's complement sign mask

// OPER Group 1

#define GROUP1			00400	///< Bit 3 is clear

#define GRP1_NOP		07000	///< NOP
#define GRP1_CLA		07200	///< Clear AC,	sequence 1
#define GRP1_CLL		07100	///< Clear link, sequence 1
#define GRP1_CMA		07040	///< Complement AC, sequence 2
#define GRP1_CML		07020	///< Complement link, sequence 2
#define GRP1_RAR		07010	///< Rotate AC and L right 1, sequence 4
#define GRP1_RTR		07012	///< Rotate AC and L right 2, sequence 4
#define GRP1_RTL		07006	///< Rotate AC and L left 1, sequence 4
#define GRP1_RAL		07004	///< Rotate AC and L left 2, sequence 4
#define GRP1_IAC		07001	///< Increment AC, sequence 3

// OPER Group 2

#define GROUP2			00400	///< Bit 3 is set, bit 11 is clear.

#define	GRP2_SKP_BIT	00010	///< Skip bit for bits 5-7

#define	GRP2_SMA		07500	///< Skip on minus AC, sequence 1
#define	GRP2_SZA		07440	///< Skip on zero AC, sequence 1
#define	GRP2_SPA		07510	///< Skip on plus AC, sequence 1
#define 	GRP2_SNA	07450	///< Skip on plus AC, sequence 1
#define	GRP2_SNL		07420	///< Skip on non-zero AC, sequence 1
#define	GRP2_SZL		07430	///< Skip on zero link, sequence 1
#define	GRP2_SKP		07410	///< Skip unconditionally, sequence 1
#define	GRP2_OSR		07404	///< Inclusive OR, switch register with AC, sequence 3
#define	GRP2_HLT		07402	///< Halt the processor, sequence 3
#define	GRP2_CLA		07600	///< Clear AC, sequence 2

// IOT

#define	IOT_DEV_SEL		00770	///< Device ID
#define	IOT_DEV_SHIFT	3

#define	IOT_OP			00007	///< IO Operations

#define	ION_DELAY		3		///< # of instructions delay before ION becomes active

/********************************************************************************************//**
 * Front Panel switches
 ************************************************************************************************/
typedef struct Flags {
    bool        sstep   	: 1;			// Front-panel single-step (cycle) switch
    bool        sinstr  	: 1;			// Front-panel single-instruction switch
	bool		int_en		: 1;			// true if interrupts are inabled.
	bool		int_pend	: 1;			// true if an interrupt is pending
} Flags;

/************************************************************************************************
 * Processor State
 *
 * @bug	Cumulative run time needs to be accumulated here.
 ************************************************************************************************/
typedef struct PDP8 {
	bool        run;
	Flags    	flags;
	unsigned	ion_delay;					// int_en set when this counts down to zero
	Registers	regs;
	State       state;
	unsigned    mem[4096];
	unsigned	nCycles;
	unsigned	nInstr;
	double		runtime;					// cumulative runtine in us.
} PDP8;

/********************************************************************************************//**
 ************************************************************************************************/
extern PDP8	p;

int pdp8_init();
int process();
void signal_handler(int signal_num);

#ifdef	_cplusplus
}
#endif

#endif
