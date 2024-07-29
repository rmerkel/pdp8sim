# pdp8sim - A PDP-8 Simulator

A PDP-8 simulator, just for the heck of it!
Version 0.2, pre-release.

## Current Status

* IOT and interrupts are not supported!
* Break is not supported!
* OPR Group 1 and 2, and MRI instructions are under test.
* JMS Fixed.
* Can load BIN files from the command line. Maybe add support for RIM format?
  Auto loading of RIM and, or BIN loaders?
* No external devices... yet! Current thinking is to model each device as a
  independent thread. How the FrontPanel (debug prompt) sharing the console
  (standard input/output) is still unclear.
* Currently run time is # of cycles * 1.5us. However, that won't work for IOT
  that cause a pause, extending Fetch to 3.75 us (2.5 machine cycles). The end
  result in a IOT time of 4.5us. See PDP8 Maintance Manual, Input/Output
  Transfer (IOT), pg 2-15.

## Testing

Currently ah-hoc running of the sample programs.

## PDP-8/I Major State Flow Diagram

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

