# Architecture Notes

Derived from the various PDP-8 sources,
including but not limited to

* pdp8/i Reference card
* Introduction to Programming (DEC), Vol 1, 2nd Edition (1970)
* Small Computer Handbook, 1st Edition (?), 1967
* PDP-8 Maintaince Manual, (DEC), 1968

## PDP-8/I Major State Flow Diagram

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

## Registers

* AC - ACcumulator (12-bits)
*  L - Link (1-bit)
* PC - Program Counter (12-bits)
* MD - Memmory Data (12-bits)
* MA - Memory Address (12-bits)
* SR - Switch Register (12-bits)

## Instruction Formats

The following diagrams where derived from the PDP-8/E Maintenance Manual,
Figure 1-3, Instruction Formats.

### Memory Reference Instruction (MRI) Format

|   0:2    | 3 | 4 |         5:11          |
| :------: | - | - | :-------------------: |
|  OpCode  | I | P |        Address			  

* OpCode 0:5
* I = Direct (0), Indirect (1)
* P = Page zero (0), Current page (1)

### IOT Instruction Format

|  0:2  |  5:8   |   9  |  10  |  11  |
| :---: | :----: | :--: | :--: | :--: |
|   6   | Device | IOP4 | IOP2 | IOP1 |

* IOP4 - Generates and IOP 4 pulse at event time 3, if 1
* IOP2 - Generates and IOP 2 pulse at event time 2, if 1
* IOP1 - Generates and IOP 1 pulse at event time 1, if 1

### Group 1 Operate "Microinstruction" Format

|  0:2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  10 |  11 |
| :---: | --- | --- | --- | --- | --- | --- | --- | --- | --- |
|   6   |  0  | CLA | CLL | CMA | CML | RAR | RAL |  P  | IAC |

* P - Rotate 1 positions if 0, 2 positions if 1
* No operation if bits 4:11 are zero.

### Group 2 Operate "Microinstruction" Format

|  0:2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  10 |  11 |
| :---: | --- | --- | --- | --- | --- | --- | --- | --- | --- |
|   6   |  1  | CLA | SMA | SZA | SNL |  R  | OSR |  HLT |  0  |

* R - Reverse skip sensing of bits 5, 6 and 7

