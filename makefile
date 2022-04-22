####################################################################################
#   ---------------------------------------------------                            #
#   C/C++ Fast Memory Resident Tables Library (libfmrt)                            #
#   ---------------------------------------------------                            #
#   Copyright 2020-2021 Roberto Mameli                                             #
#                                                                                  #
#   Licensed under the Apache License, Version 2.0 (the "License");                #
#   you may not use this file except in compliance with the License.               #
#   You may obtain a copy of the License at                                        #
#                                                                                  #
#       http://www.apache.org/licenses/LICENSE-2.0                                 #
#                                                                                  #
#   Unless required by applicable law or agreed to in writing, software            #
#   distributed under the License is distributed on an "AS IS" BASIS,              #
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.       #
#   See the License for the specific language governing permissions and            #
#   limitations under the License.                                                 #
#   ------------------------------------------------------------------------       #
#                                                                                  #
#   FILE:        libfmrt makefile                                                  #
#   VERSION:     1.0.0                                                             #
#   AUTHOR(S):   Roberto Mameli                                                    #
#   PRODUCT:     Library libfmrt                                                   #
#   DESCRIPTION: This library provides a collection of routines that can be        #
#                used in C/C++ source programs to implement memory resident        #
#                tables with fast access capability. The tables handled by         #
#                the library reside in memory and are characterized by             #
#                O(log(n)) complexity, both for read and for write operations      #
#   REV HISTORY: See updated Revision History in file RevHistory.txt               #
#                                                                                  #
####################################################################################

FMRTLIB = libfmrt
FMRTSOURCES = ./src/fmrtApi.c
FMRTOBJS = ./obj/fmrtApi.o
LIBS = -lpthread

INCLUDE = -I. -I./include -I./headers
SOLIB = /usr/local/lib
LIB = ./lib

CC = gcc
RM = rm
AR = ar
CP = cp
MV = mv

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c -Wall -v $(INCLUDE) -o $@ $<

all:
	$(CC) -c -fpic -Wall -v $(INCLUDE) $(FMRTSOURCES) $(LIBS)
	$(CC) -shared -Wl,-soname,$(FMRTLIB).so.1 -o $(FMRTLIB).so.1.0  $(FMRTOBJS) -lc
	$(AR) rcs $(FMRTLIB).a $(FMRTOBJS)

install:
	$(MV) $(FMRTLIB).a $(LIB)
	chown root:root $(LIB)/$(FMRTLIB).a
	chmod 777 $(LIB)/$(FMRTLIB).a
	$(MV) $(FMRTLIB).so.1.0 $(SOLIB)
	ln -sf $(SOLIB)/$(FMRTLIB).so.1 $(SOLIB)/$(FMRTLIB).so
	chown root:root $(SOLIB)/$(FMRTLIB).so.1.0
	chmod 777 $(SOLIB)/$(FMRTLIB).so.1.0
	$(CP) ./headers/*.h /usr/local/include
	chown root:root /usr/local/include/*.h
	ldconfig -n $(SOLIB)

clean:
	$(RM) $(FMRTOBJS) $(LIB)/$(FMRTLIB).* $(SOLIB)/$(FMRTLIB).* $(FMRTLIB).*