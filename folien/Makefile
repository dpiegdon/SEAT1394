
.PHONY: final all draft clean autoupdate autoreload autowindow

all:	final

final:	folien.pdf
	pdfopt folien.pdf folien-opt.pdf
	mv folien-opt.pdf folien.pdf

folien.pdf: folien.tex beamercolorthemelostrace.sty *.png
	pdflatex folien.tex
	pdflatex folien.tex

draft:
	sed 's/^\\documentclass{beamer}/\documentclass[draft]{beamer}/' folien.tex > foliendraft.tex
	pdflatex foliendraft.tex
	-rm -f foliendraft.aux foliendraft.log foliendraft.nav foliendraft.out foliendraft.snm foliendraft.toc foliendraft.tex

clean:
	-rm -f folien.aux folien.log folien.nav folien.out folien.snm folien.toc folien.pdf foliendraft.pdf

autowindow: folien.pdf
	xpdf -remote SEAT1394f folien.pdf&

autoreload:
	xpdf -remote SEAT1394f -reload;
	
autoupdate: folien.pdf autowindow
	while true; do										\
		sleep 1; 									\
		for FILE in *.tex *.bib *.fig *.obj *.png; do					\
			if [ -e folien.pdf -a folien.pdf -ot "$$FILE" ]; then			\
				make folien.pdf < /dev/null;					\
				if ps aux | grep xpdf | grep -v grep; then			\
					make autoreload;					\
				fi;								\
			fi;									\
		done;										\
	done;
