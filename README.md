# pdp8sim - Yet Another PDP-8 Simulator

Version 0.7 pre-release

A PDP-8 simulator, just for the heck of it!

Primally based on the PDP-8 Straight Eight,
describe3d in DEC's 1967 Small Computer Handbook, "PDP-8 Users Handbook"

## References

* [PALBART](https://www.pdp8online.com/ftp/software/palbart/palbart.c)
* Introduction to Programming (DEC), Vol 1, 2nd Edition (1970)
* Small Computer Handbook, 1st Edition (?), 1967
* PDP-8 Maintenon Manual, (DEC), 1968

## Examples

A number of example programs are available in the examples sub directory. Contents:

Extension | Description
--------- | ------------
\*.pa     | Assembly source.
\*.lst    | Assembly listing,
\*.err    | Error files.
\*.bin    | Output files in BIN tape format

All are, sometimes slightly modified, from DEC's Introduction to
Programming, Volume 1, 2nd Edition (1970), Elementary Programming Techniques. 

Note that [PALBART](https://www.pdp8online.com/ftp/software/palbart/palbart.c) was used as the MACRO-8 equipment assembler.

## Status

### Bugs fixed

* OPR Group 1 and 2, and MRI instructions tested.

### General 

* IOT, interrupts and break are not supported!
* Can load BIN files from the command line. Maybe add support for RIM format?
  Auto loading of RIM and, or BIN loaders?
* No devices... yet! Current thinking is to model each device as a independent
  thread. How the FrontPanel (debug prompt) sharing the console (standard
  input/output) is still unclear.
* Currently run time is # of cycles * 1.5us. However, that won't work for IOT
  that cause a pause, extending Fetch to 3.75 us (2.5 machine cycles). The end
  result in a IOT time of 4.5us. See PDP-8 Maintenance Manual, Input/Output
  Transfer (IOT), pg 2-15.

### Future

 * Trace command - maybe replace bool run with enum class State { Idle, Run, Trace };?
 * Improved "front-pannel", e.g, "la 0200"?
 * Breakpoints?

### Testing

Currently ah-hoc running of the example programs.

## Architecture

### PDP-8/I Major State Flow Diagram

The following diagram, derived from the PDP-8/E Maintenance Manual,
Figure 3-4 PDP-8/E Major State Flow Diagram,
was the primary reference for cycle-realistic operation of the simulator.


      |<---- 1 memory cycle ----->|<---- 1 memory cycle ---->|<---- 1 memory cycle ---->|
    
         MRI Direct/Indirect
       |<--------------------------------------------------------------------------+
       | JMP Indirect                                                              |
       |<---------------------------------------------+                            |
       | JMP Direct                                   |                            |
       |<-----------------+                           |                            |
       \       +-------+  |                +-------+  |               +---------+  |
        \      |       |--+  MRI Indirect  |       |--+               |         |  |
    +--->----->| Fetch |------------------>| Defer |----------------->| Execute |--+
    |   /      |       |----------------+  |       |                  |         |
    |  /       +-------+--+             |  +-------+              +-->+---------+
    |  |  IOT & Operate   |             |        MRI Direct       |
    |  +------------------+             +-------------------------+
    |
    | |<---- MD = Instruction --->|<-----_ MD = Address ----->|<---- MD = Operand  ---->|
    |
    |                                      +--------+
    |     Start up and Manual Operation -->| Direct |
    |                                      | Memory |----+
    |                   Data Break ------->| Access |    |
    |                                      +--------+    |
    |             Start up & Man Op Only                 |
    +----------------------------------------------------+

### Registers

* AC - ACcumulator (12-bits)
*  L - Link (1-bit)
* PC - Program Counter (12-bits)
* MD - Memmory Data (12-bits)
* MA - Memory Address (12-bits)
* SR - Switch Register (12-bits)

### Instruction Formats

The following diagrams where derived from the PDP-8/E Maintenance Manual,
Figure 1-3, Instruction Formats.

#### Memory Reference Instruction (MRI) Format

|   0:2    | 3 | 4 |         5:11          |
| :------: | - | - | :-------------------: |
|  OpCode  | I | P |        Address			  

* OpCode 0:5
* I = Direct (0), Indirect (1)
* P = Page zero (0), Current page (1)

#### IOT Instruction Format

|  0:2  |  5:8   |   9  |  10  |  11  |
| :---: | :----: | :--: | :--: | :--: |
|   6   | Device | IOP4 | IOP2 | IOP1 |

* IOP4 - Generates and IOP 4 pulse at event time 3, if 1
* IOP2 - Generates and IOP 2 pulse at event time 2, if 1
* IOP1 - Generates and IOP 1 pulse at event time 1, if 1

#### Group 1 Operate "Microinstruction" Format

|  0:2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  10 |  11 |
| :---: | --- | --- | --- | --- | --- | --- | --- | --- | --- |
|   6   |  0  | CLA | CLL | CMA | CML | RAR | RAL |  P  | IAC |

* P - Rotate 1 positions if 0, 2 positions if 1
* No operation if bits 4:11 are zero.

#### Group 2 Operate "Microinstruction" Format

|  0:2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  10 |  11 |
| :---: | --- | --- | --- | --- | --- | --- | --- | --- | --- |
|   6   |  1  | CLA | SMA | SZA | SNL |  R  | OSR |  HLT |  0  |

* R - Reverse skip sensing of bits 5, 6 and 7
