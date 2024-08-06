# pdp8sim - Yet Another PDP-8 Simulator

A PDP-8 simulator, just for the heck of it!
Version 0.6, pre-release.

Primally ostly based on the PDP-8 Straight Eight,
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

## Current Status

* JMS fixed!
* IOT and interrupts are not supported!
* Break is not supported!
* OPR Group 1 and 2, and MRI instructions are under test.
* Can load BIN files from the command line. Maybe add support for RIM format?
  Auto loading of RIM and, or BIN loaders?
* No external devices... yet! Current thinking is to model each device as a
  independent thread. How the FrontPanel (debug prompt) sharing the console
  (standard input/output) is still unclear.
* Currently run time is # of cycles * 1.5us. However, that won't work for IOT
  that cause a pause, extending Fetch to 3.75 us (2.5 machine cycles). The end
  result in a IOT time of 4.5us. See PDP-8 Maintenance Manual, Input/Output
  Transfer (IOT), pg 2-15.

## Future

 * Trace command - maybe replace bool run with enum class State { Idle, Run, Trace };?
 * Improved "front-pannel", e.g, "la 0200"?
 * Breakpoints?

## Testing

Currently ah-hoc running of the example programs.

## PDP-8/I Major State Flow Diagram

The following diagram, derived from the PDP-8/E Maintenance Manual,
was the primary reference for cycle-realistic operation of the simulator.


      |<---- 1 memory cycle ----->|<---- 1 memory cycle ---->|<---- 1 memory cycle ---->|
    
         MRI Direct/Indirect
       |<--------------------------------------------------------------------------+
       | JMP Indirect                                                              |
       |<---------------------------------------------+                            |
       | JMP Direct                                   |                            |
       |<----------------_+                           |                            |
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

