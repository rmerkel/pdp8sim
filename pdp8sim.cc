/********************************************************************************************//**
 * @file pdp8sim.cc
 * 
 * A PDP-8 Simulator
 ************************************************************************************************/

#include <cstdint>
#include <iomanip>
#include <ios>
#include <iostream>
#include <string>

using namespace std;

/********************************************************************************************//**
 * Operation codes for basic instructions
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

/************************************************************************************************
 ************************************************************************************************/

const unsigned Page_Mask    = 07600;		///< PC Address Page address mask

/********************************************************************************************//**
 * Instruction Register
 *
 * @warning Assumes bits are assigned left to right
 ************************************************************************************************/
struct IR {
    union {
        uint16_t          u;				///< As an unsigned
        struct {
			uint16_t			: 4;		// Padding
            OpCode        op    : 3;		///< OpCode
            uint16_t       i    : 1;		///< Indirect bit; 0 = Direct, 1 = Indirect
            uint16_t       p    : 1;		///< Page bit; 0 = Page zero, 1 = Current page
            uint16_t    addr    : 7;		///< Address; page offset
        };
    };

    IR() : u{0} {}							///< Defaults to AND 0
};

/********************************************************************************************//**
 * PDP8 Registers
 ************************************************************************************************/
struct registers {
    uint16_t    pc      : 12;       // may expand to include df and if
    uint16_t    ac      : 12;
    uint16_t     l      :  1;
    uint16_t    ma      : 12;       // need support for signed values?
    uint16_t    md      : 12;
    IR          ir;

    registers() : pc{0}, ac{0}, l{0}, ma{0}, md{0}  {}
};

/********************************************************************************************//**
 * Front Panel switches
 ************************************************************************************************/
struct switches {
    bool        sstep   : 1;
    bool        sinstr  : 1;

    switches() : sstep{true}, sinstr{true} {};      // defalt to true for now...
};

/********************************************************************************************//**
 * Major Memory states
 ************************************************************************************************/
enum class State { Fetch, Defer, Execute, Break };

/************************************************************************************************
 * Processor State
 ************************************************************************************************/

static bool        run 			= false;
static switches    sw;
static registers   r;
static State       s			= State::Fetch;
static unsigned    mem[4096];
    
/********************************************************************************************//**
 * Return a Cyce as a string
 ************************************************************************************************/
static const char* state(const State state) {
    switch (state) {
        case State::Fetch:      return "Fetch";
        case State::Defer:      return "Defer";
        case State::Execute:    return "Execute";
        case State::Break:      return "Break";
        default:                return "Unknown State!";
    }
}

/********************************************************************************************//**
 * Return an OpCode as a string
 ************************************************************************************************/
static const char* opcode(const OpCode op) {
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
 * Dump the processor state
 ************************************************************************************************/
static void dump(const registers& r) {
    cout    << state(s)  << ' ' << opcode(r.ir.op)                      << '\n';

    cout    << "PC: " << oct << setfill('0') << setw(4) << r.pc         << ' '
            <<  "L: "                                   << r.l          << ' '
            << "AC: " << oct << setfill('0') << setw(4) << r.ac         << '\n';

    cout    << "MA: " << oct << setfill('0') << setw(4) << r.ma         << ' '
            << "MD: " << oct << setfill('0') << setw(4) << r.md         << ' '  
            << "IR: " << oct << setfill('0') << setw(4) << r.ir.u       << '\n';
}

/********************************************************************************************//**
 * Fetch State
 ************************************************************************************************/
void fetch() {
    r.ir.u = mem[r.pc++];
    s = State::Execute;

    if (r.ir.p)
        r.ma = r.pc & Page_Mask;
     else
        r.ma = 0;
    r.ma += r.ir.addr;

    if (r.ir.op == OpCode::IOT) {
        cout << "IOT";
        s = State::Fetch;

    } else if (r.ir.i)                  // ma is address of the operand
       s = State::Defer;

    else if (r.ir.op == OpCode::JMP) {
        cout << "JMP";
        r.pc = r.ma;
        s = State::Defer;

    } else
       s = State::Execute;
}

/********************************************************************************************//**
 * Defer state
 ************************************************************************************************/
void defer() {
    s = State::Fetch;
}

/********************************************************************************************//**
 * Execute state
 ************************************************************************************************/
void execute() {
    r.md = mem[r.ma];
    switch(r.ir.op) {
        case OpCode::AND:
            r.ac &= r.md;
            break;

        case OpCode::TAD:   break;
        case OpCode::ISZ:   break;
        case OpCode::DCA:   break;
        case OpCode::JMS:   break;
        case OpCode::JMP:   break;
        case OpCode::IOT:   break;
        case OpCode::OPR:   break;
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
 * The PDP8 simulator
 ************************************************************************************************/
int main() {
    for (;;) {
        if (run) {
            do {
                do {
                    switch(s) {
                        case State::Fetch:      fetch();    break;
                        case State::Defer:      defer();    break;
                        case State::Execute:    execute();  break;
                        case State::Break:      brk();      break;
                        default: cerr << "unknown state!\n";
                    }
                } while (!sw.sstep);
            } while (!sw.sinstr);

            run = !sw.sstep && !sw.sinstr;

        } else {
            dump(r);
            cout << "> ";
 
            string cmd;
            if (!getline(cin, cmd))
                return 0;

            run = true;
        }
    }

    return 0;
}
