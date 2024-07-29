SRCS	= $(wildcard *.pa)
BINS	= $(SRCS:.pa=.bin)

%.bin: %.pa
	palbart $<

all:    $(BINS)

clean:
	@rm -f *.lst *.bin *.rim
