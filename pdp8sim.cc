/********************************************************************************************//**
 * @file pdp8sim.cc
 * 
 * A PDP-8 Simulator
 ************************************************************************************************/

#include "pdp8sim.h"

#include <cassert>
#include <csignal>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <string>
#include <sys/select.h>
#include <unistd.h>

#include "decode.h"
#include "iot.h"
#include "opcode.h"
#include "registers.h"
#include "state.h"

using namespace std;

const char* progName = "pdp8sim";

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
	if (	 (instr & GROUP1) == 0)
		oper_group1(instr);

	else if ((instr & GROUP2) == GROUP2)
		oper_group2(instr);

	else
		assert(false);						// Other groups are not implenentated
}

/********************************************************************************************//**
 * Fetch next instruction, handle JMP direct
 ************************************************************************************************/
void fetch() {
	++p.nInstr;

	p.regs.md 		= p.mem[p.regs.pc++];
	const Decode d	= Decode(p.regs.md, p.regs.pc);
	p.regs.ir 		= d.op;
	p.regs.ma 		= d.eaddr;

    if (p.regs.ir == OpCode::IOT) {			// IOT?
		iot();
        p.state = State::Fetch;

	} else if (p.regs.ir == OpCode::OPR) {	// OPR?
		oper(p.regs.md);
		p.state = State::Fetch;

    } else if (d.i)                  		// Indirect?
       p.state = State::Defer;				//	r.ma is the address of the operation
	   
    else if (p.regs.ir == OpCode::JMP) {	// JMP direct?
        p.regs.pc = p.regs.ma;
        p.state = State::Fetch;

    } else
       p.state = State::Execute;
}

/********************************************************************************************//**
 * Defer state
 ************************************************************************************************/
void defer() {
	p.regs.md = p.mem[p.regs.ma];			// Fetch indirect operand

	if (p.regs.ma >= 010 && p.regs.ma <= 017)
		p.mem[p.regs.ma] = ++p.regs.md;		// Auto increment

	if (p.regs.ir == OpCode::JMP) {			// JMP indirect?
		p.regs.pc = p.regs.md;
		p.state = State::Fetch;

	} else
		p.state = State::Execute;

	p.regs.ma = p.regs.md;
}

/********************************************************************************************//**
 * Execute state
 ************************************************************************************************/
void execute() {
    p.regs.md = p.mem[p.regs.ma];

    switch(p.regs.ir) {
    case OpCode::AND:
    	p.regs.ac &= p.regs.md;
        break;

	case OpCode::TAD: {
		uint16_t sum = p.regs.ac + p.regs.md;
		if (sum & ~UINT12_MAX)
			p.regs.l = ~p.regs.l;
		p.regs.ac = sum & UINT12_MAX;
	} break;
			
	case OpCode::ISZ:
		p.mem[p.regs.ma] = ++p.regs.md;
		if (p.regs.md == 0) 
			++p.regs.pc;
		break;

    case OpCode::DCA:
		p.regs.md = p.regs.ac;
		p.regs.ac = 0;
		p.mem[p.regs.ma] = p.regs.md;
		break;

    case OpCode::JMS:
		p.mem[p.regs.ma] = p.regs.pc;
		p.regs.pc = ++p.regs.ma;
		break;

    case OpCode::JMP:						// Not expected in this state!
		assert(false);
		break;

    case OpCode::IOT:
		assert(false);						// Not expected in this state!
		break;

    case OpCode::OPR:						// Not expected in this state!
		assert(false);
		break;
    }

    p.state = State::Fetch;
}

/********************************************************************************************//**
 * Break (DMA) state
 ************************************************************************************************/
void brk() {
	assert(false);							// Not implemented
    p.state = State::Fetch;
}

/********************************************************************************************//**
 ************************************************************************************************/
