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
#   FILE:        makefile for libfmrt examples                                     #
#   VERSION:     1.0.0                                                             #
#   AUTHOR(S):   Roberto Mameli                                                    #
#   PRODUCT:     Library libfmrt examples                                          #
#   DESCRIPTION: This library provides a collection of routines that can be        #
#                used in C/C++ source programs to implement memory resident        #
#                tables with fast access capability. The tables handled by         #
#                the library reside in memory and are characterized by             #
#                O(log(n)) complexity, both for read and for write operations      #
#   REV HISTORY: See updated Revision History in file RevHistory.txt               #
#                                                                                  #
####################################################################################

shared:
	gcc -g -c -O2 -Wall -v -I../headers ./src/ItalianWords.c
	gcc -g -o ./bin/ItalianWords ./ItalianWords.o -lfmrt
	rm ./ItalianWords.o
	
static:
	gcc  ./src/ItalianWords.c -I../headers -L../lib -v -Wall -o ./bin/ItalianWords -lfmrt

clean:
	rm ./bin/ItalianWords

