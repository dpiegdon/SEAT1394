#   $Id$
#   create pdf of paper
# 
#   Copyright (C) 2006,2007
#   losTrace aka "David R. Piegdon"
# 
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License version 2, as
#   published by the Free Software Foundation.
# 
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
# 
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

TEX = /usr/bin/latex
.PHONY : all install clean autowindow autoupdate autoreload final

TGIFS = $(shell echo *.obj)
TGIF_FIGURES = $(patsubst %.obj,%.eps,$(TGIFS))

TEXTS = $(shell echo *.tex)

BIBS = $(shell echo *.bib)



all: final

final: paper.pdf
	pdfopt paper.pdf paper-opt.pdf
	mv paper-opt.pdf paper.pdf

clean:
	-rm *.log *.lot *.lof *.aux *.toc *.bbl *.blg *.out	> /dev/null 2>&1
#	-rm $(TGIF_FIGURES) 					> /dev/null 2>&1
	-rm *.pdf *.dvi *.ps					> /dev/null 2>&1

autowindow: paper.pdf
	xpdf -remote SEAT1394 paper.pdf&

autoreload:
	xpdf -remote SEAT1394 -reload;
	
autoupdate: paper.pdf autowindow
	while true; do										\
		sleep 1; 									\
		for FILE in *.tex *.bib *.fig *.obj; do						\
			if [ -e paper.pdf -a paper.pdf -ot "$$FILE" ]; then			\
				make paper.pdf < /dev/null;					\
				if ps aux | grep xpdf | grep -v grep; then			\
					make autoreload;					\
				fi;								\
			fi;									\
		done;										\
	done;

%.eps: %.obj
	tgif -print -epsi $<

#paper.pdf: ${TGIF_FIGURES} ${TEXTS} ${BIBS}
paper.dvi: ${TEXTS} ${BIBS}
	# generate .aux for bibtex
	latex paper.tex
	# generate bibtex
	bibtex paper
	# generate aux for second run
	latex paper.tex
	# final run
	latex paper.tex

paper.ps: paper.dvi
	dvips -t a4 paper.dvi -o paper.ps

paper.pdf: paper.ps
	ps2pdf paper.ps