static void disasm_opr(unsigned instr) {
	
	if ((instr & GROUP1) == 0) {

		if (instr == GRP1_NOP)												cout << "NOP ";

		if ((instr & GRP1_CLA) == GRP1_CLA)									cout << "CLA ";
		if ((instr & GRP1_CLL) == GRP1_CLL)									cout << "CLL ";

		if ((instr & GRP1_CMA) == GRP1_CMA)									cout << "CMA ";
		if ((instr & GRP1_CML) == GRP1_CML)									cout << "CML ";
		
		if ((instr & GRP1_IAC) == GRP1_IAC)									cout << "IAC ";

		if ((instr & GRP1_RTR) == GRP1_RTR)									cout << "RTR ";
		else if ((instr & GRP1_RAR) == GRP1_RAR)							cout << "RAR ";
		if ((instr & GRP1_RTL) == GRP1_RTL)									cout << "RTL ";
		else if ((instr & GRP1_RAL) == GRP1_RAL)							cout << "RAL ";

	} else if ((instr & GROUP2) == GROUP2) {
		if ((instr & GRP2_SKP_BIT) == GRP2_SKP_BIT) {	// SKP bit is 1
			bool condition = false;			// true for unconditional SKP

			if ((instr & GRP2_SPA) == GRP2_SPA) {	condition = true;	cout << "SPA ";	}
			if ((instr & GRP2_SNA) == GRP2_SNA) {	condition = true;	cout << "SNA ";	}
			if ((instr & GRP2_SZL) == GRP2_SZL) {	condition = true;	cout << "SZL ";	}

			if ((instr & GRP2_SKP) == GRP2_SKP && !condition)			cout << "SKP";

		} else {							// SKP bit is 0
			if ((instr & GRP2_SMA) == GRP2_SMA)							cout << "SMA ";
			if ((instr & GRP2_SZA) == GRP2_SZA)							cout << "SZA ";
			if ((instr & GRP2_SNL) == GRP2_SNL)							cout << "SNL ";
		}
	
		if ((instr & GRP2_CLA) == GRP2_CLA)								cout << "CLA ";
		if ((instr & GRP2_OSR) == GRP2_OSR)								cout << "OSR ";
		if ((instr & GRP2_HLT) == GRP2_HLT)								cout << "HLT ";

	} else
		assert(false);						// Other groups are not supported... yet.
}

/********************************************************************************************//**
 ************************************************************************************************/
static void disasm_mri(const Decode& d) {
	cout 	<< d.op	<< ' ';
	if (d.i)
		cout << "I ";
	cout	<< setw(4)	<< d.eaddr
			<< " (" << setw(4) << p.mem[d.eaddr] << ')';
}


/********************************************************************************************//**
 * disasmble the the next instruction
 ************************************************************************************************/
static void disasm(unsigned addr, unsigned instr) {
	const Decode d = Decode(instr, addr);

	cout	<< oct << setfill('0');
	cout	<< setw(4) 	<< addr 		<< ' '
			<< setw(4)	<< p.mem[addr]	<< ' ';
	switch (d.op) {
		case OpCode::OPR:	disasm_opr(instr);		break;
		case OpCode::IOT: 	disasm_iot(instr);		break;
		default:			disasm_mri(d);
	}
}

/********************************************************************************************//**
 * Dump the processor state
 ************************************************************************************************/
static void dumpState() {
	cout << oct << setfill('0');

    cout
			<< "PC "	<< setw(4)	<< p.regs.pc		<< ' '
			<< '('		<< setw(4) 	<< p.mem[p.regs.pc]	<< ") "
		 	<< "L "					<< p.regs.l 		<< ' '
			<< "AC "	<< setw(4)	<< p.regs.ac		<< '\n' 

    		<< "MA "	<< setw(4)	<< p.regs.ma		<< ' '
			<< '('		<< setw(4)	<< p.mem[p.regs.ma]	<< ")     "
           	<< "MD "	<< setw(4)	<< p.regs.md		<< ' '
           	<< "SR "	<< setw(4)	<< p.regs.sr		<< '\n'

			<< "IR "				<< p.regs.ir		<< ' '
			<< setfill(' ')
						<< setw(2)  << p.state			<< ' '
						<< setw(4)	<< p.nInstr			<< " Instructions, "
						<< setw(4)	<< p.nCycles		<< " Cycles, "
			<< '(' 					<< p.runtime		<< " us)"
			<< '\n';

	if (p.state == State::Fetch) {
		disasm(p.regs.pc, p.mem[p.regs.pc]);
		cout	<< '\n';
	}
}

/********************************************************************************************//**
 * @return true if s is a number
 ************************************************************************************************/
static bool digit(const string& s) {
	try {
		const int i{std::stoi(s, nullptr, 0)};

		if (static_cast<unsigned>(i) > UINT12_MAX)
			cerr << "'" << i << "i is greater than " << INT12_MAX << "\n";

		else if (i < INT12_MIN)
			cerr << "'" << i << "i is less than " << UINT12_MAX << "\n";

		else
			p.regs.sr = i;

	} catch (std::invalid_argument const& ex) {
        return false;							// Ignore, not a "digit"

   	} catch (std::out_of_range const& ex) {
        std::cout << "std::out_of_range::what(): " << ex.what() << '\n';
   	}	

	return true;
}

/********************************************************************************************//**
 * @note	maybe replace with a map<string, action>?
 *
 * @return true to exit the simulator
 ************************************************************************************************/
static bool frontpanel() {
	dumpState();
 
    static string lcmd = "?";			// last comand
    string cmd = "";					// current command

    cout << "> ";
    if (!getline(cin, cmd))
		return true;					// Ctrl-D - exit

	if (cmd == "")
		cmd = lcmd;						// Repeat last...

	if (cmd.find("c") == 0)				// C[ontinue]
		p.run = true;

										// Help
	else if (cmd == "?" || cmd.find("h") == 0) {
		cout	<< "<number>     -- Set SR\n"
				<< "?|h[elp]     -- Print help\n"
				<< "c[ont]       -- Continue\n"
				<< "e[examine]   -- Examine memory\n"
				<< "ld[addr]     -- Load address\n"
				<< "la[ddr]      -- Load address\n"
				<< "[no]si[nstr] -- Single instruction\n"
				<< "[no]ss[tep]  -- Single step\n"
				<< "s[tart]      -- Start\n"
				<< "q[uit]       -- Exit\n"
				<< "<return>     -- Repeat last command\n"
				<< "<ctrl-c>     -- Halt\n"
				<< "<ctrl-d>     -- Same as q[uit]\n";
	} else if (cmd.find("e")	== 0) {	// examine memory
		p.regs.md = p.mem[p.regs.pc];
		p.regs.ma = p.regs.pc++;
	} else if (cmd.find("ld")	== 0 || cmd.find("la") == 0)
		p.regs.pc = p.regs.sr;			// load address
	else if (cmd.find("nosi")	== 0)	// noinstr
		p.sw.sinstr = false;
	else if (cmd.find("noss")	== 0)	// nosstep
		p.sw.sstep = false;
	else if (cmd.find("si")		== 0)	// sinstr
		p.sw.sinstr = true;
	else if (cmd.find("ss")		== 0)	// sstep
		p.sw.sstep = true;
	else if (cmd.find( "s")		== 0) {	// start
		p.regs.l				= false;
		p.regs.ac = p.regs.md	= 0;
		p.regs.ma 				= p.regs.pc;			// Shouldn't this be from the switch register?
		p.state					= State::Fetch;
		p.run 					= true;
	} else if (cmd == "q" || cmd == "quit")
		return true;					// quit
	else if (digit(cmd))				// number (simple)
		;
    else 
		cerr << "Unknown command: '" << cmd << "!\n";

	lcmd = cmd;
        
	return false;
}

/********************************************************************************************//**
 * SIGINT handler 
 ************************************************************************************************/
void signal_handler(int signal_num) {
	assert(signal_num ==  SIGINT);
	p.run = false;
	cout << endl;
}

/********************************************************************************************//**
 * Run the processor/debugger...
 ************************************************************************************************/
int process() {
	p.run = false;					// Processor starts in idle mode...
    for (;;) {
		if (p.run) {
            do {					// Next instruction (mem[r.pc])
				update_dev_ready();
                do {				// 	Next memory state

                    switch(p.state) {
                    case State::Fetch:      fetch();    break;
                    case State::Defer:      defer();    break;
                    case State::Execute:    execute();  break;
                    case State::Break:      brk();      break;
                    default: cerr << "unknown state!\n";
                    }

					++p.nCycles;
					p.runtime += 1.5;

					// Calc cumulative runtime here...

                } while (p.run && !p.sw.sstep && p.state != State::Fetch);
            } while (p.run && !p.sw.sinstr && !p.sw.sstep);

            p.run = p.run && !p.sw.sstep && !p.sw.sinstr;

        } else if (frontpanel())
			return 0;

		// else: keep going...
	}
}

/********************************************************************************************//**
 * Load a BIN file into memory
 ************************************************************************************************/
static bool load_BIN (const string& filename) {
	enum class BIN_State { Leader, OriginMSB, OriginLSB, DataMSB, DataLSB, Trailer };

	ifstream ifs{filename};
	if (!ifs) {
		cerr << progName << ": can't open '" << filename << "'!\n";
		return false;
	}

	const unsigned BIN_LEADER 		= 00200;
	const unsigned BIN_ORG_Mask		= 00100;
	const unsigned BIN_DATA_Mask	= 00077;
	const unsigned BIN_MSB_Shift	= 6;

	unsigned	data = 0;
	uint8_t		byte;
	char		c;
	BIN_State	bstate = BIN_State::DataMSB;

	while (ifs.get(c)) {
		byte = static_cast<uint8_t>(c);

		if (byte == BIN_LEADER)
			continue;

		if ((byte & BIN_ORG_Mask) == BIN_ORG_Mask)
			bstate = BIN_State::OriginMSB;

		switch (bstate) {
			case BIN_State::Leader:
				assert(false);
				break;

			case BIN_State::OriginMSB:
				p.regs.pc = (byte & BIN_DATA_Mask) << BIN_MSB_Shift;
				bstate = BIN_State::OriginLSB;
				break;

			case BIN_State::OriginLSB:
				p.regs.pc |= byte;
				bstate = BIN_State::DataMSB;
				break;

			case BIN_State::DataMSB:
				data = (byte & BIN_DATA_Mask) << BIN_MSB_Shift;
				bstate = BIN_State::DataLSB;
				break;

			case BIN_State::DataLSB:
				data |= byte;
				p.mem[p.regs.pc++] = data;
				bstate = BIN_State::DataMSB;
				break;

			default:
				assert(false);
		}
	}

	return true;
}

/********************************************************************************************//**
 * Write an help diagnostic to standard error.
 ************************************************************************************************/
static void help() {
	cerr	<< "Usage: " << progName << " [options... | filenames...]\n"
			<< "Where options is zero or more of:\n"
			<< "-h|?     -- print this message, and return 1\n"
			<< "-v       -- print the version, and return 1\n"
			<< '\n'
			<< "And where filenames is zero or more program file names to load in BIN format\n";
}

/********************************************************************************************//**
 * The PDP8 simulator
 ************************************************************************************************/
int main (int argc, char** argv) {
	// Ctrl-C (SIGINT) as the front pannel HALT switch
	if (signal(SIGINT, signal_handler) == SIG_ERR) {
		cerr << progName << ": error registering SIGINT handler!" << endl;
		return 1;
	}


	// Scan the command line...
	for (int argn = 1; argn < argc; ++argn) {
		const string arg = argv[argn];

		if (arg == "")
			continue;

		if (arg == "-")
			cerr << progName << ": unknown option '-',\n";

		if (arg[0] == '-') {
			for (auto i = arg.begin() + 1; i != arg.end(); ++i) {
				char c = *i;

				switch(c) {
					case '?': case 'h': help();			return 1;
					case 'v': cout << "version 0.9\n";	return 1;
					default:
						cerr << progName << ": unknown option '" << c << "'.\n";
						return 1;
				}
			}

		} else if (!load_BIN(arg))			// Load a program file...
			return 1;
	}

	return process();						// Run the simulation...
}

