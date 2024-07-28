all:    add_chk_ovr.bin tally.bin sub.bin

add_chk_ovr.bin:     add_chk_ovr.pa
	palbart $<

sub.bin:                sub.pa
	palbart $<
 
tally.bin:	tally.pa
	palbart $<

clean:
	@rm -f *.lst *.bin *.rim
