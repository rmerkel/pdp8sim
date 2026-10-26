# pdp8sim - Yet Another PDP-8 Simulator

Version 0.9 pre-release

A PDP-8 simulator, just for the heck of it!

Primally based on the PDP-8 Straight Eight as described in DEC's 1967 Small Computer Handbook,
"PDP-8 Users Handbook"

The simulator features an integreated debugger modeled after the PDP-8 front-panel.
IO device readyness is via select() and standard IO and files.

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

Most are, sometimes slightly modified versions, from DEC's Introduction to
Programming, Volume 1, 2nd Edition (1970), Elementary Programming Techniques. 

Note that [PALBART](https://www.pdp8online.com/ftp/software/palbart/palbart.c) was used as the MACRO-8 equipment assembler.

## Status

### Bugs

* The use of signal(SIGINT) for Ctrl-C/HALT needs to be investagated. Posix suggest sigaction(2),
  and it's unclear if handling a SIGINT requires get registering the SIGINT handler.
* Interrupts and break are not supported!

### General 

* Can load BIN files from the command line. Maybe add support for RIM format?
  Auto loading of RIM and, or BIN loaders?
* Currently run time is # of cycles * 1.5us except for IOT which has a constant
  runtime of 4.5 us.
* Ctrl-C will halts the simulator, roughly simulating the front panel halt switch.

### Future

 * Trace command - maybe replace bool run with enum class State { Idle, Run, Trace };?
 * Improved "front-panel", e.g, "la 0200"?
 * Breakpoints?

### Testing

Currently ah-hoc running of the example programs.

