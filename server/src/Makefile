# Specify compiler
CC = gcc

# Specify extensions of files to delete when cleaning
CLEANEXTS = o a 

# Specify the target file and the install directory
OUTPUTFILE  = SDRLibE.a
INSTALLDIR  = ../Linux

# Default target
.PHONY: all
all: $(OUTPUTFILE)

# Build wdsp_uni.a from *.o
$(OUTPUTFILE):  audio/local_audio.o\
                helpers/utils.o\
                pipeline/pipeline.o\
                radio/cc_in.o\
                radio/cc_out.o\
                radio/decoder.o\
                radio/encoder.o\
                radio/hw_control.o\
                radio/radio_defs.o\
                radio/seq_proc.o\
                radio/sockets.o\
                radio/udp_reader.o\
                radio/udp_writer.o\
                ringbuffer/ringb.o\
                server/dsp_man.o\
                server/server.o
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