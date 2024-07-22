## pdp8sim - a PDP-8 Simulator

A PDP-8 simulator, just for the sake of it.
Version 0.1, pre-release.

Current status:

* IOT is not supported
* OPR Group 1 and 2 under test
* Can load BIN files from the command line.

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

