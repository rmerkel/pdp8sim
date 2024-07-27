/********************************************************************************************//**
 * @file pdp8sim.cc
 * 
 * A PDP-8 Simulator
 ************************************************************************************************/

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <string>

#include "opcode.h"
#include "state.h"

using namespace std;

static const char* progName = "pdp8sim";

/************************************************************************************************
 * Constants
 ************************************************************************************************/

const unsigned UINT12_MAX		= 07777;
const int	   INT12_MAX		= +2047;
const int	   INT12_MIN		= -2048;

/************************************************************************************************
 * Bit maskes
 ************************************************************************************************/

const unsigned Page_Mask    	= 07600;	///< PC Address Page address mask
const unsigned Op_Mask			= 07000;	///< OpCode mask
const unsigned Op_Shift			= 9;		///< OpCode shift
const unsigned I_Mask			= 00400;	///< Indirect bit
const unsigned P_Mask			= 00200;	///< Page bit
const unsigned Addr_Mask		= 00177;	///< Address/page offset mask

const unsigned Sign_Mask		= 04000;	///< 2's complement sign mask

// OPER Group 1

const unsigned GROUP1			= 00400;	///< Bit 3 is clear

const unsigned GRP1_NOP			= 07000;	///< NOP
const unsigned GRP1_CLA			= 07200;	///< Clear AC,	sequence 1
const unsigned GRP1_CLL			= 07100;	///< Clear link, sequence 1
const unsigned GRP1_CMA			= 07040;	///< Complement AC, sequence 2
const unsigned GRP1_CML			= 07020;	///< Complement link, sequence 2
const unsigned GRP1_RAR			= 07010;	///< Rotate AC and L right 1, sequence 4
const unsigned GRP1_RTR			= 07012;	///< Rotate AC and L right 2, sequence 4
const unsigned GRP1_RTL			= 07006;	///< Rotate AC and L left 1, sequence 4
const unsigned GRP1_RAL			= 07004;	///< Rotate AC and L left 2, sequence 4
const unsigned GRP1_IAC			= 07001;	///< Increment AC, sequence 3

// OPER Group 2

const unsigned GROUP2			= 00400;	///< Bit 3 is set, bit 11 is clear.

const unsigned	GRP2_SKP_BIT	= 00010;	///< Skip bit for bits 5-7

const unsigned	GRP2_SMA		= 07500;	///< Skip on minus AC, sequence 1
const unsigned	GRP2_SZA		= 07440;	///< Skip on zero AC, sequence 1
const unsigned	GRP2_SPA		= 07510;	///< Skip on plus AC, sequence 1
const unsigned 	GRP2_SNA		= 07450;	///< Skip on plus AC, sequence 1
const unsigned	GRP2_SNL		= 07420;	///< Skip on non-zero AC, sequence 1
const unsigned	GRP2_SZL		= 07430;	///< Skip on zero link, sequence 1
const unsigned	GRP2_SKP		= 07410;	///< Skip unconditionally, sequence 1
const unsigned	GRP2_OSR		= 07404;	///< Inclusive OR, switch register with AC, sequence 3
const unsigned	GRP2_HLT		= 07402;	///< Halt the processor, sequence 3
const unsigned	GRP2_CLA		= 07600;	///< Clear AC, sequence 2

// IOT

const unsigned	IOT_DEV_SEL		= 00770;	///< Device ID
const unsigned	IOT_DEV_SHIFT	= 3;

const unsigned	IOT_OP			= 00007;	///< Operations

/********************************************************************************************//**
 * PDP8 Registers
 ************************************************************************************************/
struct Registers {
    uint16_t    pc      : 12;       		///< Program Counter - may expand to include df and if
    uint16_t    ac      : 12;				///< ACcumulator register
    uint16_t     l      :  1;				///< Link register
    uint16_t    ma      : 12;       		///< Memory address register
    uint16_t    md      : 12;				///< Memory data register
	uint16_t	sr		: 12;				///< Switch register
	OpCode		ir;

    Registers() : pc{0}, ac{0}, l{0}, ma{0}, md{0}, sr{0}, ir{OpCode::AND}  {}
};

/********************************************************************************************//**
 * Decoded instructon
 ************************************************************************************************/
struct Decoded {
	OpCode		op;							///< Opcode
	bool		i;							///< Indirect?
	bool		p;							///< Current page?
	uint16_t	eaddr;						///< Effective addr
	uint16_t	bits;						///< bits 3-11 of the instruction (for OPR, IOT, tbd)
};

/********************************************************************************************//**
 * Front Panel switches
 ************************************************************************************************/
struct Switches {
    bool        sstep   : 1;
    bool        sinstr  : 1;

    Switches() : sstep{false}, sinstr{false} {};
};

/************************************************************************************************
 * Processor State
 ************************************************************************************************/

static bool        	run 		= false;
static Switches    	sw;
static Registers	r;
static State       	s			= State::Fetch;
static unsigned    	mem[4096];
static unsigned		ncycles 	= 0;
static unsigned		ninstr		= 0;
    
/********************************************************************************************//**
 ************************************************************************************************/
static void ral() {
	const uint16_t prevL = r.l;
	r.l = r.ac >> 11;
	r.ac <<= 1;
	r.ac |= prevL;
}

/********************************************************************************************//**
 ************************************************************************************************/
static void rar() {
	const uint16_t prevL = r.l;
	r.l = r.ac & 1;
	r.ac >>= 1;
	r.ac |= prevL << 11;
}

/********************************************************************************************//**
 * OPeRate - Group 1
 ************************************************************************************************/
static void oper_group1(unsigned instr) {
	if (instr == GRP1_NOP)
		return;

	else {
		// Sequence 1

		if ((instr & GRP1_CLA) == GRP1_CLA)	r.ac = 0;
		if ((instr & GRP1_CLL) == GRP1_CLL)	r.l = 0;

		// Sequence ?

		if ((instr & GRP1_CMA) == GRP1_CMA)	r.ac = ~r.ac;
		if ((instr & GRP1_CML) == GRP1_CML)	r.l = ~r.l;
		
		// Sequence ?
		
		if ((instr & GRP1_IAC) == GRP1_IAC)	++r.ac;

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
			if ((r.ac & Sign_Mask) != Sign_Mask)
				++r.pc;
		}

		if ((instr & GRP2_SNA) == GRP2_SNA) {
			condition = true;
			if (r.ac != 0)
				++r.pc;
		}

		if ((instr & GRP2_SZL) == GRP2_SZL) {
			condition = true;
			if (r.l == 0)
				++r.pc;
		}

		if ((instr & GRP2_SKP) == GRP2_SKP && !condition)
			++r.pc;

	} else {							// SKP bit is 0
		if ((instr & GRP2_SMA) == GRP2_SMA)
			if ((r.ac & Sign_Mask) == Sign_Mask)
				++r.pc;

		if ((instr & GRP2_SZA) == GRP2_SZA)
			if (r.ac == 0)
				++r.pc;

		if ((instr & GRP2_SNL) == GRP2_SNL)
			if (r.l != 0)
				++r.pc;
	}
	
	// Sequence 2

	if ((instr & GRP2_CLA) == GRP2_CLA)		r.ac = 0;

	// Sequence 3

	if ((instr & GRP2_OSR) == GRP2_OSR)		r.ac |= r.sr;
	if ((instr & GRP2_HLT) == GRP2_HLT)		run = false;
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
 * Decode a instruction
 ************************************************************************************************/
static Decoded decode(unsigned instr) {
	Decoded d;
	d.op		= static_cast<OpCode>((instr & Op_Mask)	>> Op_Shift);
	d.i		= (instr & I_Mask) == I_Mask;
	d.p		= (instr & P_Mask) == P_Mask;
	unsigned	addr	= instr & Addr_Mask;

	d.eaddr	= d.p ? r.pc & Page_Mask : 0;
	d.eaddr |= addr;
	d.bits = instr & 00777;

	return d;
}

/********************************************************************************************//**
 * Fetch next instruction, handle JMP direct
 ************************************************************************************************/
void fetch() {
	++ninstr;

	r.md 			= mem[r.pc++];
	const Decoded d	= decode(r.md);
	r.ir 			= d.op;
	r.ma 			= d.eaddr;

    if (r.ir == OpCode::IOT) {			// IOT?
		assert(false);					// ... not implemented!
        s = State::Fetch;

	} else if (r.ir == OpCode::OPR) {		// OPR?
		oper(r.md);
		s = State::Fetch;

    } else if (d.i)                  		// Indirect?
       s = State::Defer;				//	r.ma is the address of the operation
	   
    else if (r.ir == OpCode::JMP) {		// JMP direct?
        r.pc = r.ma;
        s = State::Fetch;

    } else
       s = State::Execute;
}

