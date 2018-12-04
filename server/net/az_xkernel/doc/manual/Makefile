.SUFFIXES:
.SUFFIXES:	.ps .dvi .tex

LATEX = nlatex
PAGES =

% :: RCS/%,v

.dvi.ps:
	dvips -o $@ $<

SOURCE = manual

GREP = fgrep -i
# GREP = egrep

SOURCES = abstract.tex intro.tex upi.tex participant.tex message.tex \
	event.tex map.tex thread.tex trace.tex utilities.tex \
	config.tex compose.tex running.tex debug.tex\
	prots.tex protNums.tex releasing.tex install.tex \
	install_mach_kernel.tex \
	control.tex template.tex protocols/udp.tex protocols/arp.tex\
	protocols/*.tex \
	${SOURCE}.tex # udp.verbatim

# udp.verbatim: ../../merge/include/prot/udp.h ../../merge/protocols/udp/udp.c ../../merge/protocols/udp/udp_internal.h ../../merge/protocols/udp/udp_port.h ../../merge/protocols/udp/udp_port.c ../../merge/protocols/util/port_mgr.h ../../merge/protocols/util/port_mgr.c
#	udp.verbatim.csh

$(SOURCE).ps: $(SOURCE).dvi
	

${SOURCE}.dvi:	${SOURCES}
	${LATEX} ${SOURCE}

nightly:	$(SOURCES)
	$(LATEX) $(SOURCE)
	$(LATEX) $(SOURCE)
	make $(SOURCE).ps
	touch nightly
	
full:
	${LATEX} ${SOURCE}
	bibtex ${SOURCE}
	${LATEX} ${SOURCE}
	${LATEX} ${SOURCE}

spell:		${SOURCES}
	spell ${SOURCES}

all:	${SOURCES}
	${LATEX} ${SOURCE}

print:	${SOURCE}.ps
	lpr -i ${SOURCE}.ps

grep:	${REALSOURCES}
	@-${GREP} -n -e '${G}' ${REALSOURCES} | \
	sed 's/^\([^:]*\):\([^:]*\):/"\1", line \2:/'
	@sleep 1

clean:
	rm -f *.aux *.bbl *.blg *.toc *.lot *.log *.lof *.dvi *.ps \
		*.log *.bak *.ckp
