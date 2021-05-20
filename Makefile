# The Sphysl Project (C) 2021 Jyothiraditya Nellakra
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

objs = $(patsubst %.c,%.o,$(wildcard src/*.c))
headers = $(wildcard *.h)

files = $(foreach file,$(objs),$(wildcard $(file)))
files += $(wildcard *.a)

CLEAN = $(foreach file,$(files),rm $(file);)

CC ?= gcc
CFLAGS += -std=c11
CPPFLAGS += -Wall -Wextra -Werror -O3 -I.

AR ?= ar

$(objs) : %.o : %.c $(headers)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

libSphysl.a : $(objs)
	$(AR) -r libSphysl.a $(objs)

.DEFAULT_GOAL = all
.PHONY : all clean

all : libSphysl.a

clean :
	$(CLEAN)

run : simple-flight
	./simple-flight