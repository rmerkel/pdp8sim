################################################################################
# @file makefile
#
# @brief makefile for P language complier & interperter
################################################################################

################################################################################
# Build a debug (DEBUG=1: default) or release (DEBUG=0) binary?
#
# Switch between GCC and CLang with
#   sudo update-alternatives --config c++
#   sudo update-alternatives --config cc
################################################################################

CC = cc

#Support C11, enable all and extra warnings, generate dependency files
CCFLAGS += -std=c11 -Wall -Wextra -MMD -MP

# Build for debugging (default), or release/optimized
DEBUG	?= 1
ifeq	($(DEBUG),1)
	CCFLAGS += -O0 -g3 -DDEBUG
else
	CCFLAGS += -O3 -DNDEBUG
endif

################################################################################
# Project directories
################################################################################

DOCDIR	= docs
OBJDIR	= objs

################################################################################
# Project files
################################################################################

SRCS	= $(wildcard *.c) $(wildcard *.cc)
ALLSRCS	= $(SRCS) $(wildcard *.h)
OBJS	= $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
DEPS	= $(addprefix $(OBJDIR)/,$(SRCS:.c=.d))
DOCS	= $(wildcard *.md)
EXE		= pdp8sim

################################################################################
# Rules
################################################################################

$(OBJDIR)/%.o: %.c
	$(CC) $(CCFLAGS) -c -o $@ $<

.PHONY:	all clean cleanall $(DOCDIR) help pr test

################################################################################
#	The default target...
################################################################################

all:	$(EXE) $(DOCDIR)

################################################################################
# pdp8sim
################################################################################

$(EXE): $(OBJDIR) $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

$(OBJDIR):
	@mkdir -p $(OBJDIR) 

################################################################################
# Include generated dependencies
################################################################################

-include $(DEPS)

################################################################################
# Cleanup intermediates...
################################################################################

clean:
	@rm -f $(OBJDIR)/*

################################################################################
# Cleanup all targets and intermediates...
################################################################################

cleanall: clean
	@rm -rf $(EXE) $(DOCDIR) $(OBJDIR)

################################################################################
# Generate documentation
################################################################################

$(DOCDIR):	$(DOCDIR)/html/index.html

$(DOCDIR)/html/index.html:	Doxyfile $(ALLSRCS) $(DOCS)
	@doxygen

################################################################################
# Print a help message...
################################################################################

help:
	@echo "Usage:  make [DEBUG=0|1] targets..."
	@echo ""
	@echo "DEBUG=1, (default), builds a debug image and 0 builds a release."
	@echo ""
	@echo "Targets:"
	@echo "    all     - build the simulator  and generate documentation (default)."
	@echo "    clean   - to delete intermediates."
	@echo "    cleanll - to delete all targets and intermediates."
	@echo "    docs    - to generate documentation."
	@echo "    help    - prints this message."
	@echo "    pdp8sim - to build the simulator."
	@echo "    pr      - prepare source for printing"
	@echo "    test    - build and run the simulator."
	@echo ""

################################################################################
# Print
################################################################################

pr:
	@pr --expand-tabs=4 $(ALLSRCS) Makefile $(DOCS)

################################################################################
# Bring up to date and run some tests...
################################################################################

test: all
	./${EXE}

