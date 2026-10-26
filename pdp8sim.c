/********************************************************************************************//**
 * @file pdp8sim.c
 * 
 * A PDP-8 Simulator
 ************************************************************************************************/

#include "pdp8sim.h"

#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "decode.h"
#include "iot.h"
#include "opcode.h"
#include "registers.h"
#include "state.h"

PDP8	p;

/********************************************************************************************//**
 ************************************************************************************************/
static void ral() {
	const uint16_t prevL = p.regs.l;
	p.regs.l = p.regs.ac >> 11;
	p.regs.ac <<= 1;
	p.regs.ac |= prevL;
}

/********************************************************************************************//**
 ************************************************************************************************/
static void rar() {
	const uint16_t prevL = p.regs.l;
	p.regs.l = p.regs.ac & 1;
	p.regs.ac >>= 1;
	p.regs.ac |= prevL << 11;
}

/********************************************************************************************//**
 * OPeRate - Group 1
 ************************************************************************************************/
static void oper_group1(unsigned instr) {
	if (instr == GRP1_NOP)
		return;

	else {
		// Sequence 1

		if ((instr & GRP1_CLA) == GRP1_CLA)	p.regs.ac = 0;
		if ((instr & GRP1_CLL) == GRP1_CLL)	p.regs.l = 0;

		// Sequence ?

		if ((instr & GRP1_CMA) == GRP1_CMA)	p.regs.ac = ~p.regs.ac;
		if ((instr & GRP1_CML) == GRP1_CML)	p.regs.l = ~p.regs.l;
		
		// Sequence ?
		
		if ((instr & GRP1_IAC) == GRP1_IAC)	++p.regs.ac;

		// Sequence ?

		if ((instr & GRP1_RAR) == GRP1_RAR)	rar();
		if ((instr & GRP1_RTR) == GRP1_RTR)	rar();
		if ((instr & GRP1_RAL) == GRP1_RAL)	ral();
		if ((instr & GRP1_RTL) == GRP1_RTL)	ral();
	}
}

/********************************************************************************************//**
 * OPeRate - Group 2
 ************************************************************************************************/
static void oper_group2(unsigned instr) {
	// Sequence 1

	if ((instr & GRP2_SKP_BIT) == GRP2_SKP_BIT) {	// SKP bit is 1
		bool condition = false;						// true for unconditional SKP

		if ((instr & GRP2_SPA) == GRP2_SPA) {
			condition = true;
			if ((p.regs.ac & Sign_Mask) != Sign_Mask)
				++p.regs.pc;
		}

		if ((instr & GRP2_SNA) == GRP2_SNA) {
			condition = true;
			if (p.regs.ac != 0)
				++p.regs.pc;
		}

		if ((instr & GRP2_SZL) == GRP2_SZL) {
			condition = true;
			if (p.regs.l == 0)
				++p.regs.pc;
		}

		if ((instr & GRP2_SKP) == GRP2_SKP && !condition)
			++p.regs.pc;

	} else {							// SKP bit is 0
		if ((instr & GRP2_SMA) == GRP2_SMA)
			if ((p.regs.ac & Sign_Mask) == Sign_Mask)
				++p.regs.pc;

		if ((instr & GRP2_SZA) == GRP2_SZA)
			if (p.regs.ac == 0)
				++p.regs.pc;

		if ((instr & GRP2_SNL) == GRP2_SNL)
			if (p.regs.l != 0)
				++p.regs.pc;
	}
	
	// Sequence 2

	if ((instr & GRP2_CLA) == GRP2_CLA)		p.regs.ac = 0;

	// Sequence 3

	if ((instr & GRP2_OSR) == GRP2_OSR)		p.regs.ac |= p.regs.sr;
	if ((instr & GRP2_HLT) == GRP2_HLT)		p.run = false;
}

/********************************************************************************************//**
 * OPeRate
 ************************************************************************************************/
static void oper(unsigned instr) {
	if ((instr & GROUP1) == 0)
		oper_group1(instr);

	else if ((instr & GROUP2) == GROUP2)
		oper_group2(instr);

	else
		assert(false);						// Other groups are not implenentated
}

/********************************************************************************************//**
 * Fetch state...
 ************************************************************************************************/
