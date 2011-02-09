bin_PROGRAMS = xmlargs xmlforeach # xmltsort

SUBDIRS = data

xmlargs_SOURCES = \
		xmlargs.cc \
		xpath-on-stream.h \
		xml-util.h \
		crawl-with-fork.h \
		process-handler.h \
		process-handler.cc

xmlargs_LDFLAGS = @XML_LIBS@

xmlforeach_SOURCES = \
		xmlforeach.cc \
		xpath-on-stream.h \
		xml-util.h \
		crawl-with-fork.h \
		process-handler.h \
		process-handler.cc

xmlforeach_LDFLAGS = @XML_LIBS@

# xmltsort_SOURCES = \
# 		xmltsort.cc \
# 		xml-graph.h \
# 		stream-dfs.h \
# 		xpath-on-stream.h \
# 		xml-util.h \
# 		crawl-with-fork.h \
# 		process-handler.h \
# 		process-handler.cc
# 
# # Darn graphviz
# xmltsort_LDFLAGS = @XML_LIBS@ -Wl,'-rpath=/usr/lib/graphviz' @GRAPH_LIBS@

AM_CXXFLAGS = @XML_CFLAGS@

TESTS = \
	test-xmlargs.sh \
	test-xmlforeach.sh
# 	test-xmltsort.sh

EXTRA_DIST = $(TESTS)
