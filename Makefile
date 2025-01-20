.PHONY: all man man3 man3type man7 clean realclean
.SECONDARY:

top:=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))

examples:=ex1 ex2 ex3 ex4 ex5 ex6 ex-defaulted
all:: unit $(examples) man

test-src:=unit.cc test_failure.cc test_utility.cc test_reader.cc test_defaulted.cc

all-src:=$(test-src) $(patsubst %, %.cc, $(examples))
all-obj:=$(patsubst %.cc, %.o, $(all-src))

gtest-top:=$(top)test/googletest/googletest
gtest-inc:=$(gtest-top)/include
gtest-src:=$(gtest-top)/src/gtest-all.cc

vpath %.cc $(top)test
vpath %.cc $(top)ex
vpath %.md $(top)man
vpath %.3 $(top)man
vpath %.3type $(top)man
vpath %.7 $(top)man

CXXSTD?=c++17
OPTFLAGS?=-O2 -march=native
OPTFLAGS?=-fsanitize=address -march=native
CXXFLAGS+=$(OPTFLAGS) -MMD -MP -std=$(CXXSTD) -pedantic -Wall -Wextra -g -pthread
CPPFLAGS+=-isystem $(gtest-inc) -I $(top)include

depends:=$(patsubst %.cc, %.d, $(all-src)) gtest.d
-include $(depends)

gtest.o: CPPFLAGS+=-I $(gtest-top)
gtest.o: ${gtest-src}
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $<

test-obj:=$(patsubst %.cc, %.o, $(test-src))
unit: $(test-obj) gtest.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

ex1: ex1.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

ex2: ex2.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

ex3: ex3.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

ex4: ex4.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

ex5: ex5.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

ex6: ex6.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

man: man3 man3type man7

# man(3) parapara:: top-level functions

man3-pages:=explain.3
man3-targets:=$(patsubst %, man/man3/parapara\:\:%, $(man3-pages))

man3:: ; mkdir -p man/man3
man3:: $(man3-targets)

man/man3/parapara\:\:%.3: %.3.md
	pandoc -f markdown -t man --standalone --lua-filter=$(top)/man/man-custom.lua -o $@ $<

man/man3/parapara\:\:%.3: %.3
	cp --update $< $@

# man(3type) parapara:: types

man3type-pages:=expected.3type failure.3type hopefully.3type defaulted.3type
man3type-targets:=$(patsubst %, man/man3type/parapara\:\:%, $(man3type-pages))

man3type:: ; mkdir -p man/man3type
man3type:: $(man3type-targets)

man/man3type/parapara\:\:%.3type: %.3type.md
	pandoc -f markdown -t man --standalone --lua-filter=$(top)/man/man-custom.lua -o $@ $<

man/man3type/parapara\:\:%.3type: %.3type
	cp --update $< $@

# man(7) parapara concepts and overview

man7-pages:=parapara.7
man7-targets:=$(patsubst %, man/man7/%, $(man7-pages))

man7:: ; mkdir -p man/man7
man7:: $(man7-targets)

man/man7/%: %.md
	pandoc -f markdown -t man --standalone --lua-filter=$(top)/man/man-custom.lua -o $@ $<


clean:
	rm -f $(all-obj)

realclean: clean
	rm -f $(man3-targets) $(man3type-targets) $(man7-targets)
	rm -f unit $(examples) gtest.o $(depends)
	if [ -d man/man3 ]; then rmdir man/man3; fi
	if [ -d man/man3type ]; then rmdir man/man3type; fi
	if [ -d man/man7 ]; then rmdir man/man7; fi
	if [ -d man ]; then rmdir man; fi
