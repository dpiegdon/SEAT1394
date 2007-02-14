
.PHONY: final all draft clean

all:	final

final:	folien.pdf
	pdfopt folien.pdf folien-opt.pdf
	mv folien-opt.pdf folien.pdf

folien.pdf: folien.tex beamercolorthemelostrace.sty
	pdflatex folien.tex
	pdflatex folien.tex

draft:
	sed 's/^\\documentclass{beamer}/\documentclass[draft]{beamer}/' folien.tex > foliendraft.tex
	pdflatex foliendraft.tex
	-rm -f foliendraft.aux foliendraft.log foliendraft.nav foliendraft.out foliendraft.snm foliendraft.toc foliendraft.tex

clean:
	-rm -f folien.aux folien.log folien.nav folien.out folien.snm folien.toc folien.pdf foliendraft.pdf

