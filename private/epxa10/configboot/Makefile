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

configboot.o: configboot.c

$(CONFIGBOOTHEX): configboot.o crt0.o configboot.x
	$(LD) --script=configboot.x -o configboot.elf crt0.o configboot.o \
		$(ARM_HOME)/arm-elf/lib/gcc-lib/arm-elf/3.2/libgcc.a
	$(OBJCOPY) -O ihex configboot.elf $(CONFIGBOOTHEX)

crt0.o : ../loader/crt0.S ../booter/configboot-pte.S \
	../booter/configboot-epxa.h
	if [[ ! -d booter ]]; then mkdir booter; fi
	cp -v ../booter/configboot-pte.S booter/pte.S
	cp -v ../booter/configboot-epxa.h booter/epxa.h
	$(CPP) -o crt0.i -I. ../loader/crt0.S
	$(AS) $(AFLAGS) -o crt0.o crt0.i



