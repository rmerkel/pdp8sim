# pdp8sim
A PDP-8 Simulator

# PDP-8/I Major State Flow Diagram

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

