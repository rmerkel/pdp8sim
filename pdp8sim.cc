/********************************************************************************************//**
 * @file pdp8sim.cc
 * 
 * A PDP-8 Simulator
 ************************************************************************************************/

#include <cassert>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <iostream>
#include <string>

using namespace std;

/********************************************************************************************//**
 * Operation codes for the basic instructions
 ************************************************************************************************/
enum class OpCode {
    AND = 0,
    TAD = 1,
    ISZ = 2,
    DCA = 3,
    JMS = 4,
    JMP = 5,
    IOT = 6,
    OPR = 7
};

/********************************************************************************************//**
 * Return op as a string
 ************************************************************************************************/
static const char* asString(OpCode op) {
    switch (op) {
    case OpCode::AND:   return "AND";
    case OpCode::TAD:   return "TAD";
    case OpCode::ISZ:   return "ISZ";
    case OpCode::DCA:   return "DCA";
    case OpCode::JMS:   return "JMS";
    case OpCode::JMP:   return "JMP";
    case OpCode::IOT:   return "IOT";
    case OpCode::OPR:   return "OPR";
    default:            return "Undefined OpCode!";
    }
}

/********************************************************************************************//**
 * Major Memory states
 ************************************************************************************************/
enum class State {
	Fetch,
	Defer,
	Execute,
	Break
};

/********************************************************************************************//**
 * Return a state as a string
 ************************************************************************************************/
static const char* asString(const State state) {
    switch (state) {
    case State::Fetch:      return "Fetch";
    case State::Defer:      return "Defer";
    case State::Execute:    return "Execute";
    case State::Break:      return "Break";
    default:                return "Unknown State!";
    }
}

/************************************************************************************************
 * Constants
 ************************************************************************************************/

const int	 UINT12_MAX		= +2047;
const int	 UINT12_MIN		= -2048;

/************************************************************************************************
 * Bit maskes
 ************************************************************************************************/

const unsigned Page_Mask    = 07600;		///< PC Address Page address mask
const unsigned Op_Mask		= 07000;		///< OpCode mask
const unsigned Op_Shift		= 9;			///< OpCode shift
const unsigned I_Mask		= 00400;		///< Indirect bit
const unsigned P_Mask		= 00200;		///< Page bit
const unsigned Addr_Mask	= 00177;		///< Address/page offset mask

/********************************************************************************************//**
 * PDP8 Registers
 ************************************************************************************************/
struct Registers {
    uint16_t    pc      : 12;       // Program Counter - may expand to include df and if
    uint16_t    ac      : 12;		// ACcumulator register
    uint16_t     l      :  1;		// Link register
    uint16_t    ma      : 12;       // Memory address register
    uint16_t    md      : 12;		// Memory data register
	uint16_t	sr		: 12;		// Switch register
	OpCode		ir;

    Registers() : pc{0}, ac{0}, l{0}, ma{0}, md{0}, sr{0}, ir{OpCode::AND}  {}
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
static unsigned		ninstrs		= 0;
    
/********************************************************************************************//**
 * Fetch next instruction, handle JMP direct
 ************************************************************************************************/
void fetch() {
	r.md 			= mem[r.pc++];
	r.ir 			= static_cast<OpCode>((r.md & Op_Mask)	>> Op_Shift);
	const bool i	= (r.md & I_Mask) == I_Mask;
	const bool p	= (r.md & P_Mask) == P_Mask;
	unsigned addr	= r.md & Addr_Mask;

	r.ma = p ? r.pc & Page_Mask : 0;
	r.ma |= addr;

    if (r.ir == OpCode::IOT) {			// IOT?
		assert(false);					// IOT not implemented
        s = State::Fetch;

	} else if (r.ir == OpCode::OPR) {	// OPR?
		assert(false);					// OPR not implementated
		s = State::Fetch;

    } else if (i)                  		// ma is address of the operand?
       s = State::Defer;

    else if (r.ir == OpCode::JMP) {		// JMP direct?
        r.pc = r.ma;
        s = State::Defer;

    } else
       s = State::Execute;
}

/********************************************************************************************//**
 * Defer state
 ************************************************************************************************/
void defer() {
	r.md = mem[r.ma];					// Fetch indirect operand
	if (r.ma >= 010 && r.ma <= 017) {
		mem[r.ma] = ++r.md;				// Auto increment
	}

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

	case OpCode::TAD:
		if (r.ac > UINT12_MAX - r.md)
			r.l = !r.l;
		r.ac += r.md;
		break;
			
	case OpCode::ISZ:
		assert(false);		// Not implementated
		break;

    case OpCode::DCA:
		assert(false);		// Not implementated
		break;

    case OpCode::JMS:
		assert(false);		// Not implementated
		break;

    case OpCode::JMP:		// Not expected in this state!
		assert(false);
		break;

    case OpCode::IOT:
		assert(false);		// Not implementated
		break;

    case OpCode::OPR:
		assert(false);		// Not implementated
		break;
    }

