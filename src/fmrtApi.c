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
 * FILE:        fmrtApi.c                                                    *
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


/**********************
 * Linux system files *
 **********************/
#define _XOPEN_SOURCE 500   /* glibc (2.12 or above) needs this for proper handling of strptime() */

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>


/************************
 * libfmrt include files *
 ************************/
#include "fmrt.h"
#include "fmrtApi.h"


/****************************
 * Global private variables *
 ****************************/
static uint8_t          fmrtFirstInvocation = 1;
static fmrtTableItem    Tables[MAXTABLES];
static pthread_mutex_t  fmrtGlobalMtx = PTHREAD_MUTEX_INITIALIZER;
static char             fmrtTimeFormat[MAXFMRTSTRINGLEN] = FMRTTIMEFORMAT;


/*******************************
 * Debug Functions             *
 * --------------------------- *
 * They can be used for debug  *
 * purposes. Just add the      *
 * corresponding prototype into*
 * the calling source code and *
 * invoke them whenever needed *
 * to print relevant infos     *
 * To use the following        *
 * functions, macro FMRTDEBUG  *
 * must be enabled in fmrtApi.h*
 *******************************/
#ifdef FMRTDEBUG
/***********************************************************
 * __fmrtDebugPrintStructure()
 * ---------------------------------------------------------
 * Internal function useful for debugging library
 * The prototype shall be added to the caller
 * When invoked, it prints the structure of the Table[]
 * element whose tableId is specified as a parameter
 ***********************************************************/
fmrtResult __fmrtDebugPrintStructure (fmrtId tableId)
{
    /* Local Variables */
    uint8_t     i,j;

    /* If this is the first invocation of the library provide error and exit */
    if (fmrtFirstInvocation)
        return (FMRTKO);

    /* Search for the table with given tableId */
    for (i=0; (i<MAXTABLES)&&(Tables[i].tableId != tableId); i++)

    /* If not found provide an error */
    if ( (i==MAXTABLES) || (Tables[i].status==FREE) )
        return (FMRTIDNOTFOUND);

    /* Print on the screen all Table[] parameters */
    printf ("Table Name: %s (Id: %d)\n",Tables[i].tableName,Tables[i].tableId);
    printf ("\tMax Num Elems: %d - Element Size: %d bytes\n",Tables[i].tableMaxElem,Tables[i].elemSize);
    printf ("\tCurrent AVL Tree Root Index: %d\n",Tables[i].fmrtRoot);
    printf ("\tKey (Name: %s - Type: %d - Len: %d - Delta: %d\n",Tables[i].key.name, Tables[i].key.type, Tables[i].key.len, Tables[i].key.delta);
    for (j=0; j<Tables[i].numFields; j++)
        printf ("\tField: %d (Name: %s - Type: %d - Len: %d - Delta: %d\n",j,Tables[i].fields[j].name, Tables[i].fields[j].type, Tables[i].fields[j].len, Tables[i].fields[j].delta);

    return (FMRTOK);
}


/***********************************************************
 * __fmrtDebugPrintNode()
 * ---------------------------------------------------------
 * Internal function useful for debugging library
 * The prototype shall be added to the caller
 * When invoked, it prints the content of the fmrt tree
 * element whose tableId and index are given as parameters
 ***********************************************************/
fmrtResult __fmrtDebugPrintNode (fmrtId tableId, fmrtIndex index)
{
    /* Local Variables */
    uint8_t     i,j;
    fmrtIndex    leftPtr, rightPtr;
    void        *currentPtr;

    /* If this is the first invocation of the library provide error and exit */
    if (fmrtFirstInvocation)
        return (FMRTKO);

    /* Search for the table with given tableId */
    for (i=0; (i<MAXTABLES)&&(Tables[i].tableId != tableId); i++)

    /* If not found provide an error */
    if ( (i==MAXTABLES) || (Tables[i].status==FREE) )
        return (FMRTIDNOTFOUND);

    /* tableId has been found, if the AVL Tree is empty provide FMRTNOTFOUND */
    if (Tables[i].fmrtData==NULL)
        return (FMRTNOTFOUND);

    /* This points to the first byte of the structure that contains the given index */
    currentPtr = Tables[i].fmrtData + index*Tables[i].elemSize;

    /* Print first pointers and related data */
    leftPtr = *((fmrtIndex *) currentPtr);
    rightPtr = *((fmrtIndex *) (currentPtr+sizeof(fmrtIndex)));
    printf ("AVL Tree Table %s (Id: %d)\n",Tables[i].tableName, Tables[i].tableId);
    printf ("\tLeft Ptr: %d - Right Ptr: %d\n",leftPtr, rightPtr);

    /* Then print the key */
    printf ("\tKey (%s): ",Tables[i].key.name);
    switch (Tables[i].key.type)
    {
        case FMRTINT:
        {
            printf ("%d\n",*((uint32_t *)(currentPtr+Tables[i].key.delta)));
            break;
        }   /* case FMRTINT */
        case FMRTSIGNED:
        {
            printf ("%d\n",*((int32_t *)(currentPtr+Tables[i].key.delta)));
            break;
        }   /* case FMRTSIGNED */
        case FMRTDOUBLE:
        {
            printf ("%lf\n",*((double *)(currentPtr+Tables[i].key.delta)));
            break;
        }   /* case FMRTDOUBLE */
        case FMRTCHAR:
        {
            printf ("%c\n",*((char *)(currentPtr+Tables[i].key.delta)));
            break;
        }   /* case FMRTCHAR */
        case FMRTSTRING:
        {
            printf ("%s\n",(char *)(currentPtr+Tables[i].key.delta));
            break;
        }   /* case FMRTSTRING */
        case FMRTTIMESTAMP:
        {
            if (fmrtTimeFormat[0]=='\0')
                /* time format empty --> print raw timestamp */
                printf ("%ld\n",*((time_t *)(currentPtr+Tables[i].key.delta)));
            else
            {   /* convert raw timestamp into a string formatted according to fmrtTimeFormat */
                char  timestamp[MAXFMRTSTRINGLEN+1];
                strftime(timestamp, MAXFMRTSTRINGLEN, fmrtTimeFormat, localtime((time_t *)(currentPtr+Tables[i].key.delta)));
                printf ("%s\n",timestamp);
            }
            break;
        }   /* case FMRTTIMESTAMP */
    }   /* switch (Tables[i].key.type) */

    /* Then print all fields */
    for (j=0; j<Tables[i].numFields; j++)
    {
        printf ("\tField %d (%s): ",j,Tables[i].fields[j].name);
        switch (Tables[i].fields[j].type)
        {
            case FMRTINT:
            {
                printf ("%d\n",*((uint32_t *)(currentPtr+Tables[i].fields[j].delta)));
                break;
            }   /* case FMRTINT */
            case FMRTSIGNED:
            {
                printf ("%d\n",*((int32_t *)(currentPtr+Tables[i].fields[j].delta)));
                break;
            }   /* case FMRTSIGNED */
            case FMRTDOUBLE:
            {
                printf ("%lf\n",*((double *)(currentPtr+Tables[i].fields[j].delta)));
                break;
            }   /* case FMRTDOUBLE */
            case FMRTCHAR:
            {
                printf ("%c\n",*((char *)(currentPtr+Tables[i].fields[j].delta)));
                break;
            }   /* case FMRTCHAR */
            case FMRTSTRING:
            {
                printf ("%s\n",(char *)(currentPtr+Tables[i].fields[j].delta));
                break;
            }   /* case FMRTSTRING */
            case FMRTTIMESTAMP:
            {
                if (fmrtTimeFormat[0]=='\0')
                    /* time format empty --> print raw timestamp */
                    printf ("%ld\n",*((time_t *)(currentPtr+Tables[i].fields[j].delta)));
                else
                {   /* convert raw timestamp into a string formatted according to fmrtTimeFormat */
                    char  timestamp[MAXFMRTSTRINGLEN+1];
                    strftime(timestamp, MAXFMRTSTRINGLEN, fmrtTimeFormat, localtime((time_t *)(currentPtr+Tables[i].fields[j].delta)));
                    printf ("%s\n",timestamp);
                }
                break;
            }   /* case FMRTTIMESTAMP */
        }   /* switch (Tables[i].fields[j].type) */
    }   /* for (j=0; j<Tables[i].numFields; j++) */

    return (FMRTOK);
}


/***********************************************************
 * __fmrtDebugPrintTreeRecurse()
 * ---------------------------------------------------------
 * Internal function useful for debugging library
 * It is called by __fmrtDebugPrintTree() to implement
 * recursion
 ***********************************************************/
static void __fmrtDebugPrintTreeRecurse (uint8_t tableIndex, fmrtIndex nodeIndex)
{
    /* Local Variables */
    fmrtIndex    leftIndex, rightIndex;
    void        *currentPtr;

    /* If nodeIndex is NULL stop recursion */
    if (nodeIndex==FMRTNULLPTR)
        return;

    /* Set currentPtr to the first byte of the structure that contains the given index */
    currentPtr = Tables[tableIndex].fmrtData + nodeIndex*Tables[tableIndex].elemSize;

    /* Left and Right subtree indexes */
    leftIndex = *((fmrtIndex *) currentPtr);
    rightIndex = *((fmrtIndex *) (currentPtr+sizeof(fmrtIndex)));

    /* In-order traversal -> First left subtree... */
    __fmrtDebugPrintTreeRecurse (tableIndex,leftIndex);

    /* In-order traversal -> ... then current node... */
    printf ("%s: ",Tables[tableIndex].key.name);
    switch (Tables[tableIndex].key.type)
    {
        case FMRTINT:
        {
            printf ("%d",*((uint32_t *)(currentPtr+Tables[tableIndex].key.delta)));
            break;
        }   /* case FMRTINT */
        case FMRTSIGNED:
        {
            printf ("%d\n",*((int32_t *)(currentPtr+Tables[tableIndex].key.delta)));
            break;
        }   /* case FMRTSIGNED */
        case FMRTDOUBLE:
        {
            printf ("%lf\n",*((double *)(currentPtr+Tables[tableIndex].key.delta)));
            break;
        }   /* case FMRTDOUBLE */
        case FMRTCHAR:
        {
            printf ("%c",*((char *)(currentPtr+Tables[tableIndex].key.delta)));
            break;
        }   /* case FMRTCHAR */
        case FMRTSTRING:
        {
            printf ("%s",(char *)(currentPtr+Tables[tableIndex].key.delta));
            break;
        }   /* case FMRTSTRING */
        case FMRTTIMESTAMP:
        {
            if (fmrtTimeFormat[0]=='\0')
                /* time format empty --> print raw timestamp */
                printf ("%ld",*((time_t *)(currentPtr+Tables[tableIndex].key.delta)));
            else
            {   /* convert raw timestamp into a string formatted according to fmrtTimeFormat */
                char  timestamp[MAXFMRTSTRINGLEN+1];
                strftime(timestamp, MAXFMRTSTRINGLEN, fmrtTimeFormat, localtime((time_t *)(currentPtr+Tables[tableIndex].key.delta)));
                printf ("%s",timestamp);
            }
            break;
        }   /* case FMRTTIMESTAMP */
    }   /* switch (Tables[i].key.type) */
    printf ("\t(Node Index: %d - Left Ptr: %d - Right Ptr: %d)\n",nodeIndex, leftIndex, rightIndex);

    /* In-order traversal -> ... last right subtree */
    __fmrtDebugPrintTreeRecurse (tableIndex,rightIndex);

    return;
}


/***********************************************************
 * __fmrtDebugPrintTree()
 * ---------------------------------------------------------
 * Internal function useful for debugging library
 * The prototype shall be added to the caller
 * When invoked, it prints the whole tree using an in-order
 * approach (left subtree -> node -> right subtree)
 * To achieve this result, it uses the previous function
 * __fmrtDebugPrintTreeRecurse()
 ***********************************************************/
fmrtResult __fmrtDebugPrintTree (fmrtId tableId)
{
    /* Local Variables */
    uint8_t     i;

    /* If this is the first invocation of the library provide error and exit */
    if (fmrtFirstInvocation)
        return (FMRTKO);

    /* Search for the table with given tableId */
    for (i=0; (i<MAXTABLES)&&(Tables[i].tableId != tableId); i++)

    /* If not found provide an error */
    if ( (i==MAXTABLES) || (Tables[i].status==FREE) )
        return (FMRTIDNOTFOUND);

    /* tableId has been found, if the AVL Tree is empty provide FMRTNOTFOUND */
    if (Tables[i].fmrtData==NULL)
        return (FMRTNOTFOUND);

    printf ("AVL Tree Table %s (Id: %d)\n",Tables[i].tableName, Tables[i].tableId);
    if (Tables[i].currentNumElem <= FMRTDEBUGLIMIT)
        __fmrtDebugPrintTreeRecurse (i,Tables[i].fmrtRoot);
    else
        printf ("More than %d elements, skipping printout...\n",FMRTDEBUGLIMIT);

    return (FMRTOK);
}


/***********************************************************
 * __fmrtPrintStack()
 * ---------------------------------------------------------
 * Internal function useful for debugging library
 * It is an internal function invoked by searchElem(), which
 * prints the whole stack built by the latter when traversing
 * the tree. The stack is printed from found node to root
 * (i.e. elements are extracted respecting LIFO order)
 ***********************************************************/
static void __fmrtPrintStack (uint8_t tableIndex, fmrtNodeTraversalStack * ptr)
{
    /* Local variables */
    void        *currentPtr;
    char        go;

    /* If stack is empty exit and stop recursion */
    if (ptr==NULL)
        return;

    /* Set currentPtr to point to the node indexed by the current LIFO element */
    currentPtr = Tables[tableIndex].fmrtData + (ptr->index)*Tables[tableIndex].elemSize;
    /* Print (Key)  (Balance Factor)  (Next Node) */
    printf ("__fmrtPrintStack() --> index: %d (Key: ",ptr->index);
    switch (Tables[tableIndex].key.type)
    {
        case FMRTINT:
        {
            printf ("%d)\t",*((uint32_t *)(currentPtr+Tables[tableIndex].key.delta)));
            break;
        }   /* case FMRTINT */
        case FMRTSIGNED:
        {
            printf ("%d)\n",*((int32_t *)(currentPtr+Tables[tableIndex].key.delta)));
            break;
        }   /* case FMRTSIGNED */
        case FMRTDOUBLE:
        {
            printf ("%lf)\n",*((double *)(currentPtr+Tables[tableIndex].key.delta)));
            break;
        }   /* case FMRTDOUBLE */
        case FMRTCHAR:
        {
            printf ("%c)\t",*((char *)(currentPtr+Tables[tableIndex].key.delta)));
            break;
        }   /* case FMRTCHAR */
        case FMRTSTRING:
        {
            printf ("%s)\t",(char *)(currentPtr+Tables[tableIndex].key.delta));
            break;
        }   /* case FMRTSTRING */
        case FMRTTIMESTAMP:
        {
            if (fmrtTimeFormat[0]=='\0')
                /* time format empty --> print raw timestamp */
                printf ("%ld)\t",*((time_t *)(currentPtr+Tables[tableIndex].key.delta)));
            else
            {   /* convert raw timestamp into a string formatted according to fmrtTimeFormat */
                char  timestamp[MAXFMRTSTRINGLEN+1];
                strftime(timestamp, MAXFMRTSTRINGLEN, fmrtTimeFormat, localtime((time_t *)(currentPtr+Tables[tableIndex].key.delta)));
                printf ("%s)\t",timestamp);
            }
            break;
        }   /* case FMRTTIMESTAMP */
    }   /* switch (Tables[i].key.type) */
    switch (ptr->go)
    {
        case LEFT:
        {
            go = 'L';
            break;
        }
        case RIGHT:
        {
            go = 'R';
            break;
        }
        default:
        {
            go = '-';
            break;
        }
    }   /* switch (ptr->go) */
    printf ("(Next: %c)\n",go);

    /* Go on with the next element in the LIFO */
    __fmrtPrintStack (tableIndex, ptr-> next);

    return ;
}


/***********************************************************
 * __fmrtPrintEmptyList()
 * ---------------------------------------------------------
 * Internal function useful for debugging library
 * It is an internal function invoked whenever an operation
 * is done on the empty items list. It prints the whole list
 * of empty elements.
 ***********************************************************/
void __fmrtPrintEmptyList (uint8_t tableIndex, fmrtIndex node)
{
    /* Local variables */
    void        *currentPtr;
    fmrtIndex    next;

    if (Tables[tableIndex].tableMaxElem > FMRTDEBUGLIMIT)
    {
        printf ("More than %d elements, skipping printout...\n",FMRTDEBUGLIMIT);
        return;
    }

    /* If empty element list is empty exit and stop recursion */
    if (node==FMRTNULLPTR)
        return;

    /* otherwise print current node and call recursively the function */
    currentPtr = Tables[tableIndex].fmrtData + node*Tables[tableIndex].elemSize;
    next = *((fmrtIndex*)currentPtr);
    printf ("Node %d --> Node %d\n",node,next);
    __fmrtPrintEmptyList (tableIndex,next);

    return;
}
#endif

/*******************************
 * Static Functions            *
 * --------------------------- *
 * (only visible in this file) *
 *******************************/
/***********************************************************
 * getEmptyElem()
 * ---------------------------------------------------------
 * Internal function that extracts an empty element from the
 * free element list pointed by fmrtFree and makes it
 * available for inserting it into the FMRT tree
 * ---------------------------------------------------------
 * It returns the index of the free element that has been
 * extracted from the list, or FMRTNULLPTR if no more free
 * elements are available
 ***********************************************************/
static fmrtIndex getEmptyElem (uint8_t tableIndex)
{
    /* Local Variables */
    fmrtIndex    freeElem;
    void        *currentPtr;

    /* If tableIndex does not correspond to a valid table or fmrtData is NULL provide FMRTNULLPTR */
    if ( (Tables[tableIndex].status==FREE) || (Tables[tableIndex].fmrtData==NULL) )
        return (FMRTNULLPTR);

    /* If there are no more free elements in the list provide FMRTNULLPTR */
    if (Tables[tableIndex].fmrtFree == FMRTNULLPTR)
        return (FMRTNULLPTR);

    /* Extract the first element from the list and provide it back */
    freeElem = Tables[tableIndex].fmrtFree;
    currentPtr = Tables[tableIndex].fmrtData + freeElem*Tables[tableIndex].elemSize;
    Tables[tableIndex].fmrtFree = *((fmrtIndex*)currentPtr);

    #ifdef FMRTDEBUG
    printf ("Empty elements list\n");
    printf ("-------------------\n");
    __fmrtPrintEmptyList (tableIndex,Tables[tableIndex].fmrtFree);
    #endif

    return (freeElem);
}


/***********************************************************
 * freeEmptyElem()
 * ---------------------------------------------------------
 * Internal function that inserts the element referenced by
 * index into the free element list pointed by fmrtFree
 * ---------------------------------------------------------
 * Possible return values:
 * - FMRTOK
 *   Function has been executed successfully
 * - FMRTKO
 *   Result obtained when no library calls have been invoked
 *   before
 * - FMRTIDNOTFOUND
 *   tableId is not defined
 ***********************************************************/
static fmrtResult freeEmptyElem (uint8_t tableIndex, fmrtIndex index)
{
    /* Local Variables */
    void        *currentPtr;

    /* If not found provide an error */
    if ( (Tables[tableIndex].status==FREE) )
        return (FMRTIDNOTFOUND);

    /* If initEmptyList() was not called before, provide FMRTKO */
    if (Tables[tableIndex].fmrtData==NULL)
        return (FMRTKO);

    /* Table exists and is indexed by i */
    /* Add element referenced by index in front of the list pointed by fmrtRoot */
    currentPtr = Tables[tableIndex].fmrtData + index*Tables[tableIndex].elemSize;
    *((fmrtIndex*)currentPtr) = Tables[tableIndex].fmrtFree;
    Tables[tableIndex].fmrtFree = index;

    #ifdef FMRTDEBUG
    printf ("Empty elements list\n");
    printf ("-------------------\n");
    __fmrtPrintEmptyList (tableIndex,Tables[tableIndex].fmrtFree);
    #endif

    return (FMRTOK);
}


/***********************************************************
 * initEmptyList()
 * ---------------------------------------------------------
 * Internal function that performs the following actions:
 * - allocate memory (array of tableMaxElem elements of
 *   elemSize bytes)
 * - initialize fmrtRoot pointer (index in the array of the
 *   FMRT tree root node)
 * - initialize fmrtFree pointer (index in the array of the
 *   single linked list that contains free elements
 *   available for being used in the fmrt tree)
 * - link all the array elements through left pointers to
 *   build the single linked list of free elements (the
 *   elements are linked in the following order:
 *   0 -> 1 -> 2 ..... -> tableMaxElem-1 )
 * ---------------------------------------------------------
 * Possible return values:
 * - FMRTOK
 *   Function has been executed successfully
 * - FMRTNOTEMPTY
 *   The table has been already initialized
 * - FMRTOUTOFMEMORY
 *   There is no more space left for allocating memory for
 *   the table
 ***********************************************************/
static fmrtResult initEmptyList (uint8_t i)
{
    /* Local Variables */
    fmrtIndex    leftIndex;
    void        *currentPtr;

    /* If fmrtdata has already been allocated provide FMRTNOTEMPTY */
    if (Tables[i].fmrtData!=NULL)
        return (FMRTNOTEMPTY);

    /* fmrtdata has net been allocated yet - Allocate an array of elements, ... */
    Tables[i].fmrtData = calloc (Tables[i].tableMaxElem,Tables[i].elemSize);
    if (Tables[i].fmrtData==NULL)
        return (FMRTOUTOFMEMORY);

    /* ... initialize indexes and ... */
    Tables[i].fmrtFree = 0;

    /* ... set left pointers of each element to point to the next element in the array  */
    for (leftIndex = 0, currentPtr = Tables[i].fmrtData; leftIndex<Tables[i].tableMaxElem-1; leftIndex++)
    {
        *((fmrtIndex*)currentPtr) = (leftIndex+1) % MAXFMRTELEM;
        currentPtr += Tables[i].elemSize;
    }
    /* last pointer set to null */
    *((fmrtIndex*)currentPtr) = FMRTNULLPTR;

    #ifdef FMRTDEBUG
    printf ("Empty elements list\n");
    printf ("-------------------\n");
    __fmrtPrintEmptyList (i,Tables[i].fmrtFree);
    #endif

    /* Neglecting right pointers, now we have created a single linked list of empty elements  */
    /* (linked by means of the left pointers) that can be extracted and used for storing data */
    return (FMRTOK);
}


/***********************************************************
 * clearNodeTraversalStack()
 * ---------------------------------------------------------
 * Internal function that clears the stack of nodes
 * traversed built by the searchElem() function
 ***********************************************************/
