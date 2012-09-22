PAPER = hw1 
TEX = $(wildcard *.tex)
BIB = 
FIGS = $(wildcard fig/*.pdf figs/*.png figs/*.jpg)

.PHONY: all clean

$(PAPER).pdf: $(TEX) $(BIB) $(FIGS)
	pdflatex $(PAPER)

clean:
	rm -f *.aux *.bbl *.blg *.log *.out $(PAPER).pdf

