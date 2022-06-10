####################################################################################
#   ---------------------------------------------------                            #
#   C/C++ Fast Memory Resident Tables Library (libfmrt)                            #
#   ---------------------------------------------------                            #
#   Copyright 2022 Roberto Mameli                                                  #
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
	gcc -g -c -O2 -Wall -v -I../headers ./src/DictionaryWords.c
	gcc -g -o ./bin/DictionaryWords ./DictionaryWords.o -lfmrt
	rm ./DictionaryWords.o
	gcc -g -c -O2 -Wall -v -I../headers ./src/CountWordsOccurrence.c
	gcc -g -o ./bin/CountWordsOccurrence ./CountWordsOccurrence.o -lfmrt
	rm ./CountWordsOccurrence.o
	gcc -g -c -O2 -Wall -v -I../headers ./src/BarCodeCache.c
	gcc -g -o ./bin/BarCodeCache ./BarCodeCache.o -lfmrt
	rm ./BarCodeCache.o
	gcc -g -c -O2 -Wall -v -I../headers ./src/Televoting.c
	gcc -g -o ./bin/Televoting ./Televoting.o -lfmrt -lpthread
	rm ./Televoting.o

static:
	gcc ./src/DictionaryWords.c -I../headers -L../lib -v -Wall -o ./bin/DictionaryWords -lfmrt
	gcc ./src/CountWordsOccurrence.c -I../headers -L../lib -v -Wall -o ./bin/CountWordsOccurrence -lfmrt
	gcc ./src/BarCodeCache.c -I../headers -L../lib -v -Wall -o ./bin/BarCodeCache -lfmrt
	gcc ./src/Televoting.c -I../headers -L../lib -v -Wall -o ./bin/Televoting -lfmrt -lpthread

clean:
	rm ./bin/DictionaryWords
	rm ./bin/CountWordsOccurrence
	rm ./bin/BarCodeCache
	rm ./bin/Televoting
