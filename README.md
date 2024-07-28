## pdp8sim - a PDP-8 Simulator

A PDP-8 simulator, just for the heck of it!
Version 0.2, pre-release.

Current status:

* IOT is not supported
* OPR Group 1 and 2 under test
* MRI under test.
* Break cycles not supported.
* Can load BIN files from the command line.
* No devices... yet! Current thinking is to model each device as a independent
  thread. How the FrontPanel (debug prompt) share the console (standard
  input/output) is still unclear.
* Currently run time is # of cycles * 1.5us. However, that won't work for IOT
  as the pdp8/i programming card give 4.5us but for a single cycle (see
  below).

Testing:

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

