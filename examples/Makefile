SRCS	= $(wildcard *.pa)
BINS	= $(SRCS:.pa=.bin)
RIMS	= $(SRCS:.pa=.rim)
LSTS	= $(SRCS:.pa=.lst)

PAL	= palbart

%.rim: %.pa
	$(PAL) -t 8 -r $<

%.bin: %.pa
	$(PAL) -t 8 $<

all:    $(BINS)

clean:
	rm -f $(BINS) $(RIMS)

cleanall:	clean
	@rm -f $(LSTS)