static void fetch() {
	if (p.flags.int_en && p.flags.int_pend) {
		p.regs.ir = JMS;					// Force a JMS 0
		p.regs.md = 07000;
		p.regs.ma = 0;
		p.flags.int_en = false;				// Disable interrupts
		p.ion_delay = 0;
       	p.state = Defer;

	} else {								// Normal operation...
		++p.nInstr;

		Decode d;
		p.regs.md 		= p.mem[p.regs.pc];
		decode(p.regs.md, p.regs.pc, &d);
		p.regs.pc++;
		p.regs.ir 		= d.op;
		p.regs.ma 		= d.eaddr;

    	if (p.regs.ir == IOT) {				// IOT?
			iot();
        	p.state = Fetch;

		} else if (p.regs.ir == OPR) {		// OPR?
			oper(p.regs.md);
			p.state = Fetch;

    	} else if (d.i)                  	// Indirect?
       		p.state = Defer;				//	r.ma is the address of the operation
	   
    	else if (p.regs.ir == JMP) {		// JMP direct?
        	p.regs.pc = p.regs.ma;
        	p.state = Fetch;

    } else
       p.state = Execute;
	}
}

/********************************************************************************************//**
 * Defer state...
 ************************************************************************************************/
static void defer() {
	p.regs.md = p.mem[p.regs.ma];			// Fetch indirect operand

	if (p.regs.ma >= 010 && p.regs.ma <= 017)
		p.mem[p.regs.ma] = ++p.regs.md;		// Auto increment

	if (p.regs.ir == JMP) {			// JMP indirect?
		p.regs.pc = p.regs.md;
		p.state = Fetch;

	} else
		p.state = Execute;

	p.regs.ma = p.regs.md;
}

/********************************************************************************************//**
 * Execute state...
 ************************************************************************************************/
static void execute() {
    p.regs.md = p.mem[p.regs.ma];

    switch(p.regs.ir) {
    case AND:
    	p.regs.ac &= p.regs.md;
        break;

	case TAD: {
		uint16_t sum = p.regs.ac + p.regs.md;
		if (sum & ~UINT12_MAX)
			p.regs.l = ~p.regs.l;
		p.regs.ac = sum & UINT12_MAX;
	} break;
			
	case ISZ:
		p.mem[p.regs.ma] = ++p.regs.md;
		if (p.regs.md == 0) 
			++p.regs.pc;
		break;

    case DCA:
		p.regs.md = p.regs.ac;
		p.regs.ac = 0;
		p.mem[p.regs.ma] = p.regs.md;
		break;

    case JMS:
		p.mem[p.regs.ma] = p.regs.pc;
		p.regs.pc = ++p.regs.ma;
		break;

    case JMP:						// Not expected in this state!
		assert(false);
		break;

    case IOT:
		assert(false);						// Not expected in this state!
		break;

    case OPR:						// Not expected in this state!
		assert(false);
		break;
    }

    p.state = Fetch;
}

/********************************************************************************************//**
 * Break (DMA) state
 ************************************************************************************************/
static void brk() {
	assert(false);							// Not implemented
    p.state = Fetch;
}

/********************************************************************************************//**
 ************************************************************************************************/
static void opr_disasm(unsigned instr) {
	
	if ((instr & GROUP1) == 0) {

		if (instr == GRP1_NOP)											printf("NOP ");

		if ((instr & GRP1_CLA) == GRP1_CLA)								printf("CLA ");
		if ((instr & GRP1_CLL) == GRP1_CLL)								printf("CLL ");

		if ((instr & GRP1_CMA) == GRP1_CMA)								printf("CMA ");
		if ((instr & GRP1_CML) == GRP1_CML)								printf("CML ");
		
		if ((instr & GRP1_IAC) == GRP1_IAC)								printf("IAC ");

		if ((instr & GRP1_RTR) == GRP1_RTR)								printf("RTR ");
		else if ((instr & GRP1_RAR) == GRP1_RAR)						printf("RAR ");
		if ((instr & GRP1_RTL) == GRP1_RTL)								printf("RTL ");
		else if ((instr & GRP1_RAL) == GRP1_RAL)						printf("RAL ");

	} else if ((instr & GROUP2) == GROUP2) {
		if ((instr & GRP2_SKP_BIT) == GRP2_SKP_BIT) {	// SKP bit is 1
			bool condition = false;			// true for unconditional SKP

			if ((instr & GRP2_SPA) == GRP2_SPA) {	condition = true;	printf("SPA ");	}
			if ((instr & GRP2_SNA) == GRP2_SNA) {	condition = true;	printf("SNA ");	}
			if ((instr & GRP2_SZL) == GRP2_SZL) {	condition = true;	printf("SZL ");	}

			if ((instr & GRP2_SKP) == GRP2_SKP && !condition)			printf("SKP");

		} else {							// SKP bit is 0
			if ((instr & GRP2_SMA) == GRP2_SMA)							printf("SMA ");
			if ((instr & GRP2_SZA) == GRP2_SZA)							printf("SZA ");
			if ((instr & GRP2_SNL) == GRP2_SNL)							printf("SNL ");
		}
	
		if ((instr & GRP2_CLA) == GRP2_CLA)								printf("CLA ");
		if ((instr & GRP2_OSR) == GRP2_OSR)								printf("OSR ");
		if ((instr & GRP2_HLT) == GRP2_HLT)								printf("HLT ");

	} else
		assert(false);						// Other groups are not supported... yet.
}

