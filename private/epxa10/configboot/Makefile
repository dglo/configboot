all: $(CONFIGBOOTHEX)

doc: configboot-ug.dvi

configboot-ug.dvi: configboot-ug.tex configboot-ug.ind 
	latex configboot-ug.tex

configboot-ug.ind: configboot-ug.idx
	makeindex configboot-ug.idx

configboot-ug.idx: configboot-ug.tex
	latex configboot-ug.tex

configboot-ug.ps: configboot-ug.dvi
	dvips -o configboot-ug.ps configboot-ug.dvi

configboot-ug.pdf : configboot-ug.ps
	gs -dNOPAUSE -q -dBATCH -sDEVICE=pdfwrite \
		-sOutputFile=configboot-ug.pdf configboot-ug.ps

install: configboot-ug.pdf configboot-ug.ps configboot-ug.dvi
	cp -v configboot-ug.pdf configboot-ug.ps configboot-ug.dvi \
		~/public_html/dom

clean:
	rm -f configboot-ug.dvi configboot-ug.ind \
		configboot-ug.log configboot-ug.aux \
		configboot-ug.ilg configboot-ug.idx \
		configboot-ug.toc
	rm -f *.o *.i configboot.elf
	rm -rf booter

configboot.o: configboot.c memset.S crt0.S

$(CONFIGBOOTHEX): configboot.o crt0.o memset.o configboot.x
	$(LD) --script=configboot.x -o configboot.elf crt0.o configboot.o \
		memset.o \
		../hal/comm-hal.o ../hal/hal.o \
		$(ARM_HOME)/arm-elf/lib/gcc-lib/arm-elf/3.2/libgcc.a
	$(OBJCOPY) -O ihex configboot.elf $(CONFIGBOOTHEX)

crt0.o : crt0.S 
	$(CPP) -o crt0.i -I. crt0.S
	$(AS) $(AFLAGS) -o crt0.o crt0.i

memset.o: memset.S
	$(AS) $(AFLAGS) -o memset.o memset.S

#
# c:\quartus\bin\makeprogfile.exe -e EPC8 --pof configboot.pof \
#   simpletest.sbd simpletest.psof configboot.hex
#




