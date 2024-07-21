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
static const char* toString(OpCode op) {
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
static const char* toString(const State state) {
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

const unsigned UINT12_MAX	= 07777;
const int	   INT12_MAX	= +2047;
const int	   INT12_MIN	= -2048;

/************************************************************************************************
 * Bit maskes
 ************************************************************************************************/

const unsigned Page_Mask    = 07600;		///< PC Address Page address mask
const unsigned Op_Mask		= 07000;		///< OpCode mask
const unsigned Op_Shift		= 9;			///< OpCode shift
const unsigned I_Mask		= 00400;		///< Indirect bit
const unsigned P_Mask		= 00200;		///< Page bit
const unsigned Addr_Mask	= 00177;		///< Address/page offset mask

// OPER Group 1

const unsigned GROUP1		= 00400;		///< Bit 3 is clear

const unsigned GRP1_NOP		= 07000;		///< NOP
const unsigned GRP1_CLA		= 07200;		///< Clear AC,	sequence 1
const unsigned GRP1_CLL		= 07100;		///< Clear link, sequence 1
const unsigned GRP1_CMA		= 07040;		///< Complement AC, sequence 2
const unsigned GRP1_CML		= 07020;		///< Complement link, sequence 2
const unsigned GRP1_RAR		= 07010;		///< Rotate AC and L right 1, sequence 4
const unsigned GRP1_RTR		= 07012;		///< Rotate AC and L right 2, sequence 4
const unsigned GRP1_RTL		= 07006;		///< Rotate AC and L left 1, sequence 4
const unsigned GRP1_RAL		= 07004;		///< Rotate AC and L left 2, sequence 4
const unsigned GRP1_IAC		= 07001;		///< Increment AC, sequence 3

// OPER Group 2

const unsigned GROUP2		= 00400;		///< Bit 3 is set, bit 11 is clear.

const unsigned	GRP2_SMA	= 07500;		///< Skip on minus AC, sequence 1
const unsigned	GRP2_SZA	= 07440;		///< Skip on zero AC, sequence 1
const unsigned	GRP2_SPA	= 07510;		///< Skip on plus AC, sequence 1
const unsigned 	GRP2_SNA	= 07450;		///< Skip on plus AC, sequence 1
const unsigned	GRP2_SNL	= 07420;		///< Skip on non-zero AC, sequence 1
const unsigned	GRP2_SZL	= 07430;		///< Skip on zero link, sequence 1
const unsigned	GRP2_SKP	= 07410;		///< Skip unconditionally, sequence 1
const unsigned	GRP2_OSR	= 07404;		///< Inclusive OR, switch register with AC, sequence 3
const unsigned	GRP2_HLT	= 07402;		///< Halt the processor, sequence 3
const unsigned	GRP2_CLA	= 07600;		///< Clear AC, sequence 2


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
 * Decoded instructon
 ************************************************************************************************/
struct Decoded {
	OpCode		op;					///< Opcode
	bool		i;					///< Indirect?
	bool		p;					///< Current page?
	uint16_t	eaddr;				///< Effective addr
	uint16_t	bits;				///< bits 3-11 of the instruction (for OPR, IOT, tbd)
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
static void oper_group1(unsigned instrs) {
	if (instrs == GRP1_NOP)
		return;

	else {
		// Sequence 1

		if ((instrs & GRP1_CLA) == GRP1_CLA)		
			r.ac = 0;

		if ((instrs & GRP1_CLL) == GRP1_CLL)
			r.l = 0;

		// Sequence ?

		if ((instrs & GRP1_CMA) == GRP1_CMA)
			r.ac = ~r.ac;

		if ((instrs & GRP1_CML) == GRP1_CML)
			r.l = ~r.l;
		
		// Sequence ?
		
		if ((instrs & GRP1_IAC) == GRP1_IAC)
			++r.ac;

		// Sequence ?

		if ((instrs & GRP1_RAR) == GRP1_RAR)
			rar();

		if ((instrs & GRP1_RTR) == GRP1_RTR)
			rar();

		if ((instrs & GRP1_RAL) == GRP1_RAL)
			ral();

		if ((instrs & GRP1_RTL) == GRP1_RTL)
			ral();
	}
}

/********************************************************************************************//**
 * OPeRate - Group 2
 ************************************************************************************************/
static void oper_group2(unsigned instrs) {
	// Sequence 1

	if ((instrs & GRP2_SMA) == GRP2_SMA)		
		if (r.ac < 0)	// NO,r.ac is an unsigned, check bit 0 (PDP-8 bit order)
			++r.pc;

	if ((instrs & GRP2_SZA) == GRP2_SZA)
		assert(false);

	// Sequence 2

	if ((instrs & GRP2_SPA) == GRP2_SPA)
		assert(false);

	if ((instrs & GRP2_SNA) == GRP2_SNA)
		assert(false);

	// Sequence 3

	if ((instrs & GRP2_SNL) == GRP2_SNL)
		assert(false);

	// Sequence 4

	if ((instrs & GRP2_SZL) == GRP2_SZL)
		assert(false);

	if ((instrs & GRP2_SKP) == GRP2_SKP)
		assert(false);

	if ((instrs & GRP2_OSR) == GRP2_OSR)
		assert(false);

	if ((instrs & GRP2_HLT) == GRP2_HLT)
		run = false;

	if ((instrs & GRP2_CLA) == GRP2_CLA)
		assert(false);
}

/********************************************************************************************//**
 * OPeRate
 ************************************************************************************************/
static void oper(unsigned instrs) {
	if (	 (instrs & GROUP1) == 0)
		oper_group1(instrs);

	else if ((instrs & GROUP2) == GROUP2)
		oper_group2(instrs);

	else
		assert(false);						// Other group are not implenentated
}

/********************************************************************************************//**
 * Fetch next instruction, handle JMP direct
 ************************************************************************************************/
void fetch() {
	r.md 					= mem[r.pc++];	// decode
	r.ir 					= static_cast<OpCode>((r.md & Op_Mask)	>> Op_Shift);
	const bool		i		= (r.md & I_Mask) == I_Mask;
	const bool		p		= (r.md & P_Mask) == P_Mask;
	const unsigned	addr	= r.md & Addr_Mask;

	++ninstrs;
	r.ma = p ? r.pc & Page_Mask : 0;
	r.ma |= addr;

    if (r.ir == OpCode::IOT) {			// IOT?
		assert(false);					// ... not implemented!
        s = State::Fetch;

	}
	else if (r.ir == OpCode::OPR) {	// OPR?
		oper(r.md);
		s = State::Fetch;

    }
	else if (i)                  		// Indirect?
       s = State::Defer;				//	r.ma is the address of the operation
	   
    else if (r.ir == OpCode::JMP) {		// JMP direct?
        r.pc = r.ma;
        s = State::Fetch;

    }
	else
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

	case OpCode::TAD: {
		uint16_t sum = r.ac + r.md;
		if (sum & UINT12_MAX)
			r.l = ~r.l;
		r.ac = sum & 07777;
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
    s = State::Fetch;
}

/********************************************************************************************//**
 * Dump the processor state
 *
 * @note	The state is the current state, after the previous instruction, but before the 
 *			current!
 *
 * @param	compact	true for compact display
 ************************************************************************************************/
static void dump(bool compact = false) {
	cout	<< ninstrs << " instrs "	<< ncycles << " cycles " << '(' << ncycles * 1.5 << "us)\n";

	if (compact) {
		cout	<< 	toString(r.ir) << ' ' << toString(s);
		if (sw.sinstr)
			cout << " SInstr";
		if (sw.sstep)
			cout << " SStep";
    	cout	<< 				   " PC "	<< oct << setfill('0') << setw(4) << r.pc 
    			<< 				   " MA "	<< oct << setfill('0') << setw(4) << r.ma  << ' '
            	<< 				   " MD "	<< oct << setfill('0') << setw(4) << r.md  << ' '
				<< "L " << r.l	<< " AC "	<< oct << setfill('0') << setw(4) << r.ac  << ' ' 
    			<< 				   " SR "	<< oct << setfill('0') << setw(4) << r.sr  << '\n';

	} else {

    	cout	<< 				   "    PC " << oct << setfill('0') << setw(4) << r.pc	<< ' '
				<< 	toString(r.ir) << ' ' << toString(s);
		if (sw.sinstr)
			cout << " SInstr";
		if (sw.sstep)
			cout << " SStep";
		cout	<< '\n';

    	cout	<< 				   "    MA " << oct << setfill('0') << setw(4) << r.ma  << '\n'
            	<< 				   "    MD " << oct << setfill('0') << setw(4) << r.md  << '\n'
				<< "L " << r.l	<<    " AC " << oct << setfill('0') << setw(4) << r.ac  << '\n' 
    			<< 				   "    SR " << oct << setfill('0') << setw(4) << r.sr  << '\n';
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
				<< "ldaddr		-- Load Address\n"
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

enum class BIN_State { Leader, OriginMSB, OriginLSB, DataMSB, DataLSB };
#if 0
static const char* toString(BIN_State s)
{
	switch(s) {
		case BIN_State::Leader:		return "Leader";	break;
		case BIN_State::OriginMSB:	return "OriginMSB";	break;
		case BIN_State::OriginLSB:	return "OriginLSB";	break;
		case BIN_State::DataMSB:	return "DataMSB";	break;
		case BIN_State::DataLSB:	return "DataLSB";	break;
		default:	assert(false);	return "Unknown";
	}
}
#endif

/********************************************************************************************//**
 * The PDP8 simulator
 ************************************************************************************************/
int main (int argc, char** argv) {
	const char* progname = argv[0];

	if (argc == 2) {
		const char* ifile = argv[1];

		ifstream ifs{ifile};
		if (!ifs) {
			cerr << progname << ": can't open '" << ifile << "'!\n";
			return 1;
		}

		const unsigned BIN_LEADER 		= 00200;
		const unsigned BIN_ORG_Mask		= 00100;
		const unsigned BIN_DATA_Mask	= 00077;
		const unsigned BIN_MSB_Shift	= 6;

		unsigned	loc = 0;
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

//			cout << "State: " << toString(s) << " read: " << oct << setfill('0') << setw(3) << static_cast<unsigned>(byte);

			switch (s) {
				case BIN_State::Leader:
					assert(false);
					break;

				case BIN_State::OriginMSB:
					loc = (byte & BIN_DATA_Mask) << BIN_MSB_Shift;
//					cout << " Origin (1st byte) " << oct << setfill('0') << setw(3) << loc;
					s = BIN_State::OriginLSB;
					break;

				case BIN_State::OriginLSB:
					loc |= byte;
//					cout << " Origin " << oct << setfill('0') << setw(3) << loc;
					s = BIN_State::DataMSB;
					break;

				case BIN_State::DataMSB:
					data = (byte & BIN_DATA_Mask) << BIN_MSB_Shift;
//					cout << " Data (1st byte) " << oct << setfill('0') << setw(3) << data;
					s = BIN_State::DataLSB;
					break;

				case BIN_State::DataLSB:
					data |= byte;
					mem[loc] = data;
//					cout << " mem[" << oct << setfill('0') << setw(4) << loc << "] = "
//						<< oct << setfill('0') << setw(4) << data << '\n';
					mem[++loc] = data;
					s = BIN_State::DataMSB;
					break;

				default:
					assert(false);
			}
//			cout << '\n';
		};

	} else {
		mem[00200] = 07300;		// tally.lst
		mem[00201] = 01210;
		mem[00202] = 07041;
		mem[00203] = 03212;
		mem[00204] = 01211;
		mem[00205] = 02212;
		mem[00206] = 05204;
		mem[00207] = 07402;
		mem[00210] = 00022;
		mem[00211] = 00044;
		mem[00212] = 00000;
	}

	return process();
}
