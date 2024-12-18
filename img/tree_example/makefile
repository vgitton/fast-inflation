.PHONY: all
all: tree_1.png tree_2.png tree_3.png tree_4.png tree_5.png

%.png: %.pdf makefile
	convert -density 200 $< -quality 100 $@

