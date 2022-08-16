# The Sphysl Project (C) 2022 Jyothiraditya Nellakra
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see <https://www.gnu.org/licenses/>.

headers = $(shell find inc/ -name "*.h")
objs = $(patsubst %.cc,%.o,$(wildcard src/*.cc))

demos = $(patsubst demo/%.cc,%,$(wildcard demo/*.cc))
demo_objs = $(patsubst %.cc,%.o,$(wildcard demo/*.cc))

files = $(foreach file,$(objs) $(demo_objs),$(wildcard $(file)))
files += $(wildcard *.a)

CLEAN = $(foreach file,$(files),rm $(file);)

CPPFLAGS += -Wall -Wextra -Wpedantic -std=c++17 -g -I inc/ -I libClame/inc/
CXXFLAGS += -std=c++17 -g

libs = libClame/libClame.a libSphysl.a
LD_LIBS ?= -L. -lSphysl -lm -L libClame -lClame -lpthread -lm

$(objs) : %.o : %.cc $(headers)
	$(CXX) $(CPPFLAGS) -c $< -o $@

libSphysl.a : $(objs)
	$(AR) -r libSphysl.a $(objs)

$(demo_objs) : %.o : %.cc $(headers) libClame
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(demos) : % : demo/%.o $(libs)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LD_LIBS)

$(demo_shs) : % : demo/%.sh
	cp $< $@; chmod +x $@

libClame/libClame.a : libClame
	+cd libClame; make libClame.a; cd ..

.DEFAULT_GOAL = all
.PHONY : all clean

all : libSphysl.a $(demos) $(demo_shs)

clean :
	$(CLEAN)