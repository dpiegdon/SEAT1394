
all:
	echo no!

NCD-make-stuff:

distmatrix.txt:
	ncd -d testpages testpages > distmatrix.txt

tree:	distmatrix.txt
	maketree -T distmatrix.txt

png_updater:
	while /bin/true; do make tree_neato.png tree_neato-2.png; sleep 300; done;

tree_neato.png:	treefile.dot
	neato -Tpng treefile.dot > tree_neato.png
tree_neato-2.png: treefile-2.dot
	neato -Tpng treefile-2.dot > tree_neato-2.png