static void clearNodeTraversalStack (fmrtNodeTraversalStack *stackPtr)
{
    /* If input list is empty exit - base scenario */
    if (stackPtr==NULL)
        return;

    /* otherwise start recursion */
    clearNodeTraversalStack(stackPtr->next);
    free (stackPtr);
    return;
}


/***********************************************************
 * searchTable()
 * ---------------------------------------------------------
 * This function is used by fmrt library calls that perform
 * read and write access to the structure.
 * Given a tableId (first parameter) it provides the index in
 * the Table[] structure corresponding to tableId (second
 * parameter).
 * ---------------------------------------------------------
 * Possible Return Values:
 * - FMRTOK
 *   The table has been found and corresponding index
 *   properly set
 * - FMRTKO
 *   Result obtained when this is the first library
 *   call invoked by the caller. Second parameter
 *   is not meaningful
 * - FMRTIDNOTFOUND
 *   tableId is not defined. Second parameter is not
 *   meaningful
 ***********************************************************/
static fmrtResult searchTable (fmrtId tableId, uint8_t *tableIndex)
{
    /* Local variables */
    uint8_t i;

    /* If this is the first invocation of the library provide error */
    if (fmrtFirstInvocation)
        return (FMRTKO);

    /* Search for the table with given tableId */
    for (i=0; (i<MAXTABLES)&&(Tables[i].tableId!=tableId); i++);

    /* If not found provide an error */
    if ( (i==MAXTABLES) || (Tables[i].status==FREE) )
        return (FMRTIDNOTFOUND);

    /* set index as return value in the second parameter */
    *tableIndex = i;

    return (FMRTOK);
}


/***********************************************************
 * searchElem()
 * ---------------------------------------------------------
 * This function is used by fmrt library calls that perform
 * read and write access to the structure.
 * It takes the index of the Table[] array as first parameter
 * and the key to look for in the following parameters.
 * There are six parameters of different types for the key,
 * the function uses only the one corresponding to the key
 * type defined through fmrtDefineKey() (the remaining are not
 * meaningful). The function provides a result code and,
 * in case of result==FMRTOK, also a pointer to a LIFO
 * structure that contains all nodes traversed during the
 * search (last parameter).
 * ---------------------------------------------------------
 * Possible Return Values:
 * - FMRTOK
 *   The entry has been found and the LIFO structure is built.
 *   It contains the set of nodes traversed; the top of the
 *   stack contains the node found
 * - FMRTKO
 *   Result obtained when this is either the first library
 *   call invoked by the caller or the index passed as
 *   first argument exceeds allowed limits or corresponds
 *   to a table not defined
 * - FMRTNOTFOUND
 *   The entry with the given key is not present in the
 *   table. The last parameter is a valid pointer to a LIFO
 *   structure that represents the set of nodes traversed
 ***********************************************************/
static fmrtResult searchElem (uint8_t tableIndex, uint32_t keyInt, int32_t keySigned, double keyDouble, char keyChar, char *keyString, time_t keyTimestamp, fmrtNodeTraversalStack **stackPtr)
{
    /* Local Variables */
    uint8_t     found=0;
    int         cmp;
    fmrtIndex    current;
    void        *currentPtr;
    fmrtNodeTraversalStack *stackElem = NULL;

    /* Pre-initialize return parameters */
    *stackPtr = NULL;

    /* If this is the first invocation of the library provide error */
    if (fmrtFirstInvocation)
        return (FMRTKO);

    /* If either tableIndex is outside allowed limits or the corresponding table is not defined provide error */
    if ( (tableIndex>=MAXTABLES) || (Tables[tableIndex].status==FREE))
        return (FMRTKO);

    /* tableId has been found, if the AVL Tree is empty provide FMRTNOTFOUND */
    if (Tables[tableIndex].fmrtData==NULL)
        return (FMRTNOTFOUND);

    /* The FMRT tree is not empty. Set current to the root index, then start traversing the tree */
    current = Tables[tableIndex].fmrtRoot;
    while ( (found==0) && (current!=FMRTNULLPTR) )
    {   /* currentPtr is set to the first byte of the element indexed by current */
        currentPtr = Tables[tableIndex].fmrtData + current*Tables[tableIndex].elemSize;

        /* Allocate an element to keep track of node traversal into a LIFO structure */
        stackElem = (fmrtNodeTraversalStack *) malloc(sizeof(fmrtNodeTraversalStack));
        stackElem->next = *stackPtr;
        *stackPtr = stackElem;
        (*stackPtr)->index = current;

        /* Check the key pointed by current */
        switch (Tables[tableIndex].key.type)
        {
            case FMRTINT:
            {
                cmp = keyInt - *((uint32_t *)(currentPtr+Tables[tableIndex].key.delta));
                break;
            }   /* case FMRTINT */
            case FMRTSIGNED:
            {
                cmp = keySigned - *((int32_t *)(currentPtr+Tables[tableIndex].key.delta));
                break;
            }   /* case FMRTSIGNED */
            case FMRTDOUBLE:
            {
                cmp = keyDouble - *((double *)(currentPtr+Tables[tableIndex].key.delta));
                break;
            }   /* case FMRTDOUBLE */
            case FMRTCHAR:
            {
                cmp = keyChar - *((char *)(currentPtr+Tables[tableIndex].key.delta));
                break;
            }   /* case FMRTCHAR */
            case FMRTSTRING:
            {
                cmp = strcmp ( keyString, (char *)(currentPtr+Tables[tableIndex].key.delta) );
                break;
            }   /* case FMRTSTRING */
          case FMRTTIMESTAMP:
          {
              cmp = keyTimestamp - *((time_t *)(currentPtr+Tables[tableIndex].key.delta));
              break;
          }   /* case FMRTTIMESTAMP */
        }   /* switch (Tables[i].key.type) */
        if ( cmp==0 )
        {   /* key in the current elem is equal to the key we are looking for */
            found = 1;
            (*stackPtr)->go = STAY;
            continue;
        }
        else if (cmp<0)
        {   /* key we are looking for is less than the one in the current node */
            /* go through the left subtree */
            current = *((fmrtIndex *) currentPtr);
            (*stackPtr)->go = LEFT;
            continue;
        }
        else
        {   /* key we are looking for is greater than the one in the current node */
            /* go through the right subtree */
            current = *((fmrtIndex *) (currentPtr+sizeof(fmrtIndex)));
            (*stackPtr)->go = RIGHT;
            continue;
        }
    }   /* while ( (found==0) && (current!=FMRTNULLPTR) ) */

    #ifdef FMRTDEBUG
    printf ("Node traversal stack\n");
    printf ("--------------------\n");
    __fmrtPrintStack (tableIndex, *stackPtr);
    #endif

    /* If the element was not found, then found == 0 */
    if (found==0)
        return (FMRTNOTFOUND);
    else
        return (FMRTOK);
}


/***********************************************************
 * countSubtreeNodes()
 * ---------------------------------------------------------
 * This function is used to count the number of nodes of
 * the subtree whose root node is given by the second
 * parameter.
 * The function is implemented through direct recursion
 * ---------------------------------------------------------
 * It returns the number of nodes including the root
 ***********************************************************/
static int8_t countSubtreeNodes (uint8_t tableIndex, fmrtIndex node)
{
    /* Local variables */
    void        *currentPtr;
    fmrtIndex    leftIndex, rightIndex;
    int8_t      leftNodes=0, rightNodes=0;

    /* If fmrtIndex is NULL exit */
    if (node==FMRTNULLPTR)
        return (0);

    /* fmrtIndex is not NULL, evaluate currentPtr and Left and Right subtree indexes */
    currentPtr = Tables[tableIndex].fmrtData + node*Tables[tableIndex].elemSize;
    leftIndex = *((fmrtIndex *) currentPtr);
    rightIndex = *((fmrtIndex *) (currentPtr+sizeof(fmrtIndex)));

    /* If the node is a leaf provide 1 */
    if ( (leftIndex==FMRTNULLPTR) && (rightIndex==FMRTNULLPTR) )
        return (1);

    if (leftIndex!=FMRTNULLPTR)
        leftNodes = countSubtreeNodes(tableIndex,leftIndex);
    if (rightIndex!=FMRTNULLPTR)
        rightNodes = countSubtreeNodes(tableIndex,rightIndex);

    return (1+leftNodes+rightNodes);
}


/***********************************************************
 * nodeHeight()
 * ---------------------------------------------------------
 * This function is used by fmrt library calls that perform
 * write access to the structure.
 * Given a tableId (first parameter) and the fmrtIndex of a
 * node (second parameter), it provides the corresponding
 * height, defined as:
 *    1+max[height(leftsubtree),height(rightsubtree)]
 * (where leafs have height 0 by definition).
 * The function is implemented through direct recursion
 * ---------------------------------------------------------
 * It returns the height
 ***********************************************************/
static int8_t nodeHeight (uint8_t tableIndex, fmrtIndex node)
{
    /* Local variables */
    void        *currentPtr;
    fmrtIndex    leftIndex, rightIndex;
    int8_t      leftHeight=-1,
                rightHeight=-1;

    /* If fmrtIndex is NULL exit */
    if (node==FMRTNULLPTR)
        return (-1);

    /* fmrtIndex is not NULL, evaluate currentPtr and Left and Right subtree indexes */
    currentPtr = Tables[tableIndex].fmrtData + node*Tables[tableIndex].elemSize;
    leftIndex = *((fmrtIndex *) currentPtr);
    rightIndex = *((fmrtIndex *) (currentPtr+sizeof(fmrtIndex)));

    leftHeight = nodeHeight(tableIndex,leftIndex);
    rightHeight = nodeHeight(tableIndex,rightIndex);

    return ( 1+((leftHeight>rightHeight)?leftHeight:rightHeight) );
}


/***********************************************************
 * rotateLeft()
 * ---------------------------------------------------------
 * This function performs left rotation on the subtree whose
 * root node is given by the second parameter.
 * ---------------------------------------------------------
 * It returns the index of the subtree after rotation
 ***********************************************************/
static fmrtIndex rotateLeft (uint8_t tableIndex, fmrtIndex index)
{
    /* Local variables */
    fmrtIndex    index1,
                index2;
    void        *ptr,
                *ptr1;

    /* If index is NULL exit without actions */
    if (index==FMRTNULLPTR)
        return (FMRTNULLPTR);

    /* Set ptr to the first byte of the structure that contains the given index */
    ptr = Tables[tableIndex].fmrtData + index*Tables[tableIndex].elemSize;

    /* index1(ptr1) is the right child of the node pointed by index(ptr) */
    index1 = *((fmrtIndex *) (ptr+sizeof(fmrtIndex)));
    ptr1 = Tables[tableIndex].fmrtData + index1*Tables[tableIndex].elemSize;

    /* index2(ptr2) is the left chid of index1(ptr1) */
    index2 = *((fmrtIndex *) ptr1);

    /* rotate left... */
    /* let index(ptr) become left subtree of index1(ptr1)*/
    *((fmrtIndex *) ptr1) = index;
    /* then left subtree of index(ptr) points to index2(ptr2) */
    *((fmrtIndex *) (ptr+sizeof(fmrtIndex))) = index2;

    /* the new root is index1(ptr1) */
    return (index1);
}


/***********************************************************
 * rotateRight()
 * ---------------------------------------------------------
 * This function performs right rotation on the subtree whose
 * root node is given by the second parameter.
 * ---------------------------------------------------------
 * It returns the index of the subtree after rotation
 ***********************************************************/
static fmrtIndex rotateRight (uint8_t tableIndex, fmrtIndex index)
{
    /* Local variables */
    fmrtIndex    index1,
                index2;
    void        *ptr,
                *ptr1;

    /* If index is NULL exit without actions */
    if (index==FMRTNULLPTR)
        return (FMRTNULLPTR);

    /* Set ptr to the first byte of the structure that contains the given index */
    ptr = Tables[tableIndex].fmrtData + index*Tables[tableIndex].elemSize;

    /* index1(ptr1) is the left child of the node pointed by index(ptr) */
    index1 = *((fmrtIndex *) ptr);
    ptr1 = Tables[tableIndex].fmrtData + index1*Tables[tableIndex].elemSize;

    /* index2(ptr2) is the right chid of index1(ptr1) */
    index2 = *((fmrtIndex *) (ptr1+sizeof(fmrtIndex)));

    /* rotate right... */
    /* let index(ptr) become right subtree of index1(ptr1)*/
    *((fmrtIndex *) (ptr1+sizeof(fmrtIndex))) = index;
    /* then left subtree of index(ptr) points to index2(ptr2) */
    *((fmrtIndex *) ptr) = index2;

    /* the new root is index1(ptr1) */
    return (index1);
}


/***********************************************************
 * rebalanceSubTree()
 * ---------------------------------------------------------
 * This function is used to rebalance a subtree whose index
 * is given as second parameter. The first parameter is the
 * index of the table in the Table[] array.
 * Balance factor is defined here as:
 *    BF = height(right subtree) - height(left subtree)
 * (if BF>0 subtree on the right has an higher height
 *  vs left subtree and vice versa. In some papers/articles
 *  the opposite definition is assumed)
 * ---------------------------------------------------------
 * It returns the fmrtIndex pointer of the re-balanced tree
 ***********************************************************/
static fmrtIndex rebalanceSubTree (uint8_t tableIndex, fmrtIndex nodeIndex)
{
    /* Local variables */
    int8_t      balance,
                lefthgt,
                righthgt;
    fmrtIndex    leftIndex,
                rightIndex,
                workIndex,
                leftsubtree,
                rightsubtree;
    void        *currentPtr,
                *subtreePtr;

    /* If nodeIndex is NULL exit without actions */
    if (nodeIndex==FMRTNULLPTR)
        return (FMRTNULLPTR);

    /* Set currentPtr to the first byte of the structure that contains the given index */
    currentPtr = Tables[tableIndex].fmrtData + nodeIndex*Tables[tableIndex].elemSize;

    /* Left, Right and current subtree indexes */
    leftIndex = *((fmrtIndex *) currentPtr);
    rightIndex = *((fmrtIndex *) (currentPtr+sizeof(fmrtIndex)));
    workIndex = nodeIndex;

    /* evaluate balance factor of current node */
    righthgt = nodeHeight(tableIndex,rightIndex);
    lefthgt = nodeHeight(tableIndex,leftIndex);
    balance = righthgt-lefthgt;

    if (balance>1)
    {   /* the subtree whose root is nodeIndex=workIndex is unbalanced -> right subtree has higher height */
        /* Evaluate heights on the right and left subtrees of the right child                             */
        /* Since (balance>=2)   ==>   height(right subtree)>=2   ==>   rightIndex!=FMRTNULLPTR             */
        subtreePtr = Tables[tableIndex].fmrtData + rightIndex*Tables[tableIndex].elemSize;
        leftsubtree = *((fmrtIndex *) subtreePtr);
        rightsubtree = *((fmrtIndex *) (subtreePtr+sizeof(fmrtIndex)));

        if ( nodeHeight(tableIndex,rightsubtree)>nodeHeight(tableIndex,leftsubtree) )
        {   /* if right subtree of the right child has highest height simply rotate left */
            workIndex = rotateLeft(tableIndex,nodeIndex);
        }
        else
        {   /* otherwise we have to combine right and left rotation */
            *((fmrtIndex *) (currentPtr+sizeof(fmrtIndex))) = rotateRight(tableIndex,rightIndex);
            workIndex = rotateLeft(tableIndex,nodeIndex);
        }
        return (workIndex);
    }   /* if (balance>1) */

    if (balance<-1)
    {   /* the subtree whose root is nodeIndex=workIndex is unbalanced -> left subtree has higher height */
        /* Evaluate heights on the right and left subtrees of the left child                             */
        /* Since (balance<=-2)   ==>   height(left subtree)>=2   ==>   leftIndex!=FMRTNULLPTR             */
        subtreePtr = Tables[tableIndex].fmrtData + leftIndex*Tables[tableIndex].elemSize;
        leftsubtree = *((fmrtIndex *) subtreePtr);
        rightsubtree = *((fmrtIndex *) (subtreePtr+sizeof(fmrtIndex)));
        if ( nodeHeight(tableIndex,leftsubtree)>nodeHeight(tableIndex,rightsubtree) )
        {   /* if left subtree of the left child has highest height simply rotate right */
            workIndex = rotateRight(tableIndex,nodeIndex);
        }
        else
        {   /* otherwise we have to combine left and Right rotation */
            *((fmrtIndex *) (currentPtr) ) = rotateLeft(tableIndex,leftIndex);;
            workIndex = rotateRight(tableIndex,nodeIndex);
        }
        return (workIndex);
    }   /* if (balance<-1) */

    return (workIndex);

}


/***********************************************************
 * leftMostChild()
 * ---------------------------------------------------------
 * This function is used by fmrtDelete() library call. Given
 * a specified table (whose index is provided by the first
 * parameter) and a node index (second parameter), it looks
 * for the leftmost child of that node, i.e. the leaf that
 * can be reached by descending the tree only on the left
 * subtree. The routine provides back the fmrtIndex of such
 * node along with a LIFO structure that represents the
 * path to it
 * ---------------------------------------------------------
 * It returns the fmrtIndex pointer of the leftmost child
 ***********************************************************/
static fmrtIndex leftMostChild (uint8_t tableIndex, fmrtIndex index, fmrtNodeTraversalStack **stackPtr)
{
    /* Local Variables */
    fmrtIndex    current, leftmost;
    void        *currentPtr;
    fmrtNodeTraversalStack *stackElem = NULL;

    /* Pre-initialize return parameters */
    *stackPtr = NULL;

    /* The FMRT tree is not empty. Set current to the root index, then start traversing the tree */
    current = leftmost = index;
    while (current != FMRTNULLPTR)
    {
        currentPtr = Tables[tableIndex].fmrtData + current*Tables[tableIndex].elemSize;

        /* Allocate an element to keep track of node traversal into a LIFO structure */
        stackElem = (fmrtNodeTraversalStack *) malloc(sizeof(fmrtNodeTraversalStack));
        stackElem->next = *stackPtr;
        *stackPtr = stackElem;
        (*stackPtr)->index = current;
        (*stackPtr)->go = LEFT;

        leftmost = current;
        current = *((fmrtIndex *) currentPtr);
    }   /* while (current != FMRTNULLPTR) */

    return (leftmost);
}


/***********************************************************
 * copyNode()
 * ---------------------------------------------------------
 * This function is used by fmrtDelete() library call. Given
 * a specified table (whose index is provided by the first
 * parameter) it copies all the data from source node (third
 * parameter) to the destination node (second parameter).
 * Data means the key alomg with all relevant fields.
 * ---------------------------------------------------------
 * It returns the fmrtIndex pointer of the leftmost child
 ***********************************************************/
static void copyNode (uint8_t tableIndex, fmrtIndex toIndex, fmrtIndex fromIndex)
{
    /* Local Variables */
    void        *fromPtr,
                *toPtr;
    int         numBytes;

    if ( (fromIndex==FMRTNULLPTR) || (toIndex==FMRTNULLPTR) )
        return;

    fromPtr = Tables[tableIndex].fmrtData + fromIndex*Tables[tableIndex].elemSize+2*sizeof(fmrtIndex);
    toPtr = Tables[tableIndex].fmrtData + toIndex*Tables[tableIndex].elemSize+2*sizeof(fmrtIndex);
    numBytes = Tables[tableIndex].elemSize - 2*sizeof(fmrtIndex);

    memcpy (toPtr, fromPtr, numBytes);

    return;

}


/***********************************************************
 * exportTableRecurse()
 * ---------------------------------------------------------
 * This function is used by the fmrt library call
 * fmrtExportTableCsv(), and implements a recursive in-order
 * traversal of the fmrt tree.
 * It takes the table index (first parameter, which is the
 * index of the Table[] array, not the TableId), the
 * current node index (second parameter), a file pointer
 * (third parameter) and a char representing a user defined
 * separator between fields. The routine consider the
 * index as the root of a subtree which is printed into
 * the file pointed by the third parameter by using
 * in order approach.
 ***********************************************************/
static void exportTableRecurse (uint8_t tableIndex, fmrtIndex nodeIndex, FILE *fPtr, char sep, uint8_t reverse)
{
    /* Local Variables */
    fmrtIndex    leftIndex, rightIndex;
    uint8_t     j;
    void        *currentPtr;

    /* If nodeIndex is NULL stop recursion */
    if (nodeIndex==FMRTNULLPTR)
        return;

    /* Set currentPtr to the first byte of the structure that contains the given index */
    currentPtr = Tables[tableIndex].fmrtData + nodeIndex*Tables[tableIndex].elemSize;

    /* Left and Right subtree indexes */
    leftIndex = *((fmrtIndex *) currentPtr);
    rightIndex = *((fmrtIndex *) (currentPtr+sizeof(fmrtIndex)));

    /* In-order traversal -> First left subtree (in case reverse==0, right subtree otherwise)... */
    if (reverse==0)
        exportTableRecurse (tableIndex, leftIndex, fPtr, sep, reverse);
    else
        exportTableRecurse (tableIndex, rightIndex, fPtr, sep, reverse);

    /* In-order traversal -> ... then current node... */
    /* Print the key... */
    switch (Tables[tableIndex].key.type)
    {
        case FMRTINT:
        {
            fprintf (fPtr, "%d",*((uint32_t *)(currentPtr+Tables[tableIndex].key.delta)));
            break;
        }   /* case FMRTINT */
        case FMRTSIGNED:
        {
            fprintf (fPtr, "%d",*((int32_t *)(currentPtr+Tables[tableIndex].key.delta)));
            break;
        }   /* case FMRTSIGNED */
        case FMRTDOUBLE:
        {
            fprintf (fPtr, "%lf",*((double *)(currentPtr+Tables[tableIndex].key.delta)));
            break;
        }   /* case FMRTDOUBLE */
        case FMRTCHAR:
        {
            fprintf (fPtr, "%c",*((char *)(currentPtr+Tables[tableIndex].key.delta)));
            break;
        }   /* case FMRTCHAR */
        case FMRTSTRING:
        {
            fprintf (fPtr, "%s",(char *)(currentPtr+Tables[tableIndex].key.delta));
            break;
        }   /* case FMRTSTRING */
        case FMRTTIMESTAMP:
        {
            if (fmrtTimeFormat[0]=='\0')
                /* time format empty --> print raw timestamp */
                fprintf (fPtr, "%ld",*((time_t *)(currentPtr+Tables[tableIndex].key.delta)));
            else
            {   /* convert raw timestamp into a string formatted according to fmrtTimeFormat */
                char  timestamp[MAXFMRTSTRINGLEN+1];
                strftime(timestamp, MAXFMRTSTRINGLEN, fmrtTimeFormat, localtime((time_t *)(currentPtr+Tables[tableIndex].key.delta)));
                fprintf (fPtr, "%s",timestamp);
            }
            break;
        }   /* case FMRTTIMESTAMP */
    }   /* switch (Tables[i].key.type) */
    /* then loop through all the fields and print them separated by sep */
    for (j=0; j<Tables[tableIndex].numFields; j++)
    {   /* Loop through all fields */
        switch (Tables[tableIndex].fields[j].type)
        {
            case FMRTINT:
            {
                fprintf (fPtr, "%c%d",sep,*((uint32_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTINT */
            case FMRTSIGNED:
            {
                fprintf (fPtr, "%c%d",sep,*((int32_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTSIGNED */
            case FMRTDOUBLE:
            {
                fprintf (fPtr, "%c%lf",sep,*((double *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTDOUBLE */
            case FMRTCHAR:
            {
                fprintf (fPtr, "%c%c",sep,*((char *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTCHAR */
            case FMRTSTRING:
            {
                fprintf (fPtr, "%c%s",sep,(char *)(currentPtr+Tables[tableIndex].fields[j].delta));
                break;
            }   /* case FMRTSTRING */
            case FMRTTIMESTAMP:
            {
                if (fmrtTimeFormat[0]=='\0')
                    /* time format empty --> print raw timestamp */
                    fprintf (fPtr, "%c%ld",sep,*((time_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                else
                {   /* convert raw timestamp into a string formatted according to fmrtTimeFormat */
                    char  timestamp[MAXFMRTSTRINGLEN+1];
                    strftime(timestamp, MAXFMRTSTRINGLEN, fmrtTimeFormat, localtime((time_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                    fprintf (fPtr, "%c%s",sep,timestamp);
                }
                break;
            }   /* case FMRTTIMESTAMP */
        }   /* switch (Tables[i].fields[j].type) */
    }   /* for (j=0; j<Tables[i].numFields; j++) */
    fprintf (fPtr,"\n");

    /* In-order traversal -> ... last right subtree (in case reverse==0, left subtree otherwise)*/
    if (reverse==0)
        exportTableRecurse (tableIndex, rightIndex, fPtr, sep, reverse);
    else
        exportTableRecurse (tableIndex, leftIndex, fPtr, sep, reverse);

    return;
}


/***********************************************************
 * exportRangeRecurseInt()
 * ---------------------------------------------------------
 * This function is used by the fmrt library call
 * fmrtExportRangeCsv(), and implements a recursive in-order
 * traversal of the fmrt tree.
 * It takes the table index (first parameter, which is the
 * index of the Table[] array, not the TableId), the
 * current node index (second parameter), a file pointer
 * (third parameter), a char representing a user defined
 * separator between fields and two integer values
 * representing respectively the minimum and maximum key
 * value. The routine consider the index as the root of a
 * subtree which is printed into the file pointed by the
 * third parameter by using in order approach.
 ***********************************************************/
static void exportRangeRecurseInt (uint8_t tableIndex, fmrtIndex nodeIndex, FILE *fPtr, char sep, uint8_t reverse, uint32_t keyMin, uint32_t keyMax)
{
    /* Local Variables */
    fmrtIndex    leftIndex, rightIndex;
    uint8_t     j;
    uint32_t    key;
    void        *currentPtr;

    /* If nodeIndex is NULL stop recursion */
    if (nodeIndex==FMRTNULLPTR)
        return;

    /* Set currentPtr to the first byte of the structure that contains the given index */
    currentPtr = Tables[tableIndex].fmrtData + nodeIndex*Tables[tableIndex].elemSize;

    /* Extract Key, Left and Right subtree indexes */
    key = *((uint32_t *)(currentPtr+Tables[tableIndex].key.delta));
    leftIndex = *((fmrtIndex *) currentPtr);
    rightIndex = *((fmrtIndex *) (currentPtr+sizeof(fmrtIndex)));

    /* In-order traversal -> First left subtree (in case reverse==0, right subtree otherwise)... */
    /* ... but skip unnecessary node traversal in case the current key is outside the range      */
    if (key<keyMin)
    {   /* current element is lower than keyMin -> explore only right subtree */
        exportRangeRecurseInt (tableIndex, rightIndex, fPtr, sep, reverse, keyMin, keyMax);
        return;
    }
    if (key>keyMax)
    {   /* current element is higher than keyMax -> explore only left subtree */
        exportRangeRecurseInt (tableIndex, leftIndex, fPtr, sep, reverse, keyMin, keyMax);
        return;
    }
    /* Current key is within the interval... explore both subtrees, with */
    /* order depending on reverse parameter                              */
    if (reverse==0)
        exportRangeRecurseInt (tableIndex, leftIndex, fPtr, sep, reverse, keyMin, keyMax);
    else
        exportRangeRecurseInt (tableIndex, rightIndex, fPtr, sep, reverse, keyMin, keyMax);

    /* In-order traversal -> ... after subtree print current node... */
    /* Print the key... */
    fprintf (fPtr, "%d",key);
    /* then loop through all the fields and print them separated by sep */
    for (j=0; j<Tables[tableIndex].numFields; j++)
    {   /* Loop through all fields */
        switch (Tables[tableIndex].fields[j].type)
        {
            case FMRTINT:
            {
                fprintf (fPtr, "%c%d",sep,*((uint32_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTINT */
            case FMRTSIGNED:
            {
                fprintf (fPtr, "%c%d",sep,*((int32_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTSIGNED */
            case FMRTDOUBLE:
            {
                fprintf (fPtr, "%c%lf",sep,*((double *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTDOUBLE */
            case FMRTCHAR:
            {
                fprintf (fPtr, "%c%c",sep,*((char *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTCHAR */
            case FMRTSTRING:
            {
                fprintf (fPtr, "%c%s",sep,(char *)(currentPtr+Tables[tableIndex].fields[j].delta));
                break;
            }   /* case FMRTSTRING */
            case FMRTTIMESTAMP:
            {
                if (fmrtTimeFormat[0]=='\0')
                    /* time format empty --> print raw timestamp */
                    fprintf (fPtr, "%c%ld",sep,*((time_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                else
                {   /* convert raw timestamp into a string formatted according to fmrtTimeFormat */
                    char  timestamp[MAXFMRTSTRINGLEN+1];
                    strftime(timestamp, MAXFMRTSTRINGLEN, fmrtTimeFormat, localtime((time_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                    fprintf (fPtr, "%c%s",sep,timestamp);
                }
                break;
            }   /* case FMRTTIMESTAMP */
        }   /* switch (Tables[i].fields[j].type) */
    }   /* for (j=0; j<Tables[i].numFields; j++) */
    fprintf (fPtr,"\n");

    /* In-order traversal -> ... last right subtree (in case reverse==0, left subtree otherwise)*/
    if (reverse==0)
        exportRangeRecurseInt (tableIndex, rightIndex, fPtr, sep, reverse, keyMin, keyMax);
    else
        exportRangeRecurseInt (tableIndex, leftIndex, fPtr, sep, reverse, keyMin, keyMax);

    return;
}


/***********************************************************
 * exportRangeRecurseSigned()
 * ---------------------------------------------------------
 * This function is used by the fmrt library call
 * fmrtExportRangeCsv(), and implements a recursive in-order
 * traversal of the fmrt tree.
 * It takes the table index (first parameter, which is the
 * index of the Table[] array, not the TableId), the
 * current node index (second parameter), a file pointer
 * (third parameter), a char representing a user defined
 * separator between fields and two signed integer values
 * representing respectively the minimum and maximum key
 * value. The routine consider the index as the root of a
 * subtree which is printed into the file pointed by the
 * third parameter by using in order approach.
 ***********************************************************/
static void exportRangeRecurseSigned (uint8_t tableIndex, fmrtIndex nodeIndex, FILE *fPtr, char sep, uint8_t reverse, int32_t keyMin, int32_t keyMax)
{
    /* Local Variables */
    fmrtIndex   leftIndex, rightIndex;
    uint8_t     j;
    int32_t     key;
    void       *currentPtr;

    /* If nodeIndex is NULL stop recursion */
    if (nodeIndex==FMRTNULLPTR)
        return;

    /* Set currentPtr to the first byte of the structure that contains the given index */
    currentPtr = Tables[tableIndex].fmrtData + nodeIndex*Tables[tableIndex].elemSize;

    /* Extract Key, Left and Right subtree indexes */
    key = *((int32_t *)(currentPtr+Tables[tableIndex].key.delta));
    leftIndex = *((fmrtIndex *) currentPtr);
    rightIndex = *((fmrtIndex *) (currentPtr+sizeof(fmrtIndex)));

    /* In-order traversal -> First left subtree (in case reverse==0, right subtree otherwise)... */
    /* ... but skip unnecessary node traversal in case the current key is outside the range      */
    if (key<keyMin)
    {   /* current element is lower than keyMin -> explore only right subtree */
        exportRangeRecurseSigned (tableIndex, rightIndex, fPtr, sep, reverse, keyMin, keyMax);
        return;
    }
    if (key>keyMax)
    {   /* current element is higher than keyMax -> explore only left subtree */
        exportRangeRecurseSigned (tableIndex, leftIndex, fPtr, sep, reverse, keyMin, keyMax);
        return;
    }
    /* Current key is within the interval... explore both subtrees, with */
    /* order depending on reverse parameter                              */
    if (reverse==0)
        exportRangeRecurseSigned (tableIndex, leftIndex, fPtr, sep, reverse, keyMin, keyMax);
    else
        exportRangeRecurseSigned (tableIndex, rightIndex, fPtr, sep, reverse, keyMin, keyMax);

    /* In-order traversal -> ... after subtree print current node... */
    /* Print the key... */
    fprintf (fPtr, "%d",key);
    /* then loop through all the fields and print them separated by sep */
    for (j=0; j<Tables[tableIndex].numFields; j++)
    {   /* Loop through all fields */
        switch (Tables[tableIndex].fields[j].type)
        {
            case FMRTINT:
            {
                fprintf (fPtr, "%c%d",sep,*((uint32_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTINT */
            case FMRTSIGNED:
            {
                fprintf (fPtr, "%c%d",sep,*((int32_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTSIGNED */
            case FMRTDOUBLE:
            {
                fprintf (fPtr, "%c%lf",sep,*((double *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTDOUBLE */
            case FMRTCHAR:
            {
                fprintf (fPtr, "%c%c",sep,*((char *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTCHAR */
            case FMRTSTRING:
            {
                fprintf (fPtr, "%c%s",sep,(char *)(currentPtr+Tables[tableIndex].fields[j].delta));
                break;
            }   /* case FMRTSTRING */
            case FMRTTIMESTAMP:
            {
                if (fmrtTimeFormat[0]=='\0')
                    /* time format empty --> print raw timestamp */
                    fprintf (fPtr, "%c%ld",sep,*((time_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                else
                {   /* convert raw timestamp into a string formatted according to fmrtTimeFormat */
                    char  timestamp[MAXFMRTSTRINGLEN+1];
                    strftime(timestamp, MAXFMRTSTRINGLEN, fmrtTimeFormat, localtime((time_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                    fprintf (fPtr, "%c%s",sep,timestamp);
                }
                break;
            }   /* case FMRTTIMESTAMP */
        }   /* switch (Tables[i].fields[j].type) */
    }   /* for (j=0; j<Tables[i].numFields; j++) */
    fprintf (fPtr,"\n");

    /* In-order traversal -> ... last right subtree (in case reverse==0, left subtree otherwise)*/
    if (reverse==0)
        exportRangeRecurseSigned (tableIndex, rightIndex, fPtr, sep, reverse, keyMin, keyMax);
    else
        exportRangeRecurseSigned (tableIndex, leftIndex, fPtr, sep, reverse, keyMin, keyMax);

    return;
}


/***********************************************************
 * exportRangeRecurseDouble()
 * ---------------------------------------------------------
 * This function is used by the fmrt library call
 * fmrtExportRangeCsv(), and implements a recursive in-order
 * traversal of the fmrt tree.
 * It takes the table index (first parameter, which is the
 * index of the Table[] array, not the TableId), the
 * current node index (second parameter), a file pointer
 * (third parameter), a char representing a user defined
 * separator between fields and two double values
 * representing respectively the minimum and maximum key
 * value. The routine consider the index as the root of a
 * subtree which is printed into the file pointed by the
 * third parameter by using in order approach.
 ***********************************************************/
static void exportRangeRecurseDouble (uint8_t tableIndex, fmrtIndex nodeIndex, FILE *fPtr, char sep, uint8_t reverse, double keyMin, double keyMax)
{
    /* Local Variables */
    fmrtIndex   leftIndex, rightIndex;
    uint8_t     j;
    double      key;
    void       *currentPtr;

    /* If nodeIndex is NULL stop recursion */
    if (nodeIndex==FMRTNULLPTR)
        return;

    /* Set currentPtr to the first byte of the structure that contains the given index */
    currentPtr = Tables[tableIndex].fmrtData + nodeIndex*Tables[tableIndex].elemSize;

    /* Extract Key, Left and Right subtree indexes */
    key = *((double *)(currentPtr+Tables[tableIndex].key.delta));
    leftIndex = *((fmrtIndex *) currentPtr);
    rightIndex = *((fmrtIndex *) (currentPtr+sizeof(fmrtIndex)));

    /* In-order traversal -> First left subtree (in case reverse==0, right subtree otherwise)... */
    /* ... but skip unnecessary node traversal in case the current key is outside the range      */
    if (key<keyMin)
    {   /* current element is lower than keyMin -> explore only right subtree */
        exportRangeRecurseDouble (tableIndex, rightIndex, fPtr, sep, reverse, keyMin, keyMax);
        return;
    }
    if (key>keyMax)
    {   /* current element is higher than keyMax -> explore only left subtree */
        exportRangeRecurseDouble (tableIndex, leftIndex, fPtr, sep, reverse, keyMin, keyMax);
        return;
    }
    /* Current key is within the interval... explore both subtrees, with */
    /* order depending on reverse parameter                              */
    if (reverse==0)
        exportRangeRecurseDouble (tableIndex, leftIndex, fPtr, sep, reverse, keyMin, keyMax);
    else
        exportRangeRecurseDouble (tableIndex, rightIndex, fPtr, sep, reverse, keyMin, keyMax);

    /* In-order traversal -> ... after subtree print current node... */
    /* Print the key... */
    fprintf (fPtr, "%lf",key);
    /* then loop through all the fields and print them separated by sep */
    for (j=0; j<Tables[tableIndex].numFields; j++)
    {   /* Loop through all fields */
        switch (Tables[tableIndex].fields[j].type)
        {
            case FMRTINT:
            {
                fprintf (fPtr, "%c%d",sep,*((uint32_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTINT */
            case FMRTSIGNED:
            {
                fprintf (fPtr, "%c%d",sep,*((int32_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTSIGNED */
            case FMRTDOUBLE:
            {
                fprintf (fPtr, "%c%lf",sep,*((double *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTDOUBLE */
            case FMRTCHAR:
            {
                fprintf (fPtr, "%c%c",sep,*((char *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTCHAR */
            case FMRTSTRING:
            {
                fprintf (fPtr, "%c%s",sep,(char *)(currentPtr+Tables[tableIndex].fields[j].delta));
                break;
            }   /* case FMRTSTRING */
            case FMRTTIMESTAMP:
            {
                if (fmrtTimeFormat[0]=='\0')
                    /* time format empty --> print raw timestamp */
                    fprintf (fPtr, "%c%ld",sep,*((time_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                else
                {   /* convert raw timestamp into a string formatted according to fmrtTimeFormat */
                    char  timestamp[MAXFMRTSTRINGLEN+1];
                    strftime(timestamp, MAXFMRTSTRINGLEN, fmrtTimeFormat, localtime((time_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                    fprintf (fPtr, "%c%s",sep,timestamp);
                }
                break;
            }   /* case FMRTTIMESTAMP */
        }   /* switch (Tables[i].fields[j].type) */
    }   /* for (j=0; j<Tables[i].numFields; j++) */
    fprintf (fPtr,"\n");

    /* In-order traversal -> ... last right subtree (in case reverse==0, left subtree otherwise)*/
    if (reverse==0)
        exportRangeRecurseDouble (tableIndex, rightIndex, fPtr, sep, reverse, keyMin, keyMax);
    else
        exportRangeRecurseDouble (tableIndex, leftIndex, fPtr, sep, reverse, keyMin, keyMax);

    return;
}


/***********************************************************
 * exportRangeRecurseChar()
 * ---------------------------------------------------------
 * This function is used by the fmrt library call
 * fmrtExportRangeCsv(), and implements a recursive in-order
 * traversal of the fmrt tree.
 * It takes the table index (first parameter, which is the
 * index of the Table[] array, not the TableId), the
 * current node index (second parameter), a file pointer
 * (third parameter), a char representing a user defined
 * separator between fields and two char values
 * representing respectively the minimum and maximum key
 * value. The routine consider the index as the root of a
 * subtree which is printed into the file pointed by the
 * third parameter by using in order approach.
 ***********************************************************/
static void exportRangeRecurseChar (uint8_t tableIndex, fmrtIndex nodeIndex, FILE *fPtr, char sep, uint8_t reverse, char keyMin, char keyMax)
{
    /* Local Variables */
    fmrtIndex    leftIndex, rightIndex;
    uint8_t     j;
    char        key;
    void        *currentPtr;

    /* If nodeIndex is NULL stop recursion */
    if (nodeIndex==FMRTNULLPTR)
        return;

    /* Set currentPtr to the first byte of the structure that contains the given index */
    currentPtr = Tables[tableIndex].fmrtData + nodeIndex*Tables[tableIndex].elemSize;

    /* Extract Key, Left and Right subtree indexes */
    key = *((char *)(currentPtr+Tables[tableIndex].key.delta));
    leftIndex = *((fmrtIndex *) currentPtr);
    rightIndex = *((fmrtIndex *) (currentPtr+sizeof(fmrtIndex)));

    /* In-order traversal -> First left subtree (in case reverse==0, right subtree otherwise)... */
    /* ... but skip unnecessary node traversal in case the current key is outside the range      */
    if (key<keyMin)
    {   /* current element is lower than keyMin -> explore only right subtree */
        exportRangeRecurseChar (tableIndex, rightIndex, fPtr, sep, reverse, keyMin, keyMax);
        return;
    }
    if (key>keyMax)
    {   /* current element is higher than keyMax -> explore only left subtree */
        exportRangeRecurseChar (tableIndex, leftIndex, fPtr, sep, reverse, keyMin, keyMax);
        return;
    }
    /* Current key is within the interval... explore both subtrees, with */
    /* order depending on reverse parameter                              */
    if (reverse==0)
        exportRangeRecurseChar (tableIndex, leftIndex, fPtr, sep, reverse, keyMin, keyMax);
    else
        exportRangeRecurseChar (tableIndex, rightIndex, fPtr, sep, reverse, keyMin, keyMax);

    /* In-order traversal -> ... after subtree print current node... */
    /* Print the key... */
    fprintf (fPtr, "%c",key);
    /* then loop through all the fields and print them separated by sep */
    for (j=0; j<Tables[tableIndex].numFields; j++)
    {   /* Loop through all fields */
        switch (Tables[tableIndex].fields[j].type)
        {
            case FMRTINT:
            {
                fprintf (fPtr, "%c%d",sep,*((uint32_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTINT */
            case FMRTSIGNED:
            {
                fprintf (fPtr, "%c%d",sep,*((int32_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTSIGNED */
            case FMRTDOUBLE:
            {
                fprintf (fPtr, "%c%lf",sep,*((double *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTDOUBLE */
            case FMRTCHAR:
            {
                fprintf (fPtr, "%c%c",sep,*((char *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTCHAR */
            case FMRTSTRING:
            {
                fprintf (fPtr, "%c%s",sep,(char *)(currentPtr+Tables[tableIndex].fields[j].delta));
                break;
            }   /* case FMRTSTRING */
            case FMRTTIMESTAMP:
            {
                if (fmrtTimeFormat[0]=='\0')
                    /* time format empty --> print raw timestamp */
                    fprintf (fPtr, "%c%ld",sep,*((time_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                else
                {   /* convert raw timestamp into a string formatted according to fmrtTimeFormat */
                    char  timestamp[MAXFMRTSTRINGLEN+1];
                    strftime(timestamp, MAXFMRTSTRINGLEN, fmrtTimeFormat, localtime((time_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                    fprintf (fPtr, "%c%s",sep,timestamp);
                }
                break;
            }   /* case FMRTTIMESTAMP */
        }   /* switch (Tables[i].fields[j].type) */
    }   /* for (j=0; j<Tables[i].numFields; j++) */
    fprintf (fPtr,"\n");

    /* In-order traversal -> ... last right subtree (in case reverse==0, left subtree otherwise)*/
    if (reverse==0)
        exportRangeRecurseChar (tableIndex, rightIndex, fPtr, sep, reverse, keyMin, keyMax);
    else
        exportRangeRecurseChar (tableIndex, leftIndex, fPtr, sep, reverse, keyMin, keyMax);

    return;
}


/***********************************************************
 * exportRangeRecurseString()
 * ---------------------------------------------------------
 * This function is used by the fmrt library call
 * fmrtExportRangeCsv(), and implements a recursive in-order
 * traversal of the fmrt tree.
 * It takes the table index (first parameter, which is the
 * index of the Table[] array, not the TableId), the
 * current node index (second parameter), a file pointer
 * (third parameter), a char representing a user defined
 * separator between fields and two string values
 * representing respectively the minimum and maximum key
 * value. The routine consider the index as the root of a
 * subtree which is printed into the file pointed by the
 * third parameter by using in order approach.
 ***********************************************************/
static void exportRangeRecurseString (uint8_t tableIndex, fmrtIndex nodeIndex, FILE *fPtr, char sep, uint8_t reverse, char *keyMin, char *keyMax)
{
    /* Local Variables */
    fmrtIndex    leftIndex, rightIndex;
    uint8_t     j;
    char        key[MAXFMRTSTRINGLEN+1];
    void        *currentPtr;

    /* If nodeIndex is NULL stop recursion */
    if (nodeIndex==FMRTNULLPTR)
        return;

    /* Set currentPtr to the first byte of the structure that contains the given index */
    currentPtr = Tables[tableIndex].fmrtData + nodeIndex*Tables[tableIndex].elemSize;

    /* Extract Key, Left and Right subtree indexes */
    strcpy (key,(char *)(currentPtr+Tables[tableIndex].key.delta));
    leftIndex = *((fmrtIndex *) currentPtr);
    rightIndex = *((fmrtIndex *) (currentPtr+sizeof(fmrtIndex)));

    /* In-order traversal -> First left subtree (in case reverse==0, right subtree otherwise)... */
    /* ... but skip unnecessary node traversal in case the current key is outside the range      */
    if (strcmp(key,keyMin)<0)
    {   /* current element is lower than keyMin -> explore only right subtree */
        exportRangeRecurseString (tableIndex, rightIndex, fPtr, sep, reverse, keyMin, keyMax);
        return;
    }
    if (strcmp(key,keyMax)>0)
    {   /* current element is higher than keyMax -> explore only left subtree */
        exportRangeRecurseString (tableIndex, leftIndex, fPtr, sep, reverse, keyMin, keyMax);
        return;
    }
    /* Current key is within the interval... explore both subtrees, with */
    /* order depending on reverse parameter                              */
    if (reverse==0)
        exportRangeRecurseString (tableIndex, leftIndex, fPtr, sep, reverse, keyMin, keyMax);
    else
        exportRangeRecurseString (tableIndex, rightIndex, fPtr, sep, reverse, keyMin, keyMax);

    /* In-order traversal -> ... after subtree print current node... */
    /* Print the key... */
    fprintf (fPtr, "%s",key);
    /* then loop through all the fields and print them separated by sep */
    for (j=0; j<Tables[tableIndex].numFields; j++)
    {   /* Loop through all fields */
        switch (Tables[tableIndex].fields[j].type)
        {
            case FMRTINT:
            {
                fprintf (fPtr, "%c%d",sep,*((uint32_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTINT */
            case FMRTSIGNED:
            {
                fprintf (fPtr, "%c%d",sep,*((int32_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTSIGNED */
            case FMRTDOUBLE:
            {
                fprintf (fPtr, "%c%lf",sep,*((double *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTDOUBLE */
            case FMRTCHAR:
            {
                fprintf (fPtr, "%c%c",sep,*((char *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTCHAR */
            case FMRTSTRING:
            {
                fprintf (fPtr, "%c%s",sep,(char *)(currentPtr+Tables[tableIndex].fields[j].delta));
                break;
            }   /* case FMRTSTRING */
            case FMRTTIMESTAMP:
            {
                if (fmrtTimeFormat[0]=='\0')
                    /* time format empty --> print raw timestamp */
                    fprintf (fPtr, "%c%ld",sep,*((time_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                else
                {   /* convert raw timestamp into a string formatted according to fmrtTimeFormat */
                    char  timestamp[MAXFMRTSTRINGLEN+1];
                    strftime(timestamp, MAXFMRTSTRINGLEN, fmrtTimeFormat, localtime((time_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                    fprintf (fPtr, "%c%s",sep,timestamp);
                }
                break;
            }   /* case FMRTTIMESTAMP */
        }   /* switch (Tables[i].fields[j].type) */
    }   /* for (j=0; j<Tables[i].numFields; j++) */
    fprintf (fPtr,"\n");

    /* In-order traversal -> ... last right subtree (in case reverse==0, left subtree otherwise)*/
    if (reverse==0)
        exportRangeRecurseString (tableIndex, rightIndex, fPtr, sep, reverse, keyMin, keyMax);
    else
        exportRangeRecurseString (tableIndex, leftIndex, fPtr, sep, reverse, keyMin, keyMax);

    return;
}


/***********************************************************
 * exportRangeRecurseTimestamp()
 * ---------------------------------------------------------
 * This function is used by the fmrt library call
 * fmrtExportRangeCsv(), and implements a recursive in-order
 * traversal of the fmrt tree.
 * It takes the table index (first parameter, which is the
 * index of the Table[] array, not the TableId), the
 * current node index (second parameter), a file pointer
 * (third parameter), a char representing a user defined
 * separator between fields and two time_t values
 * representing respectively the minimum and maximum key
 * value. The routine consider the index as the root of a
 * subtree which is printed into the file pointed by the
 * third parameter by using in order approach.
 ***********************************************************/
static void exportRangeRecurseTimestamp (uint8_t tableIndex, fmrtIndex nodeIndex, FILE *fPtr, char sep, uint8_t reverse, time_t keyMin, time_t keyMax)
{
    /* Local Variables */
    fmrtIndex   leftIndex, rightIndex;
    uint8_t     j;
    time_t      key;
    void       *currentPtr;

    /* If nodeIndex is NULL stop recursion */
    if (nodeIndex==FMRTNULLPTR)
        return;

    /* Set currentPtr to the first byte of the structure that contains the given index */
    currentPtr = Tables[tableIndex].fmrtData + nodeIndex*Tables[tableIndex].elemSize;

    /* Extract Key, Left and Right subtree indexes */
    key = *((time_t *)(currentPtr+Tables[tableIndex].key.delta));
    leftIndex = *((fmrtIndex *) currentPtr);
    rightIndex = *((fmrtIndex *) (currentPtr+sizeof(fmrtIndex)));

    /* In-order traversal -> First left subtree (in case reverse==0, right subtree otherwise)... */
    /* ... but skip unnecessary node traversal in case the current key is outside the range      */
    if (key<keyMin)
    {   /* current element is lower than keyMin -> explore only right subtree */
        exportRangeRecurseTimestamp (tableIndex, rightIndex, fPtr, sep, reverse, keyMin, keyMax);
        return;
    }
    if (key>keyMax)
    {   /* current element is higher than keyMax -> explore only left subtree */
        exportRangeRecurseTimestamp (tableIndex, leftIndex, fPtr, sep, reverse, keyMin, keyMax);
        return;
    }
    /* Current key is within the interval... explore both subtrees, with */
    /* order depending on reverse parameter                              */
    if (reverse==0)
        exportRangeRecurseTimestamp (tableIndex, leftIndex, fPtr, sep, reverse, keyMin, keyMax);
    else
        exportRangeRecurseTimestamp (tableIndex, rightIndex, fPtr, sep, reverse, keyMin, keyMax);

    /* In-order traversal -> ... after subtree print current node... */
    /* Print the key... */
    if (fmrtTimeFormat[0]=='\0')
        /* time format empty --> print raw timestamp */
        fprintf (fPtr, "%ld",key);
    else
    {   /* convert raw timestamp into a string formatted according to fmrtTimeFormat */
        char  timestamp[MAXFMRTSTRINGLEN+1];
        strftime(timestamp, MAXFMRTSTRINGLEN, fmrtTimeFormat, localtime((time_t *)(&key)));
        fprintf (fPtr, "%c%s",sep,timestamp);
    }
    /* then loop through all the fields and print them separated by sep */
    for (j=0; j<Tables[tableIndex].numFields; j++)
    {   /* Loop through all fields */
        switch (Tables[tableIndex].fields[j].type)
        {
            case FMRTINT:
            {
                fprintf (fPtr, "%c%d",sep,*((uint32_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTINT */
            case FMRTSIGNED:
            {
                fprintf (fPtr, "%c%d",sep,*((int32_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTSIGNED */
            case FMRTDOUBLE:
            {
                fprintf (fPtr, "%c%lf",sep,*((double *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTDOUBLE */
            case FMRTCHAR:
            {
                fprintf (fPtr, "%c%c",sep,*((char *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                break;
            }   /* case FMRTCHAR */
            case FMRTSTRING:
            {
                fprintf (fPtr, "%c%s",sep,(char *)(currentPtr+Tables[tableIndex].fields[j].delta));
                break;
            }   /* case FMRTSTRING */
            case FMRTTIMESTAMP:
            {
                if (fmrtTimeFormat[0]=='\0')
                    /* time format empty --> print raw timestamp */
                    fprintf (fPtr, "%c%ld",sep,*((time_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                else
                {   /* convert raw timestamp into a string formatted according to fmrtTimeFormat */
                    char  timestamp[MAXFMRTSTRINGLEN+1];
                    strftime(timestamp, MAXFMRTSTRINGLEN, fmrtTimeFormat, localtime((time_t *)(currentPtr+Tables[tableIndex].fields[j].delta)));
                    fprintf (fPtr, "%c%s",sep,timestamp);
                }
                break;
            }   /* case FMRTTIMESTAMP */
        }   /* switch (Tables[i].fields[j].type) */
    }   /* for (j=0; j<Tables[i].numFields; j++) */
    fprintf (fPtr,"\n");

    /* In-order traversal -> ... last right subtree (in case reverse==0, left subtree otherwise)*/
    if (reverse==0)
        exportRangeRecurseTimestamp (tableIndex, rightIndex, fPtr, sep, reverse, keyMin, keyMax);
    else
        exportRangeRecurseTimestamp (tableIndex, leftIndex, fPtr, sep, reverse, keyMin, keyMax);

    return;
}


/**********************************
 *  Public Functions              *
 * ------------------------------ *
 * (can be used outside this file *
 *  and are part of the library)  *
 **********************************/
/***********************************************************
 * fmrtDefineTable()
 * ---------------------------------------------------------
 * Define a new table with the following parameters:
 * - tableId
 *   unique identifier of the table between 0 and 255
 * - tableName
 *   string of up to 32 chars identifying table name
 * - tableNumElem
 *   max number of elements in table between 1 and 2^26
 * This shall necessarily be the first library function
 * invoked by the caller.
 * A table cannot be redefined unless it is cleared first
 * through fmrtClearTable()
 * The library supports the definition of up to 32 tables
 * ---------------------------------------------------------
 * Possible Return Values:
 * - FMRTOK
 *   Table successfully defined
 * - FMRTKO
 *   Result obtained when the number of elements is outside the
 *   allowed interval (1 - 2^26)
 * - FMRTIDALREADYEXISTS
 *   tableId is already in use by another table
 * - FMRTMAXTABLEREACHED
 *   Result obtained when more than 32 tables are defined
 ***********************************************************/
fmrtResult fmrtDefineTable (fmrtId tableId, char* tableName, fmrtIndex tableNumElem)
{
    /* Local Variables */
    uint8_t     i,j;

    /* Set global lock to avoid cuncurrent access in case of parallel definition/clear of tables by different threads */
    pthread_mutex_lock(&fmrtGlobalMtx);

    /* If this is the first invocation of the library initialize Tables[] */
    if (fmrtFirstInvocation)
    {   /* All Tables[] element are set as not busy */
        fmrtFirstInvocation = 0;
        for (i=0; i<MAXTABLES; i++)
            Tables[i].status = FREE;
    }   /* if (fmrtFirstInvocation) */

    /* If tableId is already used return FMRTIDALREADYEXISTS */
    for (i=0; i<MAXTABLES; i++)
        if ( (Tables[i].tableId==tableId) && (Tables[i].status!=FREE) )
        {   /* Remove global lock before exiting */
            pthread_mutex_unlock(&fmrtGlobalMtx);
            return (FMRTIDALREADYEXISTS);
        }

    /* Loop again searching for the first element with Tables[i].status==FREE */
    for (i=0; (i<MAXTABLES)&&(Tables[i].status!=FREE); i++ );

    /* If i==MAXTABLES all elements are busy - return FMRTMAXTABLEREACHED */
    if (i==MAXTABLES)
    {   /* Remove global lock before exiting */
        pthread_mutex_unlock(&fmrtGlobalMtx);
        return (FMRTMAXTABLEREACHED);
    }

    /* Check the requested number of elements against MAXFMRTELEM and eventually provide FMRTKO */
    if ( (tableNumElem < 1) || (tableNumElem > MAXFMRTELEM) )
    {   /* Remove global lock before exiting */
        pthread_mutex_unlock(&fmrtGlobalMtx);
        return (FMRTKO);
    }

    /* If control reaches here, i is the index of the free element to use */
    Tables[i].tableId = tableId;
    Tables[i].status = DEFINED;
    Tables[i].numFields = 0;
    /* Assign Table Name truncating it to MAXFMRTTABLENAME if too long */
    strncpy(Tables[i].tableName,tableName,MAXFMRTTABLENAME+1);
    Tables[i].tableName[MAXFMRTTABLENAME] ='\0';
    Tables[i].tableMaxElem = tableNumElem;
    Tables[i].currentNumElem = 0;
    /* Set key and field names to empty string */
    Tables[i].key.name[0]='\0';
    for (j=0;j<MAXFMRTFIELDNUM;j++)
        Tables[i].fields[j].name[0] = '\0';
    /* Initial size consists in left ptr + right ptr */
    Tables[i].elemSize = 2*sizeof (fmrtIndex);
    Tables[i].fmrtRoot = FMRTNULLPTR;
    Tables[i].fmrtFree = FMRTNULLPTR;
    Tables[i].fmrtData = NULL;
    /* Initialize Table specific mutex */
    pthread_mutex_init(&(Tables[i].tableMtx), NULL);

    /* Remove global lock before exiting */
    pthread_mutex_unlock(&fmrtGlobalMtx);

    #ifdef FMRTDEBUG
    printf ("Inside fmrtDefineTable() -> TableId: %d - Table[] index: %d\n",Tables[i].tableId,i);
    #endif

    return (FMRTOK);
}


/***********************************************************
 * fmrtClearTable()
 * ---------------------------------------------------------
 * Deallocate a previously allocated table:
 * - tableId
 *   unique identifier of the table between 0 and 255
 * ---------------------------------------------------------
 * Possible Return Values:
 * - FMRTOK
 *   Table successfully deallocated
 * - FMRTKO
 *   Result obtained when this is the first library call
 *   invoked by the caller
 * - FMRTIDNOTFOUND
 *   tableId is not defined
 ***********************************************************/
fmrtResult fmrtClearTable (fmrtId tableId)
{
    /* Local Variables */
    uint8_t     i;
    fmrtResult   res;

    /* Set global lock to avoid cuncurrent access in case of parallel definition/clear of tables by different threads */
    pthread_mutex_lock(&fmrtGlobalMtx);

    /* Call searchTable() internal function to look for the given tableId */
    /* In case of error exit and report error to the calling program      */
    if ( ((res=searchTable(tableId,&i))!=FMRTOK) )
    {   /* Remove global lock before exiting */
        pthread_mutex_unlock(&fmrtGlobalMtx);
        return (res);
    }

    /* Otherwise deallocate stored data, set busy flag to 0 and destroy Table specific mutex */
    if (Tables[i].fmrtData)
        free (Tables[i].fmrtData);
    Tables[i].status = FREE;
    pthread_mutex_destroy(&(Tables[i].tableMtx));

    /* Remove global lock before exiting */
    pthread_mutex_unlock(&fmrtGlobalMtx);

    #ifdef FMRTDEBUG
    printf ("Inside fmrtClearTable() -> TableId: %d - Table[] index: %d\n",Tables[i].tableId,i);
    #endif

    return (FMRTOK);
}


/***********************************************************
 * fmrtDefineKey()
 * ---------------------------------------------------------
 * Define key name, type (and also key length for string key
 * type) for a previously defined Table:
 * - tableId
 *   unique identifier of the table between 0 and 255. The
 *   table shall be defined first through fmrtDefineTable()
 * - keyName
 *   descriptive name of the key (up to 16 chars allowed,
 *   in case of length exceeding this limit the name will
 *   be truncated)
 * - keyType
 *   identifies key type. Allowed values defined in header
 *   fmrt.h (FMRTINT, FMRTSIGNED, FMRTDOUBLE, FMRTCHAR,
 *   FMRTSTRING and FMRTTIMESTAMP)
 * - keyLen
 *   meaningful only in case of keyType==FMRTSTRING, when it
 *   represents the maximum string length for the key value
 *   Allowed values are in the interval 1-64.
 *   For FMRTINT, FMRTSIGNED, FMRTDOUBLE, FMRTCHAR and
 *   FMRTTIMESTAMP types, the parameter is meaningless,
 *   since by default key lengths in those cases are
 *   fixed (e.g. 1 for FMRTCHAR, 4 for FMRTINT, etc.).
 * This call can be invoked only before entering the first
 * element in the table. After the first fmrtCreate() operation
 * the fmrtDefineKey() call is forbidden. However, up that
 * moment the call can be invoked an arbitrary number of
 * times, observing that each invocation overwrites all
 * previous ones
 * ---------------------------------------------------------
 * Possible Return Values:
 * - FMRTOK
 *   Key definition successful
 * - FMRTKO
 *   Result obtained either when this is the first library
 *   call invoked by the caller or when keyType parameter
 *   is not valid
 * - FMRTIDNOTFOUND
 *   tableId is not defined
 * - FMRTNOTEMPTY
 *   The key cannot be redefined since the table is not
 *   empty
 * - FMRTFIELDTOOLONG
 *   In case of keyType==FMRTSTRING the keyLen parameter is
 *   outside the allowed interval
 ***********************************************************/
fmrtResult fmrtDefineKey (fmrtId tableId, char* keyName, fmrtType keyType, fmrtLen keyLen)
{
    /* Local Variables */
    uint8_t     i;
    fmrtResult   res;

    /* Call searchTable() internal function to look for the given tableId */
    /* In case of error exit and report error to the calling program      */
    /* To be completely safe, searchTable() shoud be called by locking    */
    /* global mutex (fmrtGlobalMtx), but it is very unlikely that a thread */
    /* tries to access a table which is still under definition, therefore */
    /* we prefer to avoid bottlenecks                                     */
    if ( ((res=searchTable(tableId,&i))!=FMRTOK) )
        return (res);

    /* If found, set lock and check that the AVL Tree is still empty, otherwise provide error FMRTNOTEMPTY */
    pthread_mutex_lock(&(Tables[i].tableMtx));
    if (Tables[i].status==NOTEMPTY)
    {   /* Clear lock before exiting */
        pthread_mutex_unlock(&(Tables[i].tableMtx));
        return (FMRTNOTEMPTY);
    }

    /* Check keyType and, in case of string type, also keyLen     */
    /* Also, set properly keytype and keylen into Table[] element */
    switch (keyType)
    {
        case FMRTINT:
        {
            Tables[i].key.type = keyType;
            Tables[i].key.len = sizeof (uint32_t);
            break;
        }
        case FMRTSIGNED:
        {
            Tables[i].key.type = keyType;
            Tables[i].key.len = sizeof (int32_t);
            break;
        }
        case FMRTDOUBLE:
        {
            Tables[i].key.type = keyType;
            Tables[i].key.len = sizeof (double);
            break;
        }
        case FMRTCHAR:
        {
            Tables[i].key.type = keyType;
            Tables[i].key.len = sizeof (char);
            break;
        }
        case FMRTSTRING:
        {
            if ( (keyLen<=0) || (keyLen>MAXFMRTSTRINGLEN) )
            {   /* Clear lock before exiting */
                pthread_mutex_unlock(&(Tables[i].tableMtx));
                return (FMRTFIELDTOOLONG);
            }
            Tables[i].key.type = keyType;
            Tables[i].key.len = keyLen + 1;
            break;
        }
        case FMRTTIMESTAMP:
        {
            Tables[i].key.type = keyType;
            Tables[i].key.len = sizeof (time_t);
            break;
        }
        default:
        {   /* Clear lock before exiting */
            pthread_mutex_unlock(&(Tables[i].tableMtx));
            return (FMRTKO);
        }
    }   /* switch (keyType) */

    /* Assign Key Name truncating it to MAXFMRTNAMELEN if too long */
    strncpy (Tables[i].key.name,keyName,MAXFMRTNAMELEN+1);
    Tables[i].key.name[MAXFMRTNAMELEN]='\0';

    /* Set also key.delta and update Table[] element size */
    Tables[i].key.delta = Tables[i].elemSize;
    Tables[i].elemSize += Tables[i].key.len;

    /* Clear lock before exiting */
    pthread_mutex_unlock(&(Tables[i].tableMtx));

    #ifdef FMRTDEBUG
    printf ("Inside fmrtDefineKey() -> TableId: %d - Table[] index: %d\n",Tables[i].tableId,i);
    #endif

    return (FMRTOK);
}


/***********************************************************
 * fmrtDefineFields()
 * ---------------------------------------------------------
 * Define fields for each table entry. It is a call with a
 * variable number of arguments. The first two parameters
 * (always present) are respectively:
 * - tableId
 *   unique identifier of the table between 0 and 255. The
 *   table shall be defined first through fmrtDefineTable()
 * - numFields
 *   number of fields, in the interval 1-16 (excluding the
 *   key, which is defined apart through fmrtDefineKey() )
 * The parameters after the two above mentioned are used
 * to specify name, type (and also length for string field
 * type). Specifically, for each field, there shall be:
 * - field name
 *   descriptive name of the field (up to 16 chars allowed,
 *   in case of length exceeding this limit the name will
 *   be truncated)
 * - field type
 *   identifies field type. Allowed values defined in header
 *   fmrt.h (FMRTINT, FMRTSIGNED, FMRTDOUBLE, FMRTCHAR,
 *   FMRTSTRING and FMRTTIMESTAMP)
 * - field len
 *   THIS SHALL BE INSERTED ONLY if field type == FMRTSTRING.
 *   DO NOT INSERT FIELD LEN FOR OTHER TYPES
 *   It represents the maximum string length for the field
 *   value. Allowed values are in the interval 1-64.
 * This call can be invoked only before entering the first
 * element in the table. After the first fmrtCreate() operation
 * the fmrtDefineFileds() call is forbidden. However, up that
 * moment the call can be invoked an arbitrary number of
 * times, observing that each invocation overwrites all
 * previous ones
 * ---------------------------------------------------------
 * Possible Return Values:
 * - FMRTOK
 *   Fields definition successful
 * - FMRTKO
 *   Result obtained either when this is the first library
 *   call invoked by the caller or when one of the field
 *   type parameters is not valid
 * - FMRTIDNOTFOUND
 *   tableId is not defined
 * - FMRTNOTEMPTY
 *   The fields cannot be redefined since the table is not
 *   empty
 * - FMRTMAXFIELDSINVALID
 *   The specified number of fields is outside the allowed
 *   range (1-16)
 * - FMRTFIELDTOOLONG
 *   The maximum length specified for one of the FMRTSTRING
 *   parameters is outside the allowed interval (1-64)
 ***********************************************************/
fmrtResult fmrtDefineFields (fmrtId tableId, uint8_t numFields, ...)
{
    /* Local Variables */
    va_list     args;
    uint8_t     i,j;
    char        *name;
    int         type,len;
    fmrtResult   res;

    /* Call searchTable() internal function to look for the given tableId */
    /* In case of error exit and report error to the calling program      */
    /* To be completely safe, searchTable() shoud be called by locking    */
    /* global mutex (fmrtGlobalMtx), but it is very unlikely that a thread */
    /* tries to access a table which is still under definition, therefore */
    /* we prefer to avoid bottlenecks                                     */
    if ( ((res=searchTable(tableId,&i))!=FMRTOK) )
        return (res);

    /* If found, set lock and check that the AVL Tree is still empty, otherwise provide error FMRTNOTEMPTY */
    pthread_mutex_lock(&(Tables[i].tableMtx));
    if (Tables[i].status==NOTEMPTY)
    {   /* Clear lock before exiting */
        pthread_mutex_unlock(&(Tables[i].tableMtx));
        return (FMRTNOTEMPTY);
    }

    /* If specified number of fields is outside the allowed range provide an error */
    if ( (numFields<=0) || (numFields>MAXFMRTFIELDNUM) )
    {   /* Clear lock before exiting */
        pthread_mutex_unlock(&(Tables[i].tableMtx));
        return (FMRTMAXFIELDSINVALID);
    }

    /* Start parsing variable arguments */
    va_start(args, numFields);

    for (j=0; j<numFields; j++)
    {   /* Loop on the number of fields */
        /* The first expected argument is field name (up to 16 chars, otherwise it is truncated) */
        name = va_arg (args,char*);
        strncpy (Tables[i].fields[j].name,name,MAXFMRTNAMELEN+1);
        Tables[i].fields[j].name[MAXFMRTNAMELEN]='\0';
        /* The second expected argument is argument type */
        type = va_arg (args, int);
        if ( (type<FMRTINT) || (type>FMRTTIMESTAMP) )
        {   /* Clear lock before exiting */
            pthread_mutex_unlock(&(Tables[i].tableMtx));
            va_end (args);
            return (FMRTKO);
        }
        Tables[i].fields[j].type = (fmrtType) type;
        /* In case of FMRTSTRING type we expect also string length (up to 64 characters) */
        if (type==FMRTSTRING)
        {
            len = va_arg (args, int);
            if ( (len<=0) || (len>MAXFMRTSTRINGLEN) )
            {   /* Clear lock before exiting */
                pthread_mutex_unlock(&(Tables[i].tableMtx));
                va_end (args);
                return (FMRTFIELDTOOLONG);
            }
        }   /* if (type==FMRTSTRING) */

        switch (type)
        {
            case FMRTINT:
            {
                Tables[i].fields[j].len = sizeof (uint32_t);
                break;
            }
            case FMRTSIGNED:
            {
                Tables[i].fields[j].len = sizeof (int32_t);
                break;
            }
            case FMRTDOUBLE:
            {
                Tables[i].fields[j].len = sizeof (double);
                break;
            }
            case FMRTCHAR:
            {
                Tables[i].fields[j].len = sizeof (char);
                break;
            }
            case FMRTSTRING:
            {
                Tables[i].fields[j].len = (fmrtLen) len + 1;
                break;
            }
            case FMRTTIMESTAMP:
            {
                Tables[i].fields[j].len = sizeof (time_t);
                break;
            }
        }   /* switch (type) */

        Tables[i].fields[j].delta = Tables[i].elemSize;
        Tables[i].elemSize += Tables[i].fields[j].len;
    }   /* for (j=0; j<numFields; j++) */

    /* All fields have been read, update numberof fields in Table[], */
    /* close the variable list argument and return FMRTOK             */
    Tables[i].numFields = numFields;
    va_end (args);

    /* Clear lock before exiting */
    pthread_mutex_unlock(&(Tables[i].tableMtx));

    #ifdef FMRTDEBUG
    printf ("Inside fmrtDefineFields() -> TableId: %d - Table[] index: %d\n",Tables[i].tableId,i);
    #endif

    return (FMRTOK);
}


/***********************************************************
 * fmrtRead()
 * ---------------------------------------------------------
 * This library call is used to read an entry from the fmrt
 * tree. It is a call with a variable number of arguments.
 * The first two parameters (always present) are
 * respectively:
 * - tableId
 *   unique identifier of the table between 0 and 255. The
 *   table shall be defined first through fmrtDefineTable()
 * - key
 *   contains the key value to be searched into the table.
 *   It shall be of the same type defined by the
 *   fmrtDefineKey() call. The library behaviour is undefined
 *   if this constraint is not satisfied. In case of string
 *   key, the length is truncated to the max length specified
 *   at key definition through fmrtDefineKey().
 * After those mandatory parameters, there is a list of
 * pointer parameters that are filled with values read
 * from the entry (if present). Parameters are ordered
 * according to the same order used in definition (i.e.
 * in fmrtDefinFields() library call). Be aware that
 * parameters number and types shall be correct, otherwise
 * the call may cause run-time errors.
 * ---------------------------------------------------------
 * Possible Return Values:
 * - FMRTOK
 *   The entry has been found and corresponding parameters
 *   extracted
 * - FMRTNOTFOUND
 *   The entry with the given key is not present in the table
 * - FMRTKO
 *   Result obtained when this is the first library
 *   call invoked by the caller
 * - FMRTIDNOTFOUND
 *   tableId is not defined
 ***********************************************************/
fmrtResult fmrtRead (fmrtId tableId, ...)
{
    /* Local Variables */
    va_list     args;
    uint8_t     i,j,maxLen;
    void        *currentPtr;
    fmrtResult   res;
    uint32_t    keyInt;
    int32_t     keySigned;
    double      keyDouble;
    time_t      keyTimestamp;
    char        keyChar,
                *string,
                keyString[MAXFMRTSTRINGLEN+1];
    fmrtNodeTraversalStack *traversal;

    /* Call searchTable() internal function to look for the given tableId */
    /* In case of error exit and report error to the calling program      */
    /* To be completely safe, searchTable() shoud be called by locking    */
    /* global mutex (fmrtGlobalMtx), but it is very unlikely that a thread */
    /* tries to access a table which is still under definition, therefore */
    /* we prefer to avoid bottlenecks                                     */
    if ( ((res=searchTable(tableId,&i))!=FMRTOK) )
        return (res);

    /* Set Table specific lock */
    pthread_mutex_lock(&(Tables[i].tableMtx));

    /* Initialize the list of variable arguments in order to read the key first */
    va_start (args,tableId);
    switch (Tables[i].key.type)
    {
        case FMRTINT:
        {
            keyInt = va_arg (args, uint32_t);
            break;
        }
        case FMRTSIGNED:
        {
            keySigned = va_arg (args, int32_t);
            break;
        }
        case FMRTDOUBLE:
        {
            keyDouble = va_arg (args, double);
            break;
        }
        case FMRTCHAR:
        {
            keyChar = (unsigned char) va_arg (args,int);
            break;
        }
        case FMRTSTRING:
        {   /* Read the key and truncate to the maximum length specified during definition */
            string = va_arg (args,char*);
            maxLen = Tables[i].key.len;     /* This field is max string length + trailing 0 */
            strncpy (keyString,string,maxLen);
            keyString[maxLen-1] = '\0';
            break;
        }
        case FMRTTIMESTAMP:
        {
            if (fmrtTimeFormat[0]=='\0')
                /* time format empty --> read raw timestamp from argument */
                keyTimestamp = va_arg (args, time_t);
            else
            {   /* convert string read from argument to raw timestamp according to fmrtTimeFormat */
                struct tm   TimeFromString;
                string = va_arg (args,char*);
                if (strptime (string, fmrtTimeFormat, &TimeFromString) != NULL)
                    keyTimestamp = mktime (&TimeFromString);
                else
                    keyTimestamp = 0;
            }
            break;
        }   /* case FMRTTIMESTAMP */
    }   /* switch (Tables[i].key.type) */

    /* call searchElem() internal function to look for the element and provide error if result is not FMRTOK */
    if ( (res=searchElem(i, keyInt, keySigned, keyDouble, keyChar, keyString, keyTimestamp, &traversal)) != FMRTOK)
    {
        va_end (args);
        clearNodeTraversalStack (traversal);
        /* Clear the lock before exiting */
        pthread_mutex_unlock(&(Tables[i].tableMtx));
        return (res);
    }

    /* The element was found and traversal is a pointer to a LIFO structure */
    /* whose top element contains the index of the node we searched         */
    /* Set currentPtr to point to the first byte of the structure           */
    currentPtr = Tables[i].fmrtData + (traversal->index)*Tables[i].elemSize;

    /* Now read all remaining arguments and loop through the fields */
    for (j=0; j<Tables[i].numFields; j++)
    {   /* @@@ - By removing comment from the following line we obtain for each field the pair name, value */
        /* @@@ - If the comment is present, we obtain only a list of values according to the same order    */
        /* @@@ - defined in fmrtDefineFields() library call                                                 */
        /* strcpy (va_arg (args, char *),Tables[i].fields[j].name); */
        switch (Tables[i].fields[j].type)
        {
            case FMRTINT:
            {
                *va_arg (args, uint32_t *) = *((uint32_t *)(currentPtr+Tables[i].fields[j].delta));
                break;
            }
            case FMRTSIGNED:
            {
                *va_arg (args, int32_t *) = *((int32_t *)(currentPtr+Tables[i].fields[j].delta));
                break;
            }
            case FMRTDOUBLE:
            {
                *va_arg (args, double *) = *((double *)(currentPtr+Tables[i].fields[j].delta));
                break;
            }
            case FMRTCHAR:
            {
                *va_arg (args, char *) = *((char *)(currentPtr+Tables[i].fields[j].delta));
                break;
            }
            case FMRTSTRING:
            {
                strcpy (va_arg (args, char *),(char *)(currentPtr+Tables[i].fields[j].delta));
                break;
            }
            case FMRTTIMESTAMP:
            {
                if (fmrtTimeFormat[0]=='\0')
                    /* time format empty --> use raw timestamp from argument */
                    *va_arg (args, time_t *) = *((time_t *)(currentPtr+Tables[i].fields[j].delta));
                else
                {   /* convert raw timestamp into a string formatted according to fmrtTimeFormat */
                    char  timestamp[MAXFMRTSTRINGLEN+1];
                    strftime(timestamp, MAXFMRTSTRINGLEN, fmrtTimeFormat, localtime((time_t *)(currentPtr+Tables[i].fields[j].delta)));
                    strcpy (va_arg (args, char *),timestamp);
                }
                break;
            }   /* case FMRTTIMESTAMP */
        }   /* switch (Tables[i].fields[j].type) */
    }   /* for (j=0; j<Tables[i].numFields; j++) */
    va_end (args);

    #ifdef FMRTDEBUG
    printf ("\n\nRead node at index: %d\n",traversal->index);
    __fmrtDebugPrintNode (Tables[i].tableId, traversal->index);
    printf ("Path from node up to the root:\n");
    __fmrtPrintStack(i,traversal);
    #endif

    /* clear node traversal LIFO structure allocated by searchElem() and exit */
    clearNodeTraversalStack (traversal);

    /* Clear the lock before exiting */
    pthread_mutex_unlock(&(Tables[i].tableMtx));

    return (FMRTOK);
}


/***********************************************************
 * fmrtCreate()
 * ---------------------------------------------------------
 * This library call is used to insert a new entry into the
 * fmrt tree. It is a call with a variable number of
 * arguments. The first two parameters (always present) are
 * respectively:
 * - tableId
 *   unique identifier of the table between 0 and 255. The
 *   table shall be defined first through fmrtDefineTable()
 * - key
 *   contains the key value to be searched into the table.
 *   It shall be of the same type defined by the
 *   fmrtDefineKey() call. The library behaviour is undefined
 *   if this constraint is not satisfied. In case of string
 *   key, the length is truncated to the max length specified
 *   at key definition through fmrtDefineKey().
 * After those mandatory parameters, there is a list of
 * parameters that are used to fill the fields of the new
 * entry that is going to be created. Parameters are ordered
 * according to the same order used in definition (i.e.
 * in fmrtDefinFields() library call). Be aware that
 * parameters number and types shall be correct, otherwise
 * the call may cause run-time errors. In case of string
 * parameters, the length is truncated to the max length
 * specified at field definition through fmrtDefineFields().
 * ---------------------------------------------------------
 * Possible Return Values:
 * - FMRTOK
 *   The entry has been correctly inserted into the tree
 * - FMRTDUPLICATEKEY
 *   The specified key is already present in the AVL Tree
 * - FMRTKO
 *   Result obtained when this is the first library
 *   call invoked by the caller
 * - FMRTIDNOTFOUND
 *   tableId is not defined
 * - FMRTOUTOFMEMORY
 *   The FMRT tree is full
 ***********************************************************/
fmrtResult fmrtCreate (fmrtId tableId, ...)
{
    /* Local Variables */
    va_list     args;
    uint8_t     i,j,maxLen;
    fmrtIndex    newElement,
                rebalIndex;
    void        *currentPtr;
    fmrtResult   res;
    uint32_t    keyInt;
    int32_t     keySigned;
    double      keyDouble;
    time_t      keyTimestamp;
    char        keyChar,
                *string,
                keyString[MAXFMRTSTRINGLEN+1];
    fmrtNodeTraversalStack   *traversal,
                            *rebalPtr;

    /* Call searchTable() internal function to look for the given tableId */
    /* In case of error exit and report error to the calling program      */
    /* To be completely safe, searchTable() shoud be called by locking    */
    /* global mutex (fmrtGlobalMtx), but it is very unlikely that a thread */
    /* tries to access a table which is still under definition, therefore */
    /* we prefer to avoid bottlenecks                                     */
    if ( ((res=searchTable(tableId,&i))!=FMRTOK) )
        return (res);

    /* Set Table specific lock */
    pthread_mutex_lock(&(Tables[i].tableMtx));

    /* Initialize the list of variable arguments in order to read the key first */
    va_start (args,tableId);
    switch (Tables[i].key.type)
    {
        case FMRTINT:
        {
            keyInt = va_arg (args, uint32_t);
            break;
        }
        case FMRTSIGNED:
        {
            keySigned = va_arg (args, int32_t);
            break;
        }
        case FMRTDOUBLE:
        {
            keyDouble = va_arg (args, double);
            break;
        }
        case FMRTCHAR:
        {
            keyChar = (unsigned char) va_arg (args,int);
            break;
        }
        case FMRTSTRING:
        {   /* Read the key and truncate to the maximum length specified during definition */
            string = va_arg (args,char*);
            maxLen = Tables[i].key.len;     /* This field is max string length + trailing 0 */
            strncpy (keyString,string,maxLen);
            keyString[maxLen-1] = '\0';
            break;
        }
        case FMRTTIMESTAMP:
        {
            if (fmrtTimeFormat[0]=='\0')
                /* time format empty --> read raw timestamp from argument */
                keyTimestamp = va_arg (args, time_t);
            else
            {   /* convert string read from argument to raw timestamp according to fmrtTimeFormat */
                struct tm   TimeFromString;
                string = va_arg (args,char*);
                if (strptime (string, fmrtTimeFormat, &TimeFromString) != NULL)
                    keyTimestamp = mktime (&TimeFromString);
                else
                    keyTimestamp = 0;
            }
            break;
        }   /* case FMRTTIMESTAMP */
    }   /* switch (Tables[i].key.type) */

    /* call searchElem() internal function to look for the element and provide error if result is FMRTOK */
    if ( (res=searchElem(i, keyInt, keySigned, keyDouble, keyChar, keyString, keyTimestamp, &traversal)) == FMRTOK)
    {   /* The element has been found, therefore it is already present */
        va_end (args);
        clearNodeTraversalStack (traversal);
        /* Clear the lock before exiting */
        pthread_mutex_unlock(&(Tables[i].tableMtx));
        return (FMRTDUPLICATEKEY);
    }
    if (res!=FMRTNOTFOUND)
    {   /* go on only if key is not present, otherwise provide error and return */
        va_end (args);
        clearNodeTraversalStack (traversal);
        /* Clear the lock before exiting */
        pthread_mutex_unlock(&(Tables[i].tableMtx));
        return (res);
    }

    /* The element is not present and traversal is a pointer to a LIFO structure     */
    /* whose top element contains the index of the parent node and the corresponding */
    /* subtree on which insertion shall be done                                      */

    /* If not already called before, this is the first insertion */
    if ( (Tables[i].fmrtData==NULL) || (Tables[i].status==DEFINED) )
    {   /* Initialize Empty elements list */
        res = initEmptyList(i);
        if (res!=FMRTOK)
        {   /* Not able to allocate memory and initialize empty list - very likely we have not enough memory free */
            va_end (args);
            clearNodeTraversalStack (traversal);
            /* Clear the lock before exiting */
            pthread_mutex_unlock(&(Tables[i].tableMtx));
            return (res);
        }
    }   /* if ( (Tables[i].fmrtData==NULL) ... */

    /* Get an empty element from the list of empty nodes */
    if ( (newElement=getEmptyElem(i)) == FMRTNULLPTR)
    {   /* Not able to fetch an empty element - Probably the table is full */
        va_end (args);
        clearNodeTraversalStack (traversal);
        /* Clear the lock before exiting */
        pthread_mutex_unlock(&(Tables[i].tableMtx));
        return (FMRTOUTOFMEMORY);
    }

    /* Link the new element to the existing structure (if present) */
    if (traversal==NULL)    /* the stack of nodes traversed is empty -> we are creating the root node (newElement is the root index) */
        Tables[i].fmrtRoot = newElement;
    else if (traversal->go == LEFT)
    {   /* the stack exists and the path from parent node goes through the left subtree */
        currentPtr = Tables[i].fmrtData + (traversal->index)*Tables[i].elemSize;
        *((fmrtIndex*)currentPtr) = newElement;
    }
    else
    {   /* the stack exists and the path from parent node goes through the right subtree */
        currentPtr = Tables[i].fmrtData + (traversal->index)*Tables[i].elemSize + sizeof(fmrtIndex);
        *((fmrtIndex*)currentPtr) = newElement;
    }

    /* Set currentPtr to point to this new element, which is always a leaf (at least initially) */
    currentPtr = Tables[i].fmrtData + newElement*Tables[i].elemSize;

    /* Insert null pointers to left and right subtree */
    *((fmrtIndex*)currentPtr) = FMRTNULLPTR;
    *((fmrtIndex*)(currentPtr+sizeof(fmrtIndex))) = FMRTNULLPTR;

    /* copy the key into the newly created element */
    switch (Tables[i].key.type)
    {
        case FMRTINT:
        {
            *((uint32_t *)(currentPtr+Tables[i].key.delta)) = keyInt;
            break;
        }
        case FMRTSIGNED:
        {
            *((int32_t *)(currentPtr+Tables[i].key.delta)) = keySigned;
            break;
        }
        case FMRTDOUBLE:
        {
            *((double *)(currentPtr+Tables[i].key.delta)) = keyDouble;
            break;
        }
        case FMRTCHAR:
        {
            *((char *)(currentPtr+Tables[i].key.delta)) = keyChar;
            break;
        }
        case FMRTSTRING:
        {   /* Please observe that keyString has been truncated when it has been read from the function argument */
            strcpy ((char *)(currentPtr+Tables[i].key.delta),keyString);
            break;
        }
        case FMRTTIMESTAMP:
        {
            *((time_t *)(currentPtr+Tables[i].key.delta)) = keyTimestamp;
            break;
        }
    }   /* switch (Tables[i].key.type) */

    /* Now read the variable list of arguments and use them to fill in the fields */
    for (j=0; j<Tables[i].numFields; j++)
    {   /* Loop through all fields */
        switch (Tables[i].fields[j].type)
        {
            case FMRTINT:
            {
                *((uint32_t *)(currentPtr+Tables[i].fields[j].delta)) = va_arg (args,uint32_t);
                 break;
            }
            case FMRTSIGNED:
            {
                *((int32_t *)(currentPtr+Tables[i].fields[j].delta)) = va_arg (args,int32_t);
                 break;
            }
            case FMRTDOUBLE:
            {
                *((double *)(currentPtr+Tables[i].fields[j].delta)) = va_arg (args,double);
                 break;
            }
            case FMRTCHAR:
            {
                *((char *)(currentPtr+Tables[i].fields[j].delta)) = (unsigned char) va_arg (args,int);
                break;
            }
            case FMRTSTRING:
            {   /* In case of string exceeding the maximum length it is automatically truncated */
                string = va_arg (args, char*);
                maxLen = Tables[i].fields[j].len;       /* This field is max string length + trailing 0 */
                strncpy ((char *)(currentPtr+Tables[i].fields[j].delta),string,maxLen);
                *((char *)(currentPtr+Tables[i].fields[j].delta+maxLen-1)) = '\0';
                break;
            }
            case FMRTTIMESTAMP:
            {
                if (fmrtTimeFormat[0]=='\0')
                    /* time format empty --> read raw timestamp from argument */
                    *((time_t *)(currentPtr+Tables[i].fields[j].delta)) = va_arg (args,time_t);
                else
                {   /* convert string read from argument to raw timestamp according to fmrtTimeFormat */
                    struct tm   TimeFromString;
                    string = va_arg (args, char*);
                    if (strptime (string, fmrtTimeFormat, &TimeFromString) != NULL)
                        *((time_t *)(currentPtr+Tables[i].fields[j].delta)) = mktime (&TimeFromString);
                    else
                        *((time_t *)(currentPtr+Tables[i].fields[j].delta)) = 0;
                }
                break;
            }
        }   /* switch (Tables[i].fields[j].type) */
    }   /* for (j=0; j<Tables[i].numFields; j++) */
    va_end (args);

    /* Element has been inserted - Now go through the traversal LIFO structure and */
    /* rebalance fmrt tree starting from the bottom and going up to the root        */
    rebalPtr = traversal;   /* start traversing from the top of the stack, i.e. the parent of the node just inserted) */
    while (rebalPtr!=NULL)
    {   /* rebalance the subtree whose root is the current node */
        rebalIndex = rebalanceSubTree (i,rebalPtr->index);  /* the root might change due to rotations */
        /* go up to the parent */
        rebalPtr = rebalPtr->next;
        if (rebalPtr!=NULL)
        {   /* There is a parent node - update pointer (left or right depending on the content of traversal structure) */
            if (rebalPtr->go == LEFT)
                currentPtr = Tables[i].fmrtData + (rebalPtr->index)*Tables[i].elemSize;
            else
                currentPtr = Tables[i].fmrtData + (rebalPtr->index)*Tables[i].elemSize+sizeof(fmrtIndex);
            /* The proper pointer is updated with the output of the rebalance structure */
            *((fmrtIndex*)currentPtr) = rebalIndex;
        }   /* if (rebalPtr!=NULL) */
        else
            /* There is no parent node - Rotation implies a change of the fmrt root pointer */
            Tables[i].fmrtRoot = rebalIndex;
    }   /* while (rebalPtr!=NULL) */

    #ifdef FMRTDEBUG
    printf ("\n\nCreated node at index: %d\n",newElement);
    __fmrtDebugPrintNode (Tables[i].tableId, newElement);
    printf ("Path from node up to the root:\n");
    __fmrtPrintStack(i,traversal);
    #endif

    /* set Tables[i].status, increment number of stored elements, clear node traversal LIFO structure allocated by searchElem() and exit */
    Tables[i].status = NOTEMPTY;
    Tables[i].currentNumElem += 1;
    clearNodeTraversalStack (traversal);
    /* Clear the lock before exiting */
    pthread_mutex_unlock(&(Tables[i].tableMtx));

    return (FMRTOK);
}


/***********************************************************
 * fmrtModify()
 * ---------------------------------------------------------
 * This library call is used to update an existing entry
 * into the fmrt tree. It is a call with a variable number of
 * arguments. The first three parameters (always present) are
 * respectively:
 * - tableId
 *   unique identifier of the table between 0 and 255. The
 *   table shall be defined first through fmrtDefineTable()
 * - paramMask
 *   It is a bitwise mask that is used to identify the
 *   paramaters to be changed. For example, assuming that
 *   5 parameters have been defined through fmrtDefineFields(),
 *   e.g.
 *     fmrtDefineFields(tableId,5,param#0,..,param#1,..param#4)
 *   and that we want to update only parameters 0,1 and 4,
 *   we would set paramMask as follows:
 *     paramMask = 19  -->  binary 10011
 *   The rightmost bit represents the first parameter in
 *   fmrtDefineFields(), the one immediately at its left is the
 *   second parameter, and so on.
 * - key
 *   contains the key value to be searched into the table.
 *   It shall be of the same type defined by the
 *   fmrtDefineKey() call. The library behaviour is undefined
 *   if this constraint is not satisfied. In case of string
 *   key, the length is truncated to the max length specified
 *   at key definition through fmrtDefineKey().
 * After those mandatory parameters, there is a list of
 * parameters that are used to update the fields of the
 * entry that is going to be modified. Parameters are ordered
 * according to the same order used in definition (i.e.
 * in fmrtDefinFields() library call). Be aware that
 * parameters number and types shall be correct, otherwise
 * the call may cause run-time errors. All parameters shall be
 * included in the call, but the only ones that will be updated
 * are those pointed by the paramMask, as explained above.
 * In case of string parameters, the length is truncated to
 * the max length specified at field definition through
 * fmrtDefineFields().
 * ---------------------------------------------------------
 * Possible Return Values:
 * - FMRTOK
 *   The entry has been correctly updated into the tree
 * - FMRTNOTFOUND
 *   The entry with the given key is not present in the table
 * - FMRTKO
 *   Result obtained when this is the first library
 *   call invoked by the caller
 * - FMRTIDNOTFOUND
 *   tableId is not defined
 ***********************************************************/
fmrtResult fmrtModify (fmrtId tableId, fmrtParamMask paramMask, ...)
{
    /* Local Variables */
    va_list     args;
    uint8_t     i,j,maxLen;
    void        *currentPtr;
    fmrtResult   res;
    uint32_t    keyInt,
                fieldInt;
    int32_t     keySigned,
                fieldSigned;
    double      keyDouble,
                fieldDouble;
    time_t      keyTimestamp,
                fieldTimestamp;
    char        keyChar,
                fieldChar,
                *string,
                keyString[MAXFMRTSTRINGLEN+1];
    fmrtParamMask    mask;
    fmrtNodeTraversalStack *traversal;

    /* Call searchTable() internal function to look for the given tableId */
    /* In case of error exit and report error to the calling program      */
    /* To be completely safe, searchTable() shoud be called by locking    */
    /* global mutex (fmrtGlobalMtx), but it is very unlikely that a thread */
    /* tries to access a table which is still under definition, therefore */
    /* we prefer to avoid bottlenecks                                     */
    if ( ((res=searchTable(tableId,&i))!=FMRTOK) )
        return (res);

    /* Set Table specific lock */
    pthread_mutex_lock(&(Tables[i].tableMtx));

    /* Initialize the list of variable arguments in order to read the key first */
    va_start (args,paramMask);
    switch (Tables[i].key.type)
    {
        case FMRTINT:
        {
            keyInt = va_arg (args, uint32_t);
            break;
        }
        case FMRTSIGNED:
        {
            keySigned = va_arg (args, int32_t);
            break;
        }
        case FMRTDOUBLE:
        {
            keyDouble = va_arg (args, double);
            break;
        }
        case FMRTCHAR:
        {
            keyChar = (unsigned char) va_arg (args,int);
            break;
        }
        case FMRTSTRING:
        {   /* Read the key and truncate to the maximum length specified during definition */
            string = va_arg (args,char*);
            maxLen = Tables[i].key.len;     /* This field is max string length + trailing 0 */
            strncpy (keyString,string,maxLen);
            keyString[maxLen-1] = '\0';
            break;
        }
        case FMRTTIMESTAMP:
        {
            if (fmrtTimeFormat[0]=='\0')
                /* time format empty --> read raw timestamp from argument */
                keyTimestamp = va_arg (args, time_t);
            else
            {   /* convert string read from argument to raw timestamp according to fmrtTimeFormat */
                struct tm   TimeFromString;
                string = va_arg (args,char*);
                if (strptime (string, fmrtTimeFormat, &TimeFromString) != NULL)
                    keyTimestamp = mktime (&TimeFromString);
                else
                    keyTimestamp = 0;
            }
            break;
        }   /* case FMRTTIMESTAMP */
    }   /* switch (Tables[i].key.type) */

    /* call searchElem() internal function to look for the element and provide error if result is not FMRTOK */
    if ( (res=searchElem(i, keyInt, keySigned, keyDouble, keyChar, keyString, keyTimestamp, &traversal)) != FMRTOK)
    {
        va_end (args);
        clearNodeTraversalStack (traversal);
        /* Clear the lock before exiting */
        pthread_mutex_unlock(&(Tables[i].tableMtx));
        return (res);
    }

    /* The element was found and traversal is a pointer to a LIFO structure */
    /* whose top element contains the index of the node we searched         */
    /* Set currentPtr to point to the first byte of the structure           */
    currentPtr = Tables[i].fmrtData + (traversal->index)*Tables[i].elemSize;

    /* Now read the variable list of arguments and use them to fill in the fields according to the param mask */
    mask=paramMask;
    for (j=0; j<Tables[i].numFields; j++)
    {   /* Loop through all fields */
        switch (Tables[i].fields[j].type)
        {   /* read all parameters, but update them if and only if the orresponding bit in the mask is set */
            case FMRTINT:
            {
                fieldInt = va_arg (args,uint32_t);
                if (mask%2)
                    *((uint32_t *)(currentPtr+Tables[i].fields[j].delta)) = fieldInt;
                 break;
            }
            case FMRTSIGNED:
            {
                fieldSigned = va_arg (args,int32_t);
                if (mask%2)
                    *((int32_t *)(currentPtr+Tables[i].fields[j].delta)) = fieldSigned;
                 break;
            }
            case FMRTDOUBLE:
            {
                fieldDouble = va_arg (args,double);
                if (mask%2)
                    *((double *)(currentPtr+Tables[i].fields[j].delta)) = fieldDouble;
                 break;
            }
            case FMRTCHAR:
            {
                fieldChar = (unsigned char) va_arg (args,int);
                if (mask%2)
                    *((char *)(currentPtr+Tables[i].fields[j].delta)) = fieldChar;
                break;
            }
            case FMRTSTRING:
            {   /* In case of string exceeding the maximum length it is automatically truncated */
                string = va_arg (args,char*);
                maxLen = Tables[i].fields[j].len;       /* This field is max string length + trailing 0 */
                if (mask%2)
                {
                    strncpy ((char *)(currentPtr+Tables[i].fields[j].delta),string,maxLen);
                    *((char *)(currentPtr+Tables[i].fields[j].delta+maxLen-1)) = '\0';
                }
                break;
            }
            case FMRTTIMESTAMP:
            {
                if (fmrtTimeFormat[0]=='\0')
                    /* time format empty --> read raw timestamp from argument */
                    fieldTimestamp = va_arg (args,time_t);
                else
                {   /* convert string read from argument to raw timestamp according to fmrtTimeFormat */
                    struct tm   TimeFromString;
                    string = va_arg (args, char*);
                    if (strptime (string, fmrtTimeFormat, &TimeFromString) != NULL)
                        fieldTimestamp = mktime (&TimeFromString);
                    else
                        fieldTimestamp = 0;
                }
                if (mask%2)
                    *((time_t *)(currentPtr+Tables[i].fields[j].delta)) = fieldTimestamp;
                 break;
            }
        }   /* switch (Tables[i].fields[j].type) */
        mask>>=1;
    }   /* for (j=0; j<Tables[i].numFields; j++) */
    va_end (args);

    /* Element has been updated - There is no need to rebalance the fmrt tree */

    #ifdef FMRTDEBUG
    printf ("\n\nModified node at index: %d\n",traversal->index);
    __fmrtDebugPrintNode (Tables[i].tableId, traversal->index);
    printf ("Path from node up to the root:\n");
    __fmrtPrintStack(i,traversal);
    #endif

    /* clear node traversal LIFO structure allocated by searchElem() and exit */
    clearNodeTraversalStack (traversal);

    /* Clear the lock before exiting */
    pthread_mutex_unlock(&(Tables[i].tableMtx));

    return (FMRTOK);
}


/***********************************************************
 * fmrtCreateModify()
 * ---------------------------------------------------------
 * This library call is used to insert or update an entry
 * into the fmrt tree. If the entry esists, its data are
 * updated according to the mask specified as second
 * parameter. Rather, if the entry does not exist, it is
 * created. In this case the bitwise mask is meaningless, i.e.
 * all parameters specified are used to fill the content of
 * corresponding fields in the newly created element.
 * The first three parameters (always present) are
 * respectively:
 * - tableId
 *   unique identifier of the table between 0 and 255. The
 *   table shall be defined first through fmrtDefineTable()
 * - paramMask
 *   It is a bitwise mask that is used to identify the
 *   paramaters to be changed (in case of entry already
 *   present in the table). For example, assuming that
 *   5 parameters have been defined through fmrtDefineFields(),
 *   e.g.
 *     fmrtDefineFields(tableId,5,param#0,..,param#1,..param#4)
 *   and that we want to update only parameters 0,1 and 4,
 *   we would set paramMask as follows:
 *     paramMask = 19  -->  binary 10011
 *   The rightmost bit represents the first parameter in
 *   fmrtDefineFields(), the one immediately at its left is the
 *   second parameter, and so on.
 *   This parameter is meaningless in case of new element
 *   created in the table.
 * - key
 *   contains the key value to be searched into the table.
 *   It shall be of the same type defined by the
 *   fmrtDefineKey() call. The library behaviour is undefined
 *   if this constraint is not satisfied. In case of string
 *   key, the length is truncated to the max length specified
 *   at key definition through fmrtDefineKey().
 * After those mandatory parameters, there is a list of
 * parameters that are used to update the fields of the
 * entry that is going to be modified. Parameters are ordered
 * according to the same order used in definition (i.e.
 * in fmrtDefinFields() library call). Be aware that
 * parameters number and types shall be correct, otherwise
 * the call may cause run-time errors. All parameters shall be
 * included in the call, but the only ones that will be updated
 * are those pointed by the paramMask, as explained above.
 * In case of string parameters, the length is truncated to
 * the max length specified at field definition through
 * fmrtDefineFields().
 * ---------------------------------------------------------
 * Possible Return Values:
 * - FMRTOK
 *   The entry has been correctly inserted or updated into
 *   the tree
 * - FMRTKO
 *   Result obtained when this is the first library
 *   call invoked by the caller
 * - FMRTIDNOTFOUND
 *   tableId is not defined
 * - FMRTOUTOFMEMORY
 *   The FMRT tree is full
 ***********************************************************/
fmrtResult fmrtCreateModify (fmrtId tableId, fmrtParamMask paramMask, ...)
{
    /* Local Variables */
    va_list         args;
    uint8_t         i,j,maxLen,duplKey;
    void            *currentPtr;
    fmrtResult      res;
    fmrtIndex       newElement,
                    rebalIndex;
    uint32_t        keyInt,
                    fieldInt[MAXFMRTFIELDNUM];
    int32_t         keySigned,
                    fieldSigned[MAXFMRTFIELDNUM];
    double          keyDouble,
                    fieldDouble[MAXFMRTFIELDNUM];
    time_t          keyTimestamp,
                    fieldTimestamp[MAXFMRTFIELDNUM];
    char            keyChar,
                    fieldChar[MAXFMRTFIELDNUM],
                    *string,
                    keyString[MAXFMRTSTRINGLEN+1],
                    fieldString[MAXFMRTFIELDNUM][MAXFMRTSTRINGLEN+1];
    fmrtParamMask    mask;
    fmrtNodeTraversalStack *traversal,
                           *rebalPtr;

    /* Call searchTable() internal function to look for the given tableId */
    /* In case of error exit and report error to the calling program      */
    /* To be completely safe, searchTable() shoud be called by locking    */
    /* global mutex (fmrtGlobalMtx), but it is very unlikely that a thread */
    /* tries to access a table which is still under definition, therefore */
    /* we prefer to avoid bottlenecks                                     */
    if ( ((res=searchTable(tableId,&i))!=FMRTOK) )
        return (res);

    /* Set Table specific lock */
    pthread_mutex_lock(&(Tables[i].tableMtx));

    /* Initialize the list of variable arguments in order to read the key first */
    va_start (args,paramMask);
    switch (Tables[i].key.type)
    {
        case FMRTINT:
        {
            keyInt = va_arg (args, uint32_t);
            break;
        }
        case FMRTSIGNED:
        {
            keySigned = va_arg (args, int32_t);
            break;
        }
        case FMRTDOUBLE:
        {
            keyDouble = va_arg (args, double);
            break;
        }
        case FMRTCHAR:
        {
            keyChar = (unsigned char) va_arg (args,int);
            break;
        }
        case FMRTSTRING:
        {   /* Read the key and truncate to the maximum length specified during definition */
            string = va_arg (args,char*);
            maxLen = Tables[i].key.len;     /* This field is max string length + trailing 0 */
            strncpy (keyString,string,maxLen);
            keyString[maxLen-1] = '\0';
            break;
        }
        case FMRTTIMESTAMP:
        {
            if (fmrtTimeFormat[0]=='\0')
                /* time format empty --> read raw timestamp from argument */
                keyTimestamp = va_arg (args, time_t);
            else
            {   /* convert string read from argument to raw timestamp according to fmrtTimeFormat */
                struct tm   TimeFromString;
                string = va_arg (args,char*);
                if (strptime (string, fmrtTimeFormat, &TimeFromString) != NULL)
                    keyTimestamp = mktime (&TimeFromString);
                else
                    keyTimestamp = 0;
            }
            break;
        }   /* case FMRTTIMESTAMP */
    }   /* switch (Tables[i].key.type) */

    /* Now read the variable list of arguments and use them to fill in the fields */
    for (j=0; j<Tables[i].numFields; j++)
    {   /* Loop through all fields */
        switch (Tables[i].fields[j].type)
        {
            case FMRTINT:
            {
                fieldInt[j] = va_arg (args,uint32_t);
                break;
            }
            case FMRTSIGNED:
            {
                fieldSigned[j] = va_arg (args,int32_t);
                break;
            }
            case FMRTDOUBLE:
            {
                fieldDouble[j] = va_arg (args,double);
                break;
            }
            case FMRTCHAR:
            {
                fieldChar[j] = (unsigned char) va_arg (args,int);
                break;
            }
            case FMRTSTRING:
            {   /* In case of string exceeding the maximum length it is automatically truncated */
                string = va_arg (args,char*);
                maxLen = Tables[i].fields[j].len;       /* This field is max string length + trailing 0 */
                strncpy (fieldString[j],string,maxLen);
                fieldString[j][maxLen-1] = '\0';
                break;
            }
            case FMRTTIMESTAMP:
            {
                if (fmrtTimeFormat[0]=='\0')
                    /* time format empty --> read raw timestamp from argument */
                    fieldTimestamp[j] = va_arg (args,time_t);
                else
                {   /* convert string read from argument to raw timestamp according to fmrtTimeFormat */
                    struct tm   TimeFromString;
                    string = va_arg (args, char*);
                    if (strptime (string, fmrtTimeFormat, &TimeFromString) != NULL)
                        fieldTimestamp[j] = mktime (&TimeFromString);
                    else
                        fieldTimestamp[j] = 0;
                }
                break;
            }
        }   /* switch (Tables[i].fields[j].type) */
    }   /* for (j=0; j<Tables[i].numFields; j++) */
    va_end (args);

    /* call searchElem() internal function to look for the element and provide error if result is FMRTKO */

    if ( (res=searchElem(i, keyInt, keySigned, keyDouble, keyChar, keyString, keyTimestamp, &traversal)) == FMRTKO)
    {   /* This is a blocking error -> release traversal stack, clear the lock and exit */
        clearNodeTraversalStack (traversal);
        /* Clear the lock before exiting */
        pthread_mutex_unlock(&(Tables[i].tableMtx));
        return (FMRTKO);
    }

    /* If the element is already present set duplKey to 1 and use input parameter mask*/
    if (res==FMRTOK)
    {   /* the element exists and shall be modified changing only parameters specified by paramMask */
        duplKey = 1;
        mask = paramMask;
    }
    else
    {   /* the element does not exist and shall be created - mask is set to all 1's in order to set all elements */
        duplKey = 0;
        mask = -1;
    }

    /* traversal is a pointer to a LIFO structure, while duplKey specifies if the key */
    /* is already present in the table (and shall be overwritten), or is new. In the  */
    /* former case the top element of traversal stack points directly to the index    */
    /* in the FMRT structure, while in the latter it points to the parent on which     */
    /* insertion shall be done                                                        */

    if (duplKey)
    {   /* the element is still present, overwrite data contained into the internal structure */
        /* since the element has been found traversal cannot be NULL in this case             */
        currentPtr = Tables[i].fmrtData + (traversal->index)*Tables[i].elemSize;
    }   /* if (duplKey) */
    else
    {   /* the element is not present -> create it */
        /* Check if this is the first insertion */
        if ( (Tables[i].fmrtData==NULL) || (Tables[i].status==DEFINED) )
        {   /* Initialize Empty elements list */
            res = initEmptyList(i);
            if (res!=FMRTOK)
            {   /* Not able to allocate memory and initialize empty list - very likely we have not enough memory free */
                clearNodeTraversalStack (traversal);
                /* Clear the lock before exiting */
                pthread_mutex_unlock(&(Tables[i].tableMtx));
                return (res);
            }
        }   /* if ( (Tables[i].fmrtData==NULL) ... */

        /* Get an empty element from the list of empty nodes */
        if ( (newElement=getEmptyElem(i)) == FMRTNULLPTR)
        {   /* Not able to fetch an empty element - Probably the table is full */
            clearNodeTraversalStack (traversal);
            /* Clear the lock before exiting */
            pthread_mutex_unlock(&(Tables[i].tableMtx));
            return (FMRTOUTOFMEMORY);
        }

        /* Link the new element to the existing structure (if present) */
        if (traversal==NULL)    /* the stack of nodes traversed is empty -> we are creating the root node (newElement is the root index) */
            Tables[i].fmrtRoot = newElement;
        else if (traversal->go == LEFT)
        {   /* the stack exists and the path from parent node goes through the left subtree */
            currentPtr = Tables[i].fmrtData + (traversal->index)*Tables[i].elemSize;
            *((fmrtIndex*)currentPtr) = newElement;
        }
        else
        {   /* the stack exists and the path from parent node goes through the right subtree */
            currentPtr = Tables[i].fmrtData + (traversal->index)*Tables[i].elemSize + sizeof(fmrtIndex);
            *((fmrtIndex*)currentPtr) = newElement;
        }

        /* Set currentPtr to point to this new element, which is always a leaf (at least initially) */
        currentPtr = Tables[i].fmrtData + newElement*Tables[i].elemSize;

        /* Insert null pointers to left and right subtree */
        *((fmrtIndex*)currentPtr) = FMRTNULLPTR;
        *((fmrtIndex*)(currentPtr+sizeof(fmrtIndex))) = FMRTNULLPTR;

        Tables[i].status = NOTEMPTY;
        Tables[i].currentNumElem += 1;
    }   /* else if (duplKey) */

    /* Now currentPtr points to the element that shall be filled, in both cases of new element or existing one */
    /* Fill the element with the key and the fields read from CSV file */
    switch (Tables[i].key.type)
    {
        case FMRTINT:
        {
            *((uint32_t *)(currentPtr+Tables[i].key.delta)) = keyInt;
            break;
        }
        case FMRTSIGNED:
        {
            *((int32_t *)(currentPtr+Tables[i].key.delta)) = keySigned;
            break;
        }
        case FMRTDOUBLE:
        {
            *((double *)(currentPtr+Tables[i].key.delta)) = keyDouble;
            break;
        }
        case FMRTCHAR:
        {
            *((char *)(currentPtr+Tables[i].key.delta)) = keyChar;
            break;
        }
        case FMRTSTRING:
        {   /* Please observe that keyString has been truncated when it has been read from the input csv file */
            strcpy ((char *)(currentPtr+Tables[i].key.delta),keyString);
            break;
        }
        case FMRTTIMESTAMP:
        {
            *((time_t *)(currentPtr+Tables[i].key.delta)) = keyTimestamp;
            break;
        }
    }   /* switch (Tables[i].key.type) */

    /* Now read the variable list of arguments and use them to fill in the fields */
    for (j=0; j<Tables[i].numFields; j++)
    {   /* Loop through all fields */
        switch (Tables[i].fields[j].type)
        {
            case FMRTINT:
            {
                if (mask%2)
                    *((uint32_t *)(currentPtr+Tables[i].fields[j].delta)) = fieldInt[j];
                break;
            }
            case FMRTSIGNED:
            {
                if (mask%2)
                    *((int32_t *)(currentPtr+Tables[i].fields[j].delta)) = fieldSigned[j];
                break;
            }
            case FMRTDOUBLE:
            {
                if (mask%2)
                    *((double *)(currentPtr+Tables[i].fields[j].delta)) = fieldDouble[j];
                break;
            }
            case FMRTCHAR:
            {
                if (mask%2)
                    *((char *)(currentPtr+Tables[i].fields[j].delta)) = fieldChar[j];
                break;
            }
            case FMRTSTRING:
            {   /* Please observe that fieldsString[j] has been truncated when it has been read from the input csv file */
                if (mask%2)
                    strcpy ((char *)(currentPtr+Tables[i].fields[j].delta),fieldString[j]);
                break;
            }
            case FMRTTIMESTAMP:
            {
                if (mask%2)
                    *((time_t *)(currentPtr+Tables[i].fields[j].delta)) = fieldTimestamp[j];
                break;
            }
        }   /* switch (Tables[i].fields[j].type) */
        mask>>=1;
    }   /* for (j=0; j<Tables[i].numFields; j++) */

    /* Element has been inserted - if duplKey==0 (i.e. new element) go through the   */
    /* traversal LIFO structure and rebalance fmrt tree starting from the bottom and  */
    /* going up to the root. This shal not be done if duplKey==1 (the element already*/
    /* exists and it is assumed that it is already balanced                          */
    if (duplKey==0)
    {
        rebalPtr = traversal;   /* start traversing from the top of the stack, i.e. the parent of the node just inserted) */
        while (rebalPtr!=NULL)
        {   /* rebalance the subtree whose root is the current node */
            rebalIndex = rebalanceSubTree (i,rebalPtr->index);  /* the root might change due to rotations */
            /* go up to the parent */
            rebalPtr = rebalPtr->next;
            if (rebalPtr!=NULL)
            {   /* There is a parent node - update pointer (left or right depending on the content of traversal structure) */
                if (rebalPtr->go == LEFT)
                    currentPtr = Tables[i].fmrtData + (rebalPtr->index)*Tables[i].elemSize;
                else
                    currentPtr = Tables[i].fmrtData + (rebalPtr->index)*Tables[i].elemSize+sizeof(fmrtIndex);
                /* The proper pointer is updated with the output of the rebalance structure */
                *((fmrtIndex*)currentPtr) = rebalIndex;
            }   /* if (rebalPtr!=NULL) */
            else
                /* There is no parent node - Rotation implies a change of the fmrt root pointer */
                Tables[i].fmrtRoot = rebalIndex;
        }   /* while (rebalPtr!=NULL) */
    }   /* if (duplKey==0) */

    #ifdef FMRTDEBUG
    printf ("\n\nCreateModify node at index: %d\n",traversal->index);
    __fmrtDebugPrintNode (Tables[i].tableId, traversal->index);
    printf ("Path from node up to the root:\n");
    __fmrtPrintStack(i,traversal);
    #endif

    /* clear node traversal LIFO structure allocated by searchElem() and exit */
    clearNodeTraversalStack (traversal);

    /* Clear the lock before exiting */
    pthread_mutex_unlock(&(Tables[i].tableMtx));

    return (FMRTOK);

}


/***********************************************************
 * fmrtDelete()
 * ---------------------------------------------------------
 * This library call is used to delete an existing entry
 * from the fmrt tree. It is a call with two parameters,
 * respectively:
 * - tableId
 *   unique identifier of the table between 0 and 255. The
 *   table shall be defined first through fmrtDefineTable()
 * - key
 *   contains the key value to be searched into the table.
 *   It shall be of the same type defined by the
 *   fmrtDefineKey() call. The library behaviour is undefined
 *   if this constraint is not satisfied. In case of string
 *   key, the length is truncated to the max length specified
 *   at key definition through fmrtDefineKey().
 * ---------------------------------------------------------
 * Possible Return Values:
 * - FMRTOK
 *   The entry has been correctly deleted from the tree
 * - FMRTNOTFOUND
 *   The entry with the given key is not present in the table
 * - FMRTKO
 *   Result obtained when this is the first library
 *   call invoked by the caller
 * - FMRTIDNOTFOUND
 *   tableId is not defined
 ***********************************************************/
fmrtResult fmrtDelete (fmrtId tableId, ...)
{
    /* Local Variables */
    va_list     args;
    uint8_t     i,maxLen;
    void        *currentPtr;
    fmrtResult   res;
    uint32_t    keyInt;
    int32_t     keySigned;
    double      keyDouble;
    time_t      keyTimestamp;
    char        keyChar,
                *string,
                keyString[MAXFMRTSTRINGLEN+1];
    fmrtIndex    leftSubtree,
                rightSubtree,
                leftmost,
                leftmostRightChild,
                rebalIndex;
    fmrtNodeTraversalStack   *traversal,
                            *toLeaf,
                            *rebalPtr;


    /* Call searchTable() internal function to look for the given tableId */
    /* In case of error exit and report error to the calling program      */
    /* To be completely safe, searchTable() shoud be called by locking    */
    /* global mutex (fmrtGlobalMtx), but it is very unlikely that a thread */
    /* tries to access a table which is still under definition, therefore */
    /* we prefer to avoid bottlenecks                                     */
    if ( ((res=searchTable(tableId,&i))!=FMRTOK) )
        return (res);

    /* Set Table specific lock */
    pthread_mutex_lock(&(Tables[i].tableMtx));

    /* Initialize the list of variable arguments in order to read the key */
    va_start (args,tableId);
    switch (Tables[i].key.type)
    {
        case FMRTINT:
        {
            keyInt = va_arg (args, uint32_t);
            break;
        }
        case FMRTSIGNED:
        {
            keySigned = va_arg (args, int32_t);
            break;
        }
        case FMRTDOUBLE:
        {
            keyDouble = va_arg (args, double);
            break;
        }
        case FMRTCHAR:
        {
            keyChar = (unsigned char) va_arg (args,int);
            break;
        }
        case FMRTSTRING:
        {   /* Read the key and truncate to the maximum length specified during definition */
            string = va_arg (args,char*);
            maxLen = Tables[i].key.len;     /* This field is max string length + trailing 0 */
            strncpy (keyString,string,maxLen);
            keyString[maxLen-1] = '\0';
            break;
        }
        case FMRTTIMESTAMP:
        {
            if (fmrtTimeFormat[0]=='\0')
                /* time format empty --> read raw timestamp from argument */
                keyTimestamp = va_arg (args, time_t);
            else
            {   /* convert string read from argument to raw timestamp according to fmrtTimeFormat */
                struct tm   TimeFromString;
                string = va_arg (args,char*);
                if (strptime (string, fmrtTimeFormat, &TimeFromString) != NULL)
                    keyTimestamp = mktime (&TimeFromString);
                else
                    keyTimestamp = 0;
            }
            break;
        }   /* case FMRTTIMESTAMP */
    }   /* switch (Tables[i].key.type) */
    va_end (args);

    /* call searchElem() internal function to look for the element and provide error if result is not FMRTOK */
    if ( (res=searchElem(i, keyInt, keySigned, keyDouble, keyChar, keyString, keyTimestamp, &traversal)) != FMRTOK)
    {
        clearNodeTraversalStack (traversal);
        /* Clear the lock before exiting */
        pthread_mutex_unlock(&(Tables[i].tableMtx));
        return (res);
    }

    /* The element was found and traversal is a pointer to a LIFO structure */
    /* whose top element contains the index of the node we searched         */
    /* Set currentPtr to point to the first byte of the structure           */
    currentPtr = Tables[i].fmrtData + (traversal->index)*Tables[i].elemSize;

    /* Extract left and right subtree pointers associated to the node to be deleted and initialize a pointer needed in case of leftMostChild() search */
    leftSubtree = *((fmrtIndex *) (currentPtr));
    rightSubtree = *((fmrtIndex *) (currentPtr+sizeof(fmrtIndex)));
    toLeaf = NULL;

    /* Now there are 3 possible cases:                                              */
    /* 1 - the node is a leaf           -> delete it                                */
    /* 2 - the node has only one child  -> substitute the content of the current    */
    /*                                     node with the child and delete the child */
    /* 3 - the node has both subtrees   -> find the left most child on the right    */
    /*                                     subtree, substitute the content of the   */
    /*                                     current node with it and delete it       */

    if ( (leftSubtree==FMRTNULLPTR) && (rightSubtree==FMRTNULLPTR) )
    {   /* case 1 - the node is a leaf */
        rebalPtr = traversal->next;
        if (rebalPtr!=NULL)
        {   /* the leaf we are deleting is not the root */
            /* Set left or right pointer of the parent (depending on content of traversal LIFO) to FMRTNULLPTR */
            currentPtr = Tables[i].fmrtData + (rebalPtr->index)*Tables[i].elemSize;
            if (rebalPtr->go == LEFT)
                *((fmrtIndex *) (currentPtr)) = FMRTNULLPTR;
            else
                *((fmrtIndex *) (currentPtr+sizeof(fmrtIndex))) = FMRTNULLPTR;
        }   /* if (rebalPtr!=NULL) */
        else
        {   /* the node we are deleting is a leaf, but it is also the root */
            /*  Set fmrt Root pointer to FMRTNULLPTR */
            Tables[i].fmrtRoot = FMRTNULLPTR;
        }
        /* return deleted element to the empty list and remove element from traversal LIFO */
        freeEmptyElem (i,traversal->index);
        free (traversal);
        traversal = rebalPtr;
    }   /* if ( (leftSubtree==FMRTNULLPTR) && (rightSubtree==FMRTNULLPTR) ) */

    else if ( (leftSubtree!=FMRTNULLPTR) && (rightSubtree!=FMRTNULLPTR) )
    {   /* case 3 - the node has both subtrees */
        /* find the leftmost child on the right subtree and substitute its content with the node to delete */
        leftmost = leftMostChild(i,rightSubtree,&toLeaf);
        copyNode (i,traversal->index,leftmost);
        currentPtr = Tables[i].fmrtData + leftmost*Tables[i].elemSize;
        /* The leftmost child on the right subtree is either a leaf or has just one child on the right subtree - there are no other possibilities */
        leftmostRightChild = *((fmrtIndex *) (currentPtr+sizeof(fmrtIndex)));
        if (leftmostRightChild==FMRTNULLPTR)
        {   /* The leftmost child on the right subtree is a leaf */
            freeEmptyElem (i,leftmost);
            rebalPtr = toLeaf->next;
            free (toLeaf);
            toLeaf=rebalPtr;
            if (toLeaf!=NULL)
            {
                currentPtr = Tables[i].fmrtData + (toLeaf->index)*Tables[i].elemSize;
                *((fmrtIndex *) (currentPtr)) = FMRTNULLPTR;
            }
        }   /* if (leftmostRightChild==FMRTNULLPTR) */
        else
        {
            copyNode (i,leftmost,leftmostRightChild);
            freeEmptyElem (i,leftmostRightChild);
            if (toLeaf!=NULL)
            {
                currentPtr = Tables[i].fmrtData + (toLeaf->index)*Tables[i].elemSize;
                *((fmrtIndex *) (currentPtr+sizeof(fmrtIndex))) = FMRTNULLPTR;
            }
        }   /* else if (leftmostRightChild==FMRTNULLPTR) */
        /* join traversal and toLeaf LIFO traversal structures to allow rebalance on the whole path */
        traversal->go = RIGHT;
        rebalPtr = toLeaf;
        while (rebalPtr != NULL)
        {
            if (rebalPtr->next==NULL)
            {
                rebalPtr->next=traversal;
                traversal = toLeaf;
                rebalPtr = NULL;
            }
            else
                rebalPtr=rebalPtr->next;
        }   /* while (toLeaf != NULL) */

    }   /* if ( (leftSubtree!=FMRTNULLPTR) && (rightSubtree!=FMRTNULLPTR) ) */

    else
    {   /* case 2 - the node has only one child -> Since the tree is balanced, this child cannot have further children -> this child is a leaf */
        if (leftSubtree!=FMRTNULLPTR)
        {   /* the child is on the left subtree */
            /* copy the content of the child into the node to be deleted */
            /* update the pointer and return the child to the list of empty nodes */
            copyNode (i,traversal->index,leftSubtree);
            freeEmptyElem (i,leftSubtree);
            currentPtr = Tables[i].fmrtData + (traversal->index)*Tables[i].elemSize;
            *((fmrtIndex *) (currentPtr)) = FMRTNULLPTR;
        }   /* if (leftSubtree!=FMRTNULLPTR) */
        else
        {   /* the child is on the right subtree */
            /* copy the content of the child into the node to be deleted */
            /* update the pointer and return the child to the list of empty nodes */
            copyNode (i,traversal->index,rightSubtree);
            freeEmptyElem (i,rightSubtree);
            currentPtr = Tables[i].fmrtData + (traversal->index)*Tables[i].elemSize;
            *((fmrtIndex *) (currentPtr+sizeof(fmrtIndex))) = FMRTNULLPTR;
        }   /* else if (leftSubtree!=FMRTNULLPTR) */
    }   /* else if ( (leftSubtree!=FMRTNULLPTR) && (rightSubtree!=FMRTNULLPTR) ) */

    /* Element has been deleted, now rebalance the FMRT tree */
    /* starting from the bottom and going up to the root    */
    rebalPtr = traversal;   /* start traversing from the top of the stack, i.e. from the leaf */
    while (rebalPtr!=NULL)
    {   /* rebalance the subtree whose root is the current node */
        rebalIndex = rebalanceSubTree (i,rebalPtr->index);  /* the root might change due to rotations */
        /* go up to the parent */
        rebalPtr = rebalPtr->next;
        if (rebalPtr!=NULL)
        {   /* There is a parent node - update pointer (left or right depending on the content of traversal structure) */
            if (rebalPtr->go == LEFT)
                currentPtr = Tables[i].fmrtData + (rebalPtr->index)*Tables[i].elemSize;
            else
                currentPtr = Tables[i].fmrtData + (rebalPtr->index)*Tables[i].elemSize+sizeof(fmrtIndex);
            /* The proper pointer is updated with the output of the rebalance structure */
            *((fmrtIndex*)currentPtr) = rebalIndex;
        }   /* if (rebalPtr!=NULL) */
        else
            /* There is no parent node - Rotation implies a change of the fmrt root pointer */
            Tables[i].fmrtRoot = rebalIndex;
    }   /* while (rebalPtr!=NULL) */

    #ifdef FMRTDEBUG
    printf ("\n\nDeleted node\n");
    printf ("Path from deleted node up to the root:\n");
    __fmrtPrintStack(i,traversal);
    #endif

    /* set Tables[i].status, increment number of stored elements, clear node traversal LIFO structure allocated by searchElem() and exit */
    Tables[i].currentNumElem -= 1;
    clearNodeTraversalStack (traversal);

    /* Clear the lock before exiting */
    pthread_mutex_unlock(&(Tables[i].tableMtx));

    return (FMRTOK);
}


/***********************************************************
 * fmrtImportTableCsv()
 * ---------------------------------------------------------
 * This library call is used to import the content of a
 * given file in CSV format (specified by the file pointer
 * given by the second parameter) into the table whose id
 * is provided by the first parameter.
 * It takes the following parameters:
 * - tableId
 *   unique identifier of the table between 0 and 255. The
 *   table shall be defined first through fmrtDefineTable()
 * - filePtr
 *   pointer to a file opened by the caller in read
 *   mode. It cannot be NULL, otherwise an error will be
 *   provided
 * - separator
 *   It is a char specified by the caller that is recognized
 *   as a separator between consecutive fields into the
 *   input CSV file
 * - lines
 *   is a pointer to an integer parameter that is provided
 *   back by the call. It contains either the total number
 *   of lines read from the file (if result is FMRTOK) or the
 *   line number affected by the error (in case of error)
 * Please note that the file shall be opened before calling
 * this function, otherwise a run-time error will occur.
 * Similarly, the function call does not close the input
 * file, which must be closed by the caller.
 * Data read from the file are appended to existing data
 * in the table (if the input table is not empty).
 * In case of duplicate key, the entry is overwritten:
 * no errors are provided in this case
 * ---------------------------------------------------------
 * Possible Return Values:
 * - FMRTOK
 *   The table has been successfully imported from CSV file.
 *   The last parameter contains the total number of lines
 *   read from the file
 * - FMRTKO
 *   Result obtained when this is the first library call
 *   invoked by the caller or when the specified file
 *   pointer is NULL. In those cases the last parameter
 *   provides 0. This result code is also obtained when
 *   an error is detected while reading input lines
 *   from the file (e.g. incomplete line); in such cases
 *   the last parameter reports the line where the error
 *   has been detected. Elements read from the file up to
 *   the wrong line are inserted into the table
 * - FMRTIDNOTFOUND
 *   tableId is not defined
 * - FMRTOUTOFMEMORY
 *   while inserting data into the table from the CSV file
 *   the maximum number of elements has been reached (the
 *   maximum number of elements is specified at table
 *   definition as a parameter of fmrtDefineTable() ). The
 *   last parameter identifies the line in the input file
 *   where data import was stopped.
 ***********************************************************/
fmrtResult fmrtImportTableCsv (fmrtId tableId, FILE *filePtr, char separator, int *lines)
{
    /* Local Variables */
    char                    *p,
                            *q,
                            keyChar,
                            fieldChar[MAXFMRTFIELDNUM],
                            keyString[MAXFMRTSTRINGLEN+1],
                            fieldString[MAXFMRTFIELDNUM][MAXFMRTSTRINGLEN+1],
                            inputString[MAXCSVLINELEN];
    void                    *currentPtr;
    uint8_t                 i,j, duplKey, maxLen;
    uint32_t                keyInt,
                            fieldInt[MAXFMRTFIELDNUM];
    int32_t                 keySigned,
                            fieldSigned[MAXFMRTFIELDNUM];
    double                  keyDouble,
                            fieldDouble[MAXFMRTFIELDNUM];
    time_t                  keyTimestamp,
                            fieldTimestamp[MAXFMRTFIELDNUM];
    fmrtResult              res;
    fmrtIndex               newElement,
                            rebalIndex;
    fmrtNodeTraversalStack *traversal,
                           *rebalPtr;

    /* Reset line counter */
    *lines=0;

    /* If file pointer is NULL, return FMRTKO */
    if (filePtr==NULL)
        return (FMRTKO);

    /* Call searchTable() internal function to look for the given tableId */
    /* In case of error exit and report error to the calling program      */
    /* To be completely safe, searchTable() shoud be called by locking    */
    /* global mutex (fmrtGlobalMtx), but it is very unlikely that a thread */
    /* tries to access a table which is still under definition, therefore */
    /* we prefer to avoid bottlenecks                                     */
    if ( ((res=searchTable(tableId,&i))!=FMRTOK) )
        return (res);

    /* Set Table specific lock */
    pthread_mutex_lock(&(Tables[i].tableMtx));

    /* Read lines from CSV input file and fill the structure */
    while (fgets (inputString,MAXCSVLINELEN,filePtr))
    {
        (*lines) += 1;
        /* skip empty lines */
        if (inputString[0] == '\0')
            continue;

        /* skip leading spaces and tabs */
        p = inputString;
        while ( (*p==' ') || (*p=='\t') )
            p++;

        /* Check whether this is an empty line or a comment and, if so, skip this line */
        if ( (*p=='\0') || (*p=='#') )
            continue ;

        /* the line is neither a comment nor an empty line - therefore we assume it is a valid line */
        for (q=p; (*q!='\0')&&(*q!=separator)&&(*q!='\n'); q+=1); /* stop at EOL or at separator */
        if ( (p==q) && (*q=='\n') )
            continue;
        *q = '\0';  /* p and q now points to the beginning and to the end of the first parameter, which should be the key */

        switch (Tables[i].key.type)
        {
            case FMRTINT:
            {
                keyInt = atoi (p);
                break;
            }
            case FMRTSIGNED:
            {
                keySigned = atoi (p);
                break;
            }
            case FMRTDOUBLE:
            {
                keyDouble = atof (p);
                break;
            }
            case FMRTCHAR:
            {
                keyChar = *p;
                break;
            }
            case FMRTSTRING:
            {   /* Read the key and truncate to the maximum length specified during definition */
                maxLen = Tables[i].key.len;    /* This field is max string length + trailing 0 */
                strncpy (keyString,p,maxLen);
                keyString[maxLen-1] = '\0';
                break;
            }
            case FMRTTIMESTAMP:
            {
                if (fmrtTimeFormat[0]=='\0')
                    /* time format empty --> read raw timestamp from input line */
                    keyTimestamp = atol (p);
                else
                {   /* convert string read from input line to raw timestamp according to fmrtTimeFormat */
                    struct tm   TimeFromString;
                    if (strptime (p, fmrtTimeFormat, &TimeFromString) != NULL)
                        keyTimestamp = mktime (&TimeFromString);
                    else
                        keyTimestamp = 0;
                }
                break;
            }   /* case FMRTTIMESTAMP */
        }   /* switch (Tables[i].key.type) */

        for (j=0; j<Tables[i].numFields; j++)
        {   /* Loop through all fields */
            p=q+1;
            if (*p=='\0')
                break;

            for (q=p; (*q!='\0')&&(*q!=separator)&&(*q!='\n'); q+=1); /* stop at EOL or at separator */
            *q = '\0';  /* p and q now points to the beginning and to the end of the j-th parameter  */

            switch (Tables[i].fields[j].type)
            {
                case FMRTINT:
                {
                    fieldInt[j] = atoi (p);
                    break;
                }
                case FMRTSIGNED:
                {
                    fieldSigned[j] = atoi (p);
                    break;
                }
                case FMRTDOUBLE:
                {
                    fieldDouble[j] = atof (p);
                    break;
                }
                case FMRTCHAR:
                {
                    fieldChar[j] = *p;
                    break;
                }
                case FMRTSTRING:
                {   /* In case of string exceeding the maximum length it is automatically truncated */
                    maxLen = Tables[i].fields[j].len;    /* This field is max string length + trailing 0 */
                    strncpy (fieldString[j],p,maxLen);
                    fieldString[j][maxLen-1] = '\0';
                    break;
                }
            case FMRTTIMESTAMP:
            {
                if (fmrtTimeFormat[0]=='\0')
                    /* time format empty --> read raw timestamp from input line */
                    fieldTimestamp[j] = atol (p);
                else
                {   /* convert string read from input line to raw timestamp according to fmrtTimeFormat */
                    struct tm   TimeFromString;
                    if (strptime (p, fmrtTimeFormat, &TimeFromString) != NULL)
                        fieldTimestamp[j] = mktime (&TimeFromString);
                    else
                        fieldTimestamp[j] = 0;
                }
                break;
            }   /* case FMRTTIMESTAMP */
            }   /* switch (Tables[i].fields[j].type) */
        }   /* for (j=0; j<Tables[i].numFields; j++) */

        if (j<Tables[i].numFields)
        {   /* This is a blocking error -> clear the lock and exit */
            pthread_mutex_unlock(&(Tables[i].tableMtx));
            return (FMRTKO);
        }

        /* call searchElem() internal function to look for the element and provide error if result is FMRTKO */
        duplKey = 0;
        if ( (res=searchElem(i, keyInt, keySigned, keyDouble, keyChar, keyString, keyTimestamp, &traversal)) == FMRTKO)
        {   /* This is a blocking error -> release traversal stack, clear the lock and exit */
            clearNodeTraversalStack (traversal);
            /* Clear the lock before exiting */
            pthread_mutex_unlock(&(Tables[i].tableMtx));
            return (FMRTKO);
        }

        /* If the element is already present set duplKey to 1 */
        if (res==FMRTOK)
            duplKey = 1;

        /* traversal is a pointer to a LIFO structure, while duplKey specifies if the key */
        /* is already present in the table (and shall be overwritten), or is new. In the  */
        /* former case the top element of traversal stack points directly to the index    */
        /* in the FMRT structure, while in the latter it points to the parent on which     */
        /* insertion shall be done                                                        */

        if (duplKey)
        {   /* the element is still present, overwrite data contained into the internal structure with those read from CSV */
            /* since the element has been found traversal cannot be NULL in this case */
            currentPtr = Tables[i].fmrtData + (traversal->index)*Tables[i].elemSize;
        }   /* if (duplKey) */
        else
        {   /* the element is not present -> create it */
            /* Check if this is the first insertion */
            if ( (Tables[i].fmrtData==NULL) || (Tables[i].status==DEFINED) )
            {   /* Initialize Empty elements list */
                res = initEmptyList(i);
                if (res!=FMRTOK)
                {   /* Not able to allocate memory and initialize empty list - very likely we have not enough memory free */
                    clearNodeTraversalStack (traversal);
                    /* Clear the lock before exiting */
                    pthread_mutex_unlock(&(Tables[i].tableMtx));
                    return (res);
                }
            }   /* if ( (Tables[i].fmrtData==NULL) ... */

            /* Get an empty element from the list of empty nodes */
            if ( (newElement=getEmptyElem(i)) == FMRTNULLPTR)
            {   /* Not able to fetch an empty element - Probably the table is full */
                clearNodeTraversalStack (traversal);
                /* Clear the lock before exiting */
                pthread_mutex_unlock(&(Tables[i].tableMtx));
                return (FMRTOUTOFMEMORY);
            }

            /* Link the new element to the existing structure (if present) */
            if (traversal==NULL)    /* the stack of nodes traversed is empty -> we are creating the root node (newElement is the root index) */
                Tables[i].fmrtRoot = newElement;
            else if (traversal->go == LEFT)
            {   /* the stack exists and the path from parent node goes through the left subtree */
                currentPtr = Tables[i].fmrtData + (traversal->index)*Tables[i].elemSize;
                *((fmrtIndex*)currentPtr) = newElement;
            }
            else
            {   /* the stack exists and the path from parent node goes through the right subtree */
                currentPtr = Tables[i].fmrtData + (traversal->index)*Tables[i].elemSize + sizeof(fmrtIndex);
                *((fmrtIndex*)currentPtr) = newElement;
            }

            /* Set currentPtr to point to this new element, which is always a leaf (at least initially) */
            currentPtr = Tables[i].fmrtData + newElement*Tables[i].elemSize;

            /* Insert null pointers to left and right subtree */
            *((fmrtIndex*)currentPtr) = FMRTNULLPTR;
            *((fmrtIndex*)(currentPtr+sizeof(fmrtIndex))) = FMRTNULLPTR;

            Tables[i].status = NOTEMPTY;
            Tables[i].currentNumElem += 1;
        }   /* else if (duplKey) */

        /* Now currentPtr points to the element that shall be filled, in both cases of new element or existing one */
        /* Fill the element with the key and the fields read from CSV file */
        switch (Tables[i].key.type)
        {
            case FMRTINT:
            {
                *((uint32_t *)(currentPtr+Tables[i].key.delta)) = keyInt;
                break;
            }
            case FMRTSIGNED:
            {
                *((int32_t *)(currentPtr+Tables[i].key.delta)) = keySigned;
                break;
            }
            case FMRTDOUBLE:
            {
                *((double *)(currentPtr+Tables[i].key.delta)) = keyDouble;
                break;
            }
            case FMRTCHAR:
            {
                *((char *)(currentPtr+Tables[i].key.delta)) = keyChar;
                break;
            }
            case FMRTSTRING:
            {   /* Please observe that keyString has been truncated when it has been read from the input csv file */
                strcpy ((char *)(currentPtr+Tables[i].key.delta),keyString);
                break;
            }
            case FMRTTIMESTAMP:
            {
                *((time_t *)(currentPtr+Tables[i].key.delta)) = keyTimestamp;
                break;
            }
        }   /* switch (Tables[i].key.type) */

        /* Now read the variable list of arguments and use them to fill in the fields */
        for (j=0; j<Tables[i].numFields; j++)
        {   /* Loop through all fields */
            switch (Tables[i].fields[j].type)
            {
                case FMRTINT:
                {
                    *((uint32_t *)(currentPtr+Tables[i].fields[j].delta)) = fieldInt[j];
                    break;
                }
                case FMRTSIGNED:
                {
                    *((int32_t *)(currentPtr+Tables[i].fields[j].delta)) = fieldSigned[j];
                    break;
                }
                case FMRTDOUBLE:
                {
                    *((double *)(currentPtr+Tables[i].fields[j].delta)) = fieldDouble[j];
                    break;
                }
                case FMRTCHAR:
                {
                    *((char *)(currentPtr+Tables[i].fields[j].delta)) = fieldChar[j];
                    break;
                }
                case FMRTSTRING:
                {   /* Please observe that fieldsString[j] has been truncated when it has been read from the input csv file */
                    strcpy ((char *)(currentPtr+Tables[i].fields[j].delta),fieldString[j]);
                    break;
                }
                case FMRTTIMESTAMP:
                {
                    *((time_t *)(currentPtr+Tables[i].fields[j].delta)) = fieldTimestamp[j];
                    break;
                }
            }   /* switch (Tables[i].fields[j].type) */
        }   /* for (j=0; j<Tables[i].numFields; j++) */

        /* Element has been inserted - if duplKey==0 (i.e. new element) go through the   */
        /* traversal LIFO structure and rebalance fmrt tree starting from the bottom and  */
        /* going up to the root. This shal not be done if duplKey==1 (the element already*/
        /* exists and it is assumed that it is already balanced                          */
        if (duplKey==0)
        {
            rebalPtr = traversal;   /* start traversing from the top of the stack, i.e. the parent of the node just inserted) */
            while (rebalPtr!=NULL)
            {   /* rebalance the subtree whose root is the current node */
                rebalIndex = rebalanceSubTree (i,rebalPtr->index);  /* the root might change due to rotations */
                /* go up to the parent */
                rebalPtr = rebalPtr->next;
                if (rebalPtr!=NULL)
                {   /* There is a parent node - update pointer (left or right depending on the content of traversal structure) */
                    if (rebalPtr->go == LEFT)
                        currentPtr = Tables[i].fmrtData + (rebalPtr->index)*Tables[i].elemSize;
                    else
                        currentPtr = Tables[i].fmrtData + (rebalPtr->index)*Tables[i].elemSize+sizeof(fmrtIndex);
                    /* The proper pointer is updated with the output of the rebalance structure */
                    *((fmrtIndex*)currentPtr) = rebalIndex;
                }   /* if (rebalPtr!=NULL) */
                else
                    /* There is no parent node - Rotation implies a change of the fmrt root pointer */
                    Tables[i].fmrtRoot = rebalIndex;
            }   /* while (rebalPtr!=NULL) */
        }   /* if (duplKey==0) */

        clearNodeTraversalStack (traversal);

    }   /* while (fgets (inputString)... */

    /* Clear the lock before exiting */
    pthread_mutex_unlock(&(Tables[i].tableMtx));

    return (FMRTOK);
}


/***********************************************************
 * fmrtExportTableCsv()
 * ---------------------------------------------------------
 * This library call is used to export the content of the
 * given table into a specified file in CSV format.
 * It takes the following parameters:
 * - tableId
 *   unique identifier of the table between 0 and 255. The
 *   table shall be defined first through fmrtDefineTable()
 * - filePtr
 *   pointer to a file opened by the caller in write/append
 *   mode. The output will be written on that file. If NULL
 *   output is written on stdout
 * - separator
 *   It is a char specified by the caller that is used to
 *   separate fields into the output
 * - reverseFlag
 *   if 0 the table is exported in ascending order with
 *   respect to the key, otherwise descending order is used
 * Please note that the file shall be opened before calling
 * this function, otherwise a run-time error will occur.
 * Similarly, the function call does not close the output
 * file, which must be closed by the caller
 * ---------------------------------------------------------
 * Possible Return Values:
 * - FMRTOK
 *   Export was successful
 * - FMRTKO
 *   Result obtained when this is the first library
 *   call invoked by the caller
 * - FMRTIDNOTFOUND
 *   tableId is not defined
 ***********************************************************/
fmrtResult fmrtExportTableCsv (fmrtId tableId, FILE *filePtr, char separator, uint8_t reverseFlag)
{
    /* Local Variables */
    uint8_t     i,j;
    fmrtResult   res;

    /* Call searchTable() internal function to look for the given tableId */
    /* In case of error exit and report error to the calling program      */
    /* To be completely safe, searchTable() shoud be called by locking    */
    /* global mutex (fmrtGlobalMtx), but it is very unlikely that a thread */
    /* tries to access a table which is still under definition, therefore */
    /* we prefer to avoid bottlenecks                                     */
    if ( ((res=searchTable(tableId,&i))!=FMRTOK) )
        return (res);

    /* Set Table specific lock */
    pthread_mutex_lock(&(Tables[i].tableMtx));

    /* if file pointer is NULL, print output on stdout */
    if (filePtr==NULL)
        filePtr = stdout;

    fprintf (filePtr, "#Table: %s (Id: %d)\n",Tables[i].tableName, Tables[i].tableId);
    fprintf (filePtr, "#%s", Tables[i].key.name);
    for (j=0; j<Tables[i].numFields; j++)
        fprintf (filePtr, "%c%s", separator, Tables[i].fields[j].name);
    fprintf (filePtr,"\n");

    /* Start recursion from root node */
    exportTableRecurse (i, Tables[i].fmrtRoot, filePtr, separator, reverseFlag);

    /* Clear the lock before exiting */
    pthread_mutex_unlock(&(Tables[i].tableMtx));

    return (FMRTOK);
}


/***********************************************************
 * fmrtExportRangeCsv()
 * ---------------------------------------------------------
 * This library call is used to export a subset of the
 * content of the given table into a specified file in
 * CSV format. Specifically, the export contains all the
 * table entries in the range between a minimum and a maximum
 * key value (specified by the last two parameters).
 * It takes the following parameters:
 * - tableId
 *   unique identifier of the table between 0 and 255. The
 *   table shall be defined first through fmrtDefineTable()
 * - filePtr
 *   pointer to a file opened by the caller in write/append
 *   mode. The output will be written on that file. If NULL
 *   output is written on stdout
 * - separator
 *   It is a char specified by the caller that is used to
 *   separate fields into the output
 * - reverseFlag
 *   if 0 the table is exported in ascending order with
 *   respect to the key, otherwise descending order is used
 * - keyMin
 *   Minimum value of the key. It must be of the same type
 *   specified at key definition (fmrtDefineKey() call).
 *   Library call behaviour is undefined if this condition
 *   is not satisfied
 * - keyMax
 *   Maximum value of the key. It must be of the same type
 *   specified at key definition (fmrtDefineKey() call).
 *   Library call behaviour is undefined if this condition
 *   is not satisfied
 * Please note that the file shall be opened before calling
 * this function, otherwise a run-time error will occur.
 * Similarly, the function call does not close the output
 * file, which must be closed by the caller
 * ---------------------------------------------------------
 * Possible Return Values:
 * - FMRTOK
 *   Export was successful
 * - FMRTKO
 *   Result obtained when this is the first library
 *   call invoked by the caller or when the keyMin parameter
 *   is greater than keyMax
 * - FMRTIDNOTFOUND
 *   tableId is not defined
 ***********************************************************/
fmrtResult fmrtExportRangeCsv (fmrtId tableId, FILE *filePtr, char separator, uint8_t reverseFlag, ...)
{
    /* Local Variables */
    va_list     args;
    uint8_t     i,j,maxLen;
    fmrtResult   res;
    uint32_t    keyIntMin,
                keyIntMax;
    int32_t     keySignedMin,
                keySignedMax;
    double      keyDoubleMin,
                keyDoubleMax;
    time_t      keyTimestampMin,
                keyTimestampMax;
    char        keyCharMin,
                keyCharMax,
                keyStringMin[MAXFMRTSTRINGLEN+1],
                keyStringMax[MAXFMRTSTRINGLEN+1],
                *string;

    /* Call searchTable() internal function to look for the given tableId */
    /* In case of error exit and report error to the calling program      */
    /* To be completely safe, searchTable() shoud be called by locking    */
    /* global mutex (fmrtGlobalMtx), but it is very unlikely that a thread */
    /* tries to access a table which is still under definition, therefore */
    /* we prefer to avoid bottlenecks                                     */
    if ( ((res=searchTable(tableId,&i))!=FMRTOK) )
        return (res);

    /* Parse Min and Max key Value (depending on key type) */
    va_start (args, reverseFlag);
    switch (Tables[i].key.type)
    {
        case FMRTINT:
        {
            keyIntMin = va_arg (args, uint32_t);
            keyIntMax = va_arg (args, uint32_t);
            if (keyIntMin>keyIntMax)
            {
                va_end (args);
                return (FMRTKO);
            }
            break;
        }
        case FMRTSIGNED:
        {
            keySignedMin = va_arg (args, int32_t);
            keySignedMax = va_arg (args, int32_t);
            if (keySignedMin>keySignedMax)
            {
                va_end (args);
                return (FMRTKO);
            }
            break;
        }
        case FMRTDOUBLE:
        {
            keyDoubleMin = va_arg (args, double);
            keyDoubleMax = va_arg (args, double);
            if (keyDoubleMin>keyDoubleMax)
            {
                va_end (args);
                return (FMRTKO);
            }
            break;
        }
        case FMRTCHAR:
        {
            keyCharMin = (unsigned char) va_arg (args,int);
            keyCharMax = (unsigned char) va_arg (args,int);
            if (keyCharMin>keyCharMax)
            {
                va_end (args);
                return (FMRTKO);
            }
            break;
        }
        case FMRTSTRING:
        {   /* Read the key and truncate to the maximum length specified during definition */
            maxLen = Tables[i].key.len;     /* This field is max string length + trailing 0 */
            string = va_arg (args,char*);
            strncpy (keyStringMin,string,maxLen);
            keyStringMin[maxLen-1] = '\0';
            string = va_arg (args,char*);
            strncpy (keyStringMax,string,maxLen);
            keyStringMax[maxLen-1] = '\0';
            if (strcmp(keyStringMin,keyStringMax)>0)
            {
                va_end (args);
                return (FMRTKO);
            }
            break;
        }
        case FMRTTIMESTAMP:
        {
            if (fmrtTimeFormat[0]=='\0')
            {
                /* time format empty --> read raw timestamps from arguments */
                keyTimestampMin = va_arg (args, time_t);
                keyTimestampMax = va_arg (args, time_t);
            }
            else
            {   /* convert strings read from arguments to raw timestamps according to fmrtTimeFormat */
                struct tm   TimeFromString;
                string = va_arg (args,char*);
                if (strptime (string, fmrtTimeFormat, &TimeFromString) != NULL)
                    keyTimestampMin = mktime (&TimeFromString);
                else
                    keyTimestampMin = 0;
                string = va_arg (args,char*);
                if (strptime (string, fmrtTimeFormat, &TimeFromString) != NULL)
                    keyTimestampMax = mktime (&TimeFromString);
                else
                    keyTimestampMax = 0;
            }
            if (keyTimestampMin>keyTimestampMax)
            {
                va_end (args);
                return (FMRTKO);
            }
            break;
        }
    }   /* switch (Tables[i].key.type) */
    va_end (args);

    /* Set Table specific lock */
    pthread_mutex_lock(&(Tables[i].tableMtx));

    /* if file pointer is NULL, print output on stdout */
    if (filePtr==NULL)
        filePtr = stdout;

    fprintf (filePtr, "#Table: %s (Id: %d)\n",Tables[i].tableName, Tables[i].tableId);
    fprintf (filePtr, "#%s", Tables[i].key.name);
    for (j=0; j<Tables[i].numFields; j++)
        fprintf (filePtr, "%c%s", separator, Tables[i].fields[j].name);
    fprintf (filePtr,"\n");

    /* Start exporting recursively from the root node (depending on key type) */
    switch (Tables[i].key.type)
    {
        case FMRTINT:
        {
            exportRangeRecurseInt (i, Tables[i].fmrtRoot, filePtr, separator, reverseFlag, keyIntMin, keyIntMax);
            break;
        }
        case FMRTSIGNED:
        {
            exportRangeRecurseSigned (i, Tables[i].fmrtRoot, filePtr, separator, reverseFlag, keySignedMin, keySignedMax);
            break;
        }
        case FMRTDOUBLE:
        {
            exportRangeRecurseDouble (i, Tables[i].fmrtRoot, filePtr, separator, reverseFlag, keyDoubleMin, keyDoubleMax);
            break;
        }
        case FMRTCHAR:
        {
            exportRangeRecurseChar (i, Tables[i].fmrtRoot, filePtr, separator, reverseFlag, keyCharMin, keyCharMax);
            break;
        }
        case FMRTSTRING:
        {
            exportRangeRecurseString (i, Tables[i].fmrtRoot, filePtr, separator, reverseFlag, keyStringMin, keyStringMax);
            break;
        }
        case FMRTTIMESTAMP:
        {
            exportRangeRecurseTimestamp (i, Tables[i].fmrtRoot, filePtr, separator, reverseFlag, keyTimestampMin, keyTimestampMax);
            break;
        }
    }   /* switch (Tables[i].key.type) */


    /* Clear the lock before exiting */
    pthread_mutex_unlock(&(Tables[i].tableMtx));

    return (FMRTOK);
}


/***********************************************************
 * fmrtCountEntries()
 * ---------------------------------------------------------
 * This library call is used to count the number of elements
 * stored into the table. It takes just one input parameter:
 * - tableId
 *   unique identifier of the table between 0 and 255. The
 *   table shall be defined first through fmrtDefineTable()
 * ---------------------------------------------------------
 * It returns the number of elements contained into the
 * table or FMRTNULLPTR in case of any problem (e.g. tableId
 * not defined)
 ***********************************************************/
fmrtIndex fmrtCountEntries(fmrtId tableId)
{
    /* Local variables */
    uint8_t     i;
    fmrtIndex    num;
    fmrtResult   res;

    /* Call searchTable() internal function to look for the given tableId */
    /* In case of error exit and report error to the calling program      */
    /* To be completely safe, searchTable() shoud be called by locking    */
    /* global mutex (fmrtGlobalMtx), but it is very unlikely that a thread */
    /* tries to access a table which is still under definition, therefore */
    /* we prefer to avoid bottlenecks                                     */
    if ( ((res=searchTable(tableId,&i))!=FMRTOK) )
        return (FMRTNULLPTR);

    /* Set Table specific lock */
    pthread_mutex_lock(&(Tables[i].tableMtx));

    /* countSubtreeNodes() counts the number of elements recursively, it might require too many iterations in case of large tables */
    /* num = countSubtreeNodes (i, Tables[i].fmrtRoot); */
    num = Tables[i].currentNumElem;

    /* Clear the lock before exiting */
    pthread_mutex_unlock(&(Tables[i].tableMtx));

    return (num);
}


/***********************************************************
 * fmrtGetMemoryFootPrint()
 * ---------------------------------------------------------
 * This library call provides the memory footprint of the
 * table whose tableId is given as a parameter.
 * It takes just one input parameter:
 * - tableId
 *   unique identifier of the table between 0 and 255. The
 *   table shall be defined first through fmrtDefineTable()
 * ---------------------------------------------------------
 * It returns the number of bytes allocated for the given
 * table (0 in case of any problem, e.g. tableId not defined)
 ***********************************************************/
long fmrtGetMemoryFootPrint(fmrtId tableId)
{
    /* Local variables */
    uint8_t     i;
    long        bytes;
    fmrtResult   res;

    /* Call searchTable() internal function to look for the given tableId */
    /* In case of error exit and report error to the calling program      */
    /* To be completely safe, searchTable() shoud be called by locking    */
    /* global mutex (fmrtGlobalMtx), but it is very unlikely that a thread */
    /* tries to access a table which is still under definition, therefore */
    /* we prefer to avoid bottlenecks                                     */
    if ( ((res=searchTable(tableId,&i))!=FMRTOK) )
        return (0);

    bytes = sizeof(fmrtTableItem) + Tables[i].tableMaxElem*Tables[i].elemSize;

    return (bytes);
}


/***********************************************************
 * fmrtDefineTimeFormat()
 * ---------------------------------------------------------
 * This library call allows to change time format
 * representation of FMRTTIMEFORMAT data handled by the
 * library.
 * Please observe that the library manages internally
 * timestamps using always the linux representation (i.e.
 * seconds from Epoch, Jan 1st 1970, 00:00:00 UTC), however
 * it allows changing the format used to display and enter
 * FMRTTIMEFORMAT data. When a format is specified, then
 * timestamp keys and fields in fmrtRead(), frmrtCreate(),
 * etc. must be defined as character strings with the
 * correct format (e.g. "Mon Jun 21 17:08:55 2021").
 * The library allows also the usage of raw time format; in
 * that case, timestamp keys and fields in fmrtXxxx()
 * routines must be defined as time_t data.
 * The call takes just one input parameter:
 * - fmrtFormat
 *   This is a string representing the desired time format,
 *   according to the same convention specified by strftime()
 *   man page (e.g. "%c" for full date and time, "%T" for
 *   time in HH:MM:YY format, "%F" for date in the format
 *   YYYY-MM-DD, etc.).
 *   In case the parameter is either NULL or an empty
 *   string, then time format is set to raw (i.e. timestamps
 *   are represented as raw time_t data)
 * This routine is OPTIONAL. If not called, default time
 * format will be set to "%c", i.e. current locale.
 * The routine can be called as many times as desired. This
 * will not affect internal data representation, but only
 * data management and display (e.g. the format of timestamp
 * data in CSV output files).
 * Finally, observe that timestamp format is GLOBAL, i.e.
 * it is a configuration that affects all tables
 * ---------------------------------------------------------
 * Possible Return Values:
 * - FMRTOK
 *   Time format correctly defined and set
 * - FMRTKO
 *   Time format not accepted (e.g. due to wrong fmrtFormat
 *   parameter)
 ***********************************************************/
fmrtResult fmrtDefineTimeFormat(char * fmrtFormat)
{
    /* Local Variables */
    struct tm   TimeFromString;
    time_t      now;
    char        TimeDateString[MAXFMRTSTRINGLEN];

    if ( (fmrtFormat==NULL) || (fmrtFormat[0]=='\0') )
    {
        fmrtTimeFormat[0] = '\0';
        return (FMRTOK);
    }

    /* Try to retrieve current time and to format it according to specified format */
    /* This is done to understand if the specified format is correct               */
    time (&now);
    strftime(TimeDateString, MAXFMRTSTRINGLEN-1, fmrtFormat, localtime(&now));
    if (strptime (TimeDateString, fmrtFormat, &TimeFromString) == NULL)
        return (FMRTKO);

    strcpy (fmrtTimeFormat,fmrtFormat);
    return (FMRTOK);

 }


/***********************************************************
 * fmrtEncodeTimeStamp()
 * ---------------------------------------------------------
 * This library call takes an input string representing
 * a time stamp and provides back the corresponding value
 * encoded as a time_t (Linux time). The input string shall
 * be formatted according to the timestamp format defined
 * through fmrtDefineTimeFormat()
 * ---------------------------------------------------------
 * It provides the timestamp encoded as a time_t data or
 * 0 in case of errors
 ***********************************************************/
time_t fmrtEncodeTimeStamp(char * timeStamp)
{
    struct tm   TimeFromString;

    if (strptime (timeStamp, fmrtTimeFormat, &TimeFromString) != NULL)
        return ( mktime(&TimeFromString) );
    else
        return (0);

}


/***********************************************************
 * fmrtDecodeTimeStamp()
 * ---------------------------------------------------------
 * This library call takes a raw time stamp (i.e. a time_t
 * data) as first parameter and provides back in the second
 * parameter a string formatted according to the timestamp
 * format defined through fmrtDefineTimeFormat().
 * The string must be allocated before calling this function
 * ---------------------------------------------------------
 * It does not return anything
 ***********************************************************/
void fmrtDecodeTimeStamp(time_t rawTimeStamp, char * formattedTimeStamp)
{
    strftime(formattedTimeStamp, strlen(formattedTimeStamp), fmrtTimeFormat, localtime(&rawTimeStamp));
    return ;
}