/********************************************************************************************//**
 * Defer state
 ************************************************************************************************/
void defer() {
	r.md = mem[r.ma];					// Fetch indirect operand
	if (r.ma >= 010 && r.ma <= 017)
		mem[r.ma] = ++r.md;				// Auto increment

	if (r.ir == OpCode::JMP) {			// JMP indirect?
		r.pc = r.md;
		s = State::Fetch;

	} else
		s = State::Execute;
}

/********************************************************************************************//**
 * Execute state
 ************************************************************************************************/
void execute() {
    r.md = mem[r.ma];

    switch(r.ir) {
    case OpCode::AND:
    	r.ac &= r.md;
        break;

	case OpCode::TAD: {
		uint16_t sum = r.ac + r.md;
		if (sum & ~UINT12_MAX)
			r.l = ~r.l;
		r.ac = sum & UINT12_MAX;
	} break;
			
	case OpCode::ISZ:
		mem[r.ma] = ++r.md;
		if (r.md == 0) 
			++r.pc;
		break;

    case OpCode::DCA:
		mem[r.ma] = r.ac;
		r.ac = 0;
		break;

    case OpCode::JMS:
		mem[r.ma] = r.pc;
		r.pc = r.ma++;
		break;

    case OpCode::JMP:		// Not expected in this state!
		assert(false);
		break;

    case OpCode::IOT:
		assert(false);		// Not expected in this state!
		break;

    case OpCode::OPR:		// Not expected in this state!
		assert(false);
		break;
    }

    s = State::Fetch;
}

/********************************************************************************************//**
 * Break (DMA) state
 ************************************************************************************************/
void brk() {
	assert(false);			// Not implemented
    s = State::Fetch;
}

/********************************************************************************************//**
 ************************************************************************************************/
static void disasm_opr(unsigned instr) {
	cout << oct;

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
			bool condition = false;						// true for unconditional SKP

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
		assert(false);									// Other groups are not supported... yet.
}

/********************************************************************************************//**
 ************************************************************************************************/
static void disasm_iot(unsigned instr) {
	cout	<< "IOT ";
	const unsigned dev	= (instr & IOT_DEV_SEL) >> IOT_DEV_SHIFT;
	const unsigned ops	= instr & IOT_OP;
	cout << setw(3)	<< dev  << ' '
		 << setw(1)  << ops	<< "\n";
}

/********************************************************************************************//**
 ************************************************************************************************/
static void disasm_mri(const Decoded& d) {
	cout 	<< d.op	<< ' ';
	if (d.i)
		cout << "I ";
	cout	<< setw(4)	<< d.eaddr
			<< " (" << setw(4) << mem[d.eaddr] << ')';
}


/********************************************************************************************//**
 * disasmble the the next instruction
 ************************************************************************************************/
static void disasm(unsigned addr, unsigned instr) {
	const Decoded d = decode(instr);

	cout	<< oct << setfill('0');
	cout	<< setw(4) 	<< addr 		<< ' '
			<< setw(4)	<< mem[addr]	<< ' ';
	switch (d.op) {
		case OpCode::OPR:	disasm_opr(instr);		break;
		case OpCode::IOT: 	disasm_iot(instr);		break;
		default:			disasm_mri(d);
	}
}

/********************************************************************************************//**
 * Dump the processor state
 ************************************************************************************************/
static void dump() {
	const double	us = ncycles * 1.5;			// Not accurate, IOT takes 4.5us!

	cout << oct << setfill('0');

    cout
			<< "PC "	<< setw(4)	<< r.pc		<< ' '
		 	<< "L "					<< r.l 		<< ' '
			<< "AC "	<< setw(4)	<< r.ac		<< '\n' 

    		<< "MA "	<< setw(4)	<< r.ma		<< "     "
           	<< "MD "	<< setw(4)	<< r.md		<< ' '
           	<< "SR "	<< setw(4)	<< r.sr		<< '\n'

			<< "IR "				<< r.ir		<< ' '
									<< s		<< ' '
			<< setfill(' ')
						<< setw(4)	<< ninstr	<< " instrs "
						<< setw(4)	<< ncycles	<< " cycles "
			<< '(' 					<< us 		<< " us)\n";

	if (s == State::Fetch) {
		disasm(r.pc, mem[r.pc]);
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
			r.sr = i;

	} catch (std::invalid_argument const& ex) {
        return false;							// Ignore, not a "digit"

   	} catch (std::out_of_range const& ex) {
        std::cout << "std::out_of_range::what(): " << ex.what() << '\n';
   	}	

	return true;
}

/********************************************************************************************//**
 * @return true to exit the simulator
 ************************************************************************************************/
static bool frontpanel() {
	dump();
    cout << "> ";
 
    string cmd = "";
    if (!getline(cin, cmd))
    	return true;			// Ctrl-D - exit

//	cerr << "read: '" << cmd << "'\n";		// For testing only...

	else if (cmd == "" || cmd == "c" || cmd == "cont")	run = true;
	else if (cmd == "?" || cmd == "h" || cmd == "help") {
		cout	<< "number      -- Set Sr\n"
				<< "?|h[elp]    -- Print help\n"
				<< "c[ont]      -- Continue\n"
				<< "la          -- Load Address\n"
				<< "ldaddr      -- Load Address\n"
				<< "[no]sinstr  -- Single Instruction\n"
				<< "[no]sstep   -- Single Step\n"
				<< "s[tart]     -- Start\n"
				<< "q[uit]      -- Exit\n"
				<< "<return>    -- Same as cont\n"
				<< "<ctrl-d>    -- Same as q[uit]\n";

	}
	else if (cmd == "nosinstr")						    sw.sinstr = false;
	else if (cmd == "nosstep")							sw.sstep = false;
	else if (cmd == "sinstr")							sw.sinstr = true;
	else if (cmd == "sstep")							sw.sstep = true;
	else if (cmd == "s" || cmd == "start") {
		r.l				= false;
		r.ac = r.md 	= 0;
		r.ma 			= r.pc;
		s 				= State::Fetch;
		run 			= true;
	}
	else if (cmd == "q" || cmd == "quit")				return true;
	else if (cmd == "la" || cmd == "ldaddr")			r.pc = r.sr;
	else if (digit(cmd))
		;
    else 
		cerr << "Unknown command: '" << cmd << "!\n";
        
	return false;
}

/********************************************************************************************//**
 * Run the processor/debugger...
 ************************************************************************************************/
int process() {
	run = false;					// Processor starts in idle mode...
    for (;;) {
        while (run) {
            do {					// Next instruction (mem[r.pc])
                do {				// Next memory state
                    switch(s) {
                    case State::Fetch:      fetch();    break;
                    case State::Defer:      defer();    break;
                    case State::Execute:    execute();  break;
                    case State::Break:      brk();      break;
                    default: cerr << "unknown state!\n";
                    }

					++ncycles;

                } while (run && !sw.sstep && s != State::Fetch);
            } while (run && !sw.sinstr && !sw.sstep);

            run = run && !sw.sstep && !sw.sinstr;
        }

		while (!run) {
			if (frontpanel())
				return 0;
		}
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
	BIN_State	s = BIN_State::DataMSB;

	while (ifs.get(c)) {
		byte = static_cast<uint8_t>(c);

		if (byte == BIN_LEADER)
			continue;

		if ((byte & BIN_ORG_Mask) == BIN_ORG_Mask)
			s = BIN_State::OriginMSB;

		switch (s) {
			case BIN_State::Leader:
				assert(false);
				break;

			case BIN_State::OriginMSB:
				r.pc = (byte & BIN_DATA_Mask) << BIN_MSB_Shift;
				s = BIN_State::OriginLSB;
				break;

			case BIN_State::OriginLSB:
				r.pc |= byte;
				s = BIN_State::DataMSB;
				break;

			case BIN_State::DataMSB:
				data = (byte & BIN_DATA_Mask) << BIN_MSB_Shift;
				s = BIN_State::DataLSB;
				break;

			case BIN_State::DataLSB:
				data |= byte;
				mem[r.pc] = data;
				mem[++r.pc] = data;
				s = BIN_State::DataMSB;
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
					case 'v': cout << "version 0.2\n";	return 1;
					default:
						cerr << progName << ": unknown option '" << c << "'.\n";
						return 1;
				}
			}

			
		} else if (!load_BIN(arg))
			return 1;
	}

	return process();
}