/********************************************************************************************//**
 ************************************************************************************************/
static void mri_disasm(const Decode* d) {
	printf("%s ", OpCode2Str(d->op));
	if (d->i)
		fputs("I ", stdout);
	printf("%04o (%04o)", d->eaddr, p.mem[d->eaddr] & UINT8_MAX);
}


/********************************************************************************************//**
 * disasmble the the next instruction
 ************************************************************************************************/
static void disasm(unsigned addr, unsigned instr) {
	Decode d;
	decode(instr, addr, &d);

	printf("%04o (%04o) ", addr, p.mem[addr] & UINT8_MAX);
	switch (d.op) {
		case OPR:	opr_disasm(instr);		break;
		case IOT: 	iot_disasm(instr);		break;
		default:	mri_disasm(&d);
	}
}

/********************************************************************************************//**
 * Dump the processor state
 ************************************************************************************************/
static void dumpState() {
	printf("PC %04o ",	p.regs.pc);
	printf("(%04o) ",	p.mem[p.regs.pc] & UINT8_MAX);
	printf("L %d ",		p.regs.l);
	printf("AC %04o ", 	p.regs.ac);

	printf("MA %04o ", 	p.regs.ma);
	printf("(%04o)    ",p.mem[p.regs.ma] & UINT8_MAX);
    printf("MD %04o ", 	p.regs.md);
    printf("SR %04o\n", p.regs.sr);

	printf("IR %s ",	OpCode2Str(p.regs.ir));
	printf("%s ", 		State2Str(&p.state));
	printf("%u ",  		p.nInstr);
	printf("%u ",  		p.nCycles);
	printf("(%0.1f us)\n", p.runtime);

	if (p.state == Fetch) {
		disasm(p.regs.pc, p.mem[p.regs.pc]);
		putchar('\n');
	}
}

/********************************************************************************************//**
 * @return true if s is a valid 12-bit number
 ************************************************************************************************/
static bool digit(const char* s) {
	const long i = strtol(s, NULL, 0);
	if (i > UINT12_MAX) {
		fprintf(stderr, "%s: %ld must be less than or equal to %d!\n", progName, i, UINT12_MAX);
		return false;

	} else if (i < INT12_MIN) {
		fprintf(stderr, "%s: %ld must be greeater or euqal to %d!\n", progName, i, INT12_MIN);
		return true;

	} else {
		p.regs.sr = i;
		return true;
	}
} 

/********************************************************************************************//**
 * From K&R 2nd Ed, 4.1
 ************************************************************************************************/
static int getline(char* s, int lim) {
	int c;
	int i = 0;
	while (--lim > 0 && (c = getchar()) != EOF && c != '\n')
		s[i++] = c;
	if (c == '\n')
		s[i++] = c;
	s[i] = '\0';

	return i;
}

/********************************************************************************************//**
 * @note	maybe replace with a map<string, action>?
 *
 * @return true to exit the simulator
 ************************************************************************************************/
static bool frontpanel() {
			char cmd[100];				// current command
	static	char lcmd[100] = {0};		// last command

	cmd[0] = '\0';
	dumpState();
    fputs("> ", stdout);

	int len = getline(cmd, sizeof(cmd));
	if (len == 0)
		return true;					// Ctrl-D - exit

	cmd[--len] = '\0';					// Zap newline

	if (len == 0)
		strcpy(cmd, lcmd);				// Repeat last...
	if (cmd[0] == 'c')
		p.run = true;
	else if (cmd[0] == '?' || cmd[0] == 'h') {
		fputs("<number>     -- Set SR\n", stdout);
		fputs("?|h[elp]     -- Print help\n", stdout);
		fputs("c[ont]       -- Continue\n", stdout);
		fputs("e[examine]   -- Examine memory\n", stdout);
		fputs("ld[addr]     -- Load address\n", stdout);
		fputs("la[ddr]      -- Load address\n", stdout);
		fputs("[no]si[nstr] -- Single instruction\n", stdout);
		fputs("[no]ss[tep]  -- Single step\n", stdout);
		fputs("s[tart]      -- Start\n", stdout);
		fputs("q[uit]       -- Exit\n", stdout);
		fputs("<return>     -- Repeat last command\n", stdout);
		fputs("<ctrl-c>     -- Halt\n", stdout);
		fputs("<ctrl-d>     -- Same as q[uit]\n", stdout);
	} else if (cmd[0] == 'e') {			// examine memory
		p.regs.md = p.mem[p.regs.pc];
		p.regs.ma = p.regs.pc++;
	} else if (cmd[0] == 'l')			// ld or la
		p.regs.pc = p.regs.sr;			// load address
	else if (strcmp(cmd, "nosi") == 0)	// noinstr
		p.flags.sinstr = false;
	else if (strcmp(cmd, "noss") == 0)	// nosstep
		p.flags.sstep = false;
	else if (strcmp(cmd, "si") 	== 0)	// sinstr
		p.flags.sinstr = true;
	else if (strcmp(cmd, "ss")	== 0)	// sstep
		p.flags.sstep = true;
	else if (cmd[0] == 's') {			// start
		p.regs.l				= false;
		p.regs.ac	= p.regs.md	= 0;
		p.regs.ma 				= p.regs.pc = p.regs.sr;
		p.state					= Fetch;
		p.run 					= true;
	} else if (cmd[0] == 'q')
		return true;					// quit
	else if (digit(cmd))				// number (simple)
		;
    else 
		fprintf(stderr, "%s Unknown command: %s\n", progName, cmd);

	strcpy(lcmd, cmd);
        
	return false;
}

/********************************************************************************************//**
 * SIGINT handler 
 *
 * @bug Do we need to register the SIGINT handler here?
 ************************************************************************************************/
void signal_handler(int signal_num) {
	assert(signal_num ==  SIGINT);
	(void) (signal_num);
	p.run = false;

	if (signal(SIGINT, signal_handler) == SIG_ERR) {
		fprintf(stderr, "%s: error registering SIGINT handler!\n", progName);
		assert(false);
	}
	putchar('\n');					// So that the "^C" is on it's own line
}

/********************************************************************************************//**
 * Run the processor/debugger...
 ************************************************************************************************/
int process() {
	p.run = false;					// Processor starts in idle mode...
    for (;;) {
		if (p.run) {
            do {					// Next instruction (mem[r.pc])
				iot_update_devs();

                do {				// 	Next memory state
                	switch(p.state) {
                    case Fetch:      fetch();    break;
                    case Defer:      defer();    break;
                    case Execute:    execute();  break;
                    case Break:      brk();      break;
                    default:
						assert(false);	// shouldn't be possible
						fprintf(stderr, "%s: unknown state!\n", progName);
                    }

					++p.nCycles;
					p.runtime += 1.5;	// 1.5us per cycle, except for IOT

                } while (p.run && !p.flags.sstep && p.state != Fetch);
            } while (p.run && !p.flags.sinstr && !p.flags.sstep);

			if (p.ion_delay && --p.ion_delay == 0)
				p.flags.int_en  = true;	// ION takes effect

            p.run = p.run && !p.flags.sstep && !p.flags.sinstr;

        } else if (frontpanel())
			return 0;

		// else: keep going...
	}
}

/********************************************************************************************//**
 * Initialize the simulator
 *
 * @bug	Maybe allocate and return a PDP8 object, to be passed to [PDP8sim_]process()?
 ************************************************************************************************/
int pdp8_init() {
	memset(&p, 0, sizeof(p));

	// Ctrl-C (SIGINT) as the front pannel HALT switch
	if (signal(SIGINT, signal_handler) == SIG_ERR) {
		fprintf(stderr, "%s: error registering SIGINT handler!\n", progName);
		return 1;
	}

	return 0;
}

