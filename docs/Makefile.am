asciidocfiles1 = \
	xmlargs.txt \
	xmlforeach.txt

EXTRA_DIST = $(asciidocfiles1)

dist_man_MANS = $(asciidocfiles1:.txt=.1)
dist_html_DATA = $(asciidocfiles1:.txt=.html)

%.1 : %.xml
	xmlto man $<

%.7 : %.xml
	xmlto man $<

%.xml : %.txt asciidoc.conf
	asciidoc -b docbook -d manpage -f $(srcdir)/asciidoc.conf -o $@ $<

%.html : %.txt asciidoc.conf
	asciidoc -b xhtml11 -d manpage -f $(srcdir)/asciidoc.conf -o $@ $<
