/*****************************************************************************
 * ---------------------------------------------------                       *
 * C/C++ Fast Memory Resident Tables Library (libfmrt)                       *
 * ---------------------------------------------------                       *
 * Copyright 2020-2021 Roberto Mameli                                        *
 *                                                                           *
 * Licensed under the Apache License, Version 2.0 (the "License");           *
 * you may not use this file except in compliance with the License.          *
 * You may obtain a copy of the License at                                   *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 * Unless required by applicable law or agreed to in writing, software       *
 * distributed under the License is distributed on an "AS IS" BASIS,         *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
 * See the License for the specific language governing permissions and       *
 * limitations under the License.                                            *
 * ------------------------------------------------------------------------- *
 *                                                                           *
 * FILE:        fmrtApi.h                                                    *
 * VERSION:     1.0.0                                                        *
 * AUTHOR(S):   Roberto Mameli                                               *
 * PRODUCT:     Library libfmrt                                              *
 * DESCRIPTION: This library provides a collection of routines that can be   *
 *              used in C/C++ source programs to implement memory resident   *
 *              tables with fast access capability. The tables handled by    *
 *              the library reside in memory and are characterized by        *
 *              O(log(n)) complexity, both for read and for write operations *
 * REV HISTORY: See updated Revision History in file RevHistory.txt          *
 *                                                                           *
 *****************************************************************************/

#ifndef FMRTAPI_H_
#define FMRTAPI_H_


/*****************
 * Include Files *
 *****************/
#include <pthread.h>
#include "fmrt.h"


/*******************************
 * General Purpose Definitions *
 *******************************/
/* Comment the following line to avoid debug printouts - Uncomment it during development and test */
//#define FMRTDEBUG
#define FMRTDEBUGLIMIT           100    /* Debug printouts are obtained only if the number of elements is less than FMRTDEBUGLIMIT */

/* Some libfmrt limits */
#define MAXTABLES                 32    /* Max number of AVL Tree tables             */
#define MAXFMRTELEM         67108864    /* Max Number Elements in table - 2^26       */
#define MAXFMRTFIELDNUM           16    /* Max num of fieds for each table           */
#define MAXFMRTTABLENAME          32    /* Max Length for table name                 */
#define MAXFMRTNAMELEN            16    /* Max length for key/field name             */
#define MAXFMRTSTRINGLEN          64    /* Max length for string data                */
#define MAXCSVLINELEN           1200    /* Max allowed length for lines in CSV files */

/* Constant used to indicate the null fmrtIndex pointer */
#define FMRTNULLPTR       0xFFFFFFFF    /* Constant used to identify NULL ptr        */

/* Used in traversal node LIFO structure to indicate the path to the next node       */
#define LEFT                      -1    /* Used to identify LEFT subtree             */
#define STAY                       0    /* This is the node we were looking for      */
#define RIGHT                      1    /* Used to identify RIGHT subtree            */

/* Possible statuses of a fmrtTableItem */
#define FREE                       0    /* Available for allocation to new table     */
#define DEFINED                    1    /* Defined, but without elements so far      */
#define NOTEMPTY                   2    /* At least one element in the AVL Tree      */

/* Default time format for FMRTTIMESTAMP type */
//#define FMRTTIMEFORMAT            ""    /* Dafault value is empty string, i.e. time_stamp printed in raw format */
#define FMRTTIMEFORMAT            "%c"    /* Dafault value is preferred date and time representation for the current locale */


/********************
* Type Definitions *
********************/
/* Internal structure representing key and/or generic field */
typedef struct field
{
    char            name[MAXFMRTNAMELEN+1];
    fmrtType        type;
    fmrtLen         len;
    uint16_t        delta;
} fmrtField;

/* Set of information stored internally for each table */
typedef struct tableItem
{
    fmrtId          tableId;
    uint8_t         status,
                    numFields;
    char            tableName[MAXFMRTTABLENAME+1];
    fmrtIndex       tableMaxElem,
                    currentNumElem,
                    fmrtRoot,
                    fmrtFree;
    fmrtField       key,
                    fields[MAXFMRTFIELDNUM];
    uint16_t        elemSize;
    pthread_mutex_t tableMtx;
    void            *fmrtData;
} fmrtTableItem;

/* Generic element in the LIFO structure built by searchElem() when traversing the tree */
typedef struct nodeTraversalStack
{
    fmrtIndex       index;
    int8_t          go;
    struct nodeTraversalStack *next;
} fmrtNodeTraversalStack;


/***********************
 * Function Prototypes *
 ***********************/


#endif /* FMRTAPI_H_*/