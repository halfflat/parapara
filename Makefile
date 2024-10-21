.PHONY: all clean realclean
.SECONDARY:

top:=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))

examples:=ex1 ex2 ex3 ex4
all:: unit $(examples)

test-src:=unit.cc test_failure.cc test_utility.cc

all-src:=$(test-src) $(patsubst %, %.cc, $(examples))
all-obj:=$(patsubst %.cc, %.o, $(all-src))

gtest-top:=$(top)test/googletest/googletest
gtest-inc:=$(gtest-top)/include
gtest-src:=$(gtest-top)/src/gtest-all.cc

vpath %.cc $(top)test
vpath %.cc $(top)ex

CXXSTD?=c++17
#OPTFLAGS?=-O2 -fsanitize=address -march=native
OPTFLAGS?=-march=native
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


clean:
	rm -f $(all-obj)

realclean: clean
	rm -f unit $(examples) gtest.o $(depends)
