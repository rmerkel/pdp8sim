# pdp8sim - Yet Another PDP-8 Simulator

Version 0.9 pre-release

A PDP-8 simulator, just for the heck of it!

Primally based on the PDP-8 Straight Eight,
describe3d in DEC's 1967 Small Computer Handbook, "PDP-8 Users Handbook"

## References

* [PALBART](https://www.pdp8online.com/ftp/software/palbart/palbart.c)
* Introduction to Programming (DEC), Vol 1, 2nd Edition (1970)
* Small Computer Handbook, 1st Edition (?), 1967
* PDP-8 Maintaince Manual, (DEC), 1968

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

* Decode.cc builds, includes cstdint
* Ctrl-C halts the simulator.

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
* Ctrl-C will halt the simulator.

### Future

 * Trace command - maybe replace bool run with enum class State { Idle, Run, Trace };?
 * Improved "front-pannel", e.g, "la 0200"?
 * Breakpoints?

### Testing

Currently ah-hoc running of the example programs.

