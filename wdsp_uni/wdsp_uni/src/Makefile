# Specify compiler
CC = gcc

# Specify extensions of files to delete when cleaning
CLEANEXTS = o a 

# Specify the target file and the install directory
OUTPUTFILE  = libwdsp_uni.a
INSTALLDIR  = ../Linux

# Default target
.PHONY: all
all: $(OUTPUTFILE)

# Build wdsp_uni.a from *.o
$(OUTPUTFILE):  amd.o\
                cfcomp.o\
                eq.o\
                iobuffs.o\
                osctrl.o\
                TXA.o\
                ammod.o\
                cfir.o\
                fcurve.o\
                iqc.o\
                patchpanel.o\
                utilities.o\
                amsq.o\
                channel.o\
                fir.o\
                linux_port.o\
                resample.o\
                varsamp.o\
                analyzer.o\
                comm.o\
                firmin.o\
                lmath.o\
                rmatch.o\
                version.o\
                anf.o\
                compress.o\
                fmd.o\
                main.o\
                RXA.o\
                wcpAGC.o\
                anr.o\
                delay.o\
                fmmod.o\
                meter.o\
                sender.o\
                wisdom.o\
                bandpass.o\
                div.o\
                fmsq.o\
                meterlog10.o\
                shift.o\
                calcc.o\
                eer.o\
                gain.o\
                nbp.o\
                siphon.o\
                calculus.o\
                emnr.o\
                gen.o\
                nob.o\
                slew.o\
                cblock.o\
                emph.o\
                iir.o\
                nobII.o\
                snb.o
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