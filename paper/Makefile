# $Id: Makefile 156 2005-12-03 20:26:19Z lostrace $

TEX = /usr/bin/latex
.PHONY : all install clean autowindow autoupdate autoreload

TGIFS = $(shell echo *.obj)
TGIF_FIGURES = $(patsubst %.obj,%.eps,$(TGIFS))

TEXTS = $(shell echo *.tex)

BIBS = $(shell echo *.bib)



all: paper.pdf

clean:
	-rm *.log *.lot *.lof *.aux *.toc *.bbl *.blg *.out	> /dev/null 2>&1
	-rm $(TGIF_FIGURES) 					> /dev/null 2>&1
	-rm *.pdf *.dvi *.ps					> /dev/null 2>&1

autowindow: paper.pdf
	xpdf -remote dako paper.pdf&

autoreload:
	xpdf -remote dako -reload;
	
autoupdate: paper.pdf autowindow
	while true; do										\
		sleep 1; 									\
		for FILE in *.tex *.bib *.fig *.obj; do						\
			if [ -e paper.pdf -a paper.pdf -ot "$$FILE" ]; then	\
				make paper.pdf < /dev/null;				\
				if ps aux | grep xpdf | grep -v grep; then			\
					make autoreload;					\
				fi;								\
			fi;									\
		done;										\
	done;

%.eps: %.obj
	tgif -print -epsi $<

#paper.pdf: ${TGIF_FIGURES} ${TEXTS} ${BIBS}
paper.pdf: ${TEXTS}
	latex paper.tex
	bibtex paper
	latex paper.tex
	latex paper.tex
	#dvipdfm -o paper.pdf paper.dvi
	dvipdf paper.dvi