    s = State::Fetch;
}

/********************************************************************************************//**
 * Break (DMA) state
 ************************************************************************************************/
void brk() {
    s = State::Fetch;
}

/********************************************************************************************//**
 * Dump the processor state
 ************************************************************************************************/
static void dump() {
	// Processor state, switches, disassemble
	cout	<< asString(r.ir) << ' ' << asString(s);
	if (sw.sinstr)
		cout << " SInstr";
	if (sw.sstep)
		cout << " SStep";
	cout << '\n';
	
	// registers
    cout    << "PC " << oct << setfill('0') << setw(4) << r.pc  << ' '
			<<  "L "                                   << r.l   << ' '
            << "AC " << oct << setfill('0') << setw(4) << r.ac  << ' '
    		<< "MA " << oct << setfill('0') << setw(4) << r.ma  << ' '
            << "MD " << oct << setfill('0') << setw(4) << r.md  << ' '  
    		<< "SR " << oct << setfill('0') << setw(4) << r.sr  << '\n';

	// counters
	cout << "# instrs " << ninstrs << " # cycles " << ncycles << " (" << ncycles * 1.5 << "us)\n";
}

/********************************************************************************************//**
 * @return true if s is a number
 ************************************************************************************************/
static bool digit(const string& s) {
	try {
		const int i{std::stoi(s)};

		if (i > UINT12_MAX || i < UINT12_MIN)
			cerr << "'" << i << "i is out of range " << UINT12_MAX << " .. " << UINT12_MIN << '\n';

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
				<< "q[uit]      -- Exit\n"
				<< "[no]sinstr  -- Single Step\n"
				<< "[no]sstep   -- Single Instruction\n"
				<< "s[tart]     -- Start\n"
				<< "<return>    -- Same as cont\n"
				<< "<ctrl-d>    -- Same as q[uit]\n";

	}
	else if (cmd == "nosinstr")						    sw.sinstr = false;
	else if (cmd == "nosstep")							sw.sstep = false;
	else if (cmd == "q" || cmd == "quit")				return true;
	else if (cmd == "sinstr")							sw.sinstr = true;
	else if (cmd == "sstep")							sw.sstep = true;
	else if (cmd == "s" || cmd == "start") {
		r.l				= false;
		r.ac = r.md 	= 0;
		s 				= State::Fetch;
		run 			= true;
		
	}
	else if (digit(cmd))
		;
    else 
		cerr << "unknown command: '" << cmd << "'\n";
        
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

                } while (!sw.sstep && s != State::Fetch);

				if (s == State::Fetch)
					++ninstrs;

            } while (!sw.sinstr && !sw.sstep);

            run = !sw.sstep && !sw.sinstr;
        }

		while (!run) {
			if (frontpanel())
				return 0;
		}
    }
}

/********************************************************************************************//**
 * The PDP8 simulator
 ************************************************************************************************/
int main() {
	mem[00000] = 05410;	// JMP I 00010
	mem[00001] = 05410;	// JMP I 00010
	mem[00002] = 05410;	// JMP I 00010
	mem[00003] = 05410;	// JMP I 00010
	mem[00004] = 05410;	// JMP I 00010
	mem[00005] = 05410;	// JMP I 00010
	mem[00006] = 05410;	// JMP I 00010
	mem[00007] = 05410;	// JMP I 00010

	return process();
}
