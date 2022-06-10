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
#                                                                                  #
#   ----------------------------------------------------------------------------   #
#   DISCLAIMER                                                                     #
#   ----------------------------------------------------------------------------   #
#   Example files are provided only as an example of development of a working      #
#   software program using libfmrt libraries. The source code provided is not      #
#   written as an example of a released, production level application, it is       #
#   intended only to demonstrate usage of the API functions used herein.           #
#   The Author provides the source code examples "AS IS" without warranty of       #
#   any kind, either expressed or implied, including, but not limited to the       #
#   implied warranties of merchantability and fitness for a particular purpose.    #
#   The entire risk as to the quality and performance of the source code           #
#   examples is with you. Should any part of the source code examples prove        #
#   defective you (and not the Author) assume the entire cost of all necessary     #
#   servicing, repair or correction. In no event shall the Author be liable for    #
#   damages of any kind, including direct, indirect, incidental, consequential,    #
#   special, exemplary or punitive, even if it has been advised of the possibility #
#   of such damage.                                                                #
#   The Author does not warrant that the contents of the source code examples,     #
#   whether will meet your requirements or that the source code examples are       #
#   error free.                                                                    #
#                                                                                  #
####################################################################################

-----------
(1) PURPOSE
-----------
This file describes libfmrt usage examples and how to compile and use them.


---------------
(2) ASSUMPTIONS
---------------
It is assumed that libraries are correctly installed into /usr/local/lib and that
this path is either configured in /etc/ld.so.conf or in environment variable
$LD_LIBRARY_PATH.


----------------
(3) INSTRUCTIONS
----------------
The library comes with some example files in the ./examples subdirectory.
To compile a generic example file (let's say Example.c), simply type:

    gcc -g -c -O2 -Wall -v -I../../headers Example.c
    gcc -g -o ../bin/Example Example.o -lfmrt

(for shared library linking) or:

    gcc  ./Example.c -I../../headers -L../../lib  -o ../bin/Example -lfmrt

However, they can be compiled all at once by typing either:

    make static

or

    make shared

Both commands above will produce executables in the ./examples/bin subdirectory;
specifically, the former will produce executables linked statically with the
libfmrt library, while the latter will provide executables linked dinamically.

The following command clears all executables:

    make clean


-----------------------------
(4) BRIEF EXAMPLE DESCRIPTION
-----------------------------
Please refer to the description provided in ../docs/libfmrt.doc for a detailed
explanation of the examples in the corresponding directory.


--------------
(5) DISCLAIMER
--------------
Example files are provided only as an example of development of a working
software program using libfmrt libraries. The source code provided is not
written as an example of a released, production level application, it is
intended only to demonstrate usage of the API functions used herein.
The Author provides the source code examples "AS IS" without warranty of
any kind, either expressed or implied, including, but not limited to the
implied warranties of merchantability and fitness for a particular purpose.
The entire risk as to the quality and performance of the source code
examples is with you. Should any part of the source code examples prove
defective you (and not the Author) assume the entire cost of all necessary
servicing, repair or correction. In no event shall the Author be liable for
damages of any kind, including direct, indirect, incidental, consequential,
special, exemplary or punitive, even if it has been advised of the possibility
of such damage.
The Author does not warrant that the contents of the source code examples,
whether will meet your requirements or that the source code examples are
error free.