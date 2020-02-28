# Specify extensions of files to delete when cleaning
CLEANEXTS   = o a 

# Specify the target file and the install directory
OUTPUTFILE  = wdsp_uni.a
INSTALLDIR  = ./Linux

# Default target
.PHONY: all
all: $(OUTPUTFILE)

# Build wdsp_uni.a from *.o
$(OUTPUTFILE): *.o
    ar ru $@ $^
    ranlib $@

# No rule to build john.o, paul.o, and johnpaul.o from .cpp 
# files is required; this is handled by make's database of
# implicit rules

.PHONY: install
install:
    mkdir -p $(INSTALLDIR)
    cp -p $(OUTPUTFILE) $(INSTALLDIR)

.PHONY: clean 
clean:
    for file in $(CLEANEXTS); do rm -f *.$$file; done

# Indicate dependencies of .ccp files on .h files
*.o: comm.h
