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
 * FILE:        fmrt.h                                                       *
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

#ifndef FMRT_H_
#define FMRT_H_


/*****************
 * Include Files *
 *****************/
#include <stdint.h>
#include <stdarg.h>


/*******************************
 * General Purpose Definitions *
 *******************************/
/* FMRT types */
#define FMRTINT              0    /* FMRT Unsigned Integer Type (32 bits)  */
#define FMRTSIGNED           1    /* FMRT Signed Integer Type (32 bits)    */
#define FMRTDOUBLE           2    /* FMRT floating double precision type   */
#define FMRTCHAR             3    /* FMRT Char Type (8 bits)               */
#define FMRTSTRING           4    /* FMRT String Type (max len 64 chars)   */
#define FMRTTIMESTAMP        5    /* FMRT Time Stamp (i.e. time_t type)    */


/*********************
 * Error Definitions *
 *********************/
#define FMRTOK               0    /* Result OK                            */
#define FMRTKO               1    /* Something unspecified went wrong     */
#define FMRTIDALREADYEXISTS  2    /* Id in the request is already in use  */
#define FMRTIDNOTFOUND       3    /* Id in the request does not exist     */
#define FMRTMAXTABLEREACHED  4    /* No more tables left to define        */
#define FMRTMAXFIELDSINVALID 5    /* Fields num outside of allowed range  */
#define FMRTDUPLICATEKEY     6    /* Key already exists in Create oper.   */
#define FMRTNOTEMPTY         7    /* The Table contains at least one elem */
#define FMRTNOTFOUND         8    /* Searched Element has not been found  */
#define FMRTFIELDTOOLONG     9    /* String field exceeds max length (64) */
#define FMRTOUTOFMEMORY     10    /* No More space left for new elements  */


/********************
 * Type Definitions *
 ********************/
typedef uint8_t     fmrtId,
                    fmrtType,
                    fmrtLen,
                    fmrtResult;

typedef uint32_t    fmrtIndex;

typedef uint16_t    fmrtParamMask;

/***********************
 * Function Prototypes *
 ***********************/
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
fmrtResult fmrtDefineTable (fmrtId, char*, fmrtIndex);


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
fmrtResult fmrtClearTable (fmrtId);


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
fmrtResult fmrtDefineKey (fmrtId, char*, fmrtType, fmrtLen);


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
fmrtResult fmrtDefineFields (fmrtId, uint8_t, ...);


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
fmrtResult fmrtRead (fmrtId, ...);


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
fmrtResult fmrtCreate (fmrtId, ...);


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
fmrtResult fmrtModify (fmrtId, fmrtParamMask, ...);


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
fmrtResult fmrtCreateModify (fmrtId, fmrtParamMask, ...);


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
fmrtResult fmrtDelete (fmrtId, ...);


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
fmrtResult fmrtImportTableCsv (fmrtId, FILE *, char, int *);


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
fmrtResult fmrtExportTableCsv (fmrtId, FILE *, char, uint8_t);


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
fmrtResult fmrtExportRangeCsv (fmrtId, FILE *, char , uint8_t , ...);


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
fmrtIndex fmrtCountEntries(fmrtId);


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
long fmrtGetMemoryFootPrint(fmrtId);


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
fmrtResult fmrtDefineTimeFormat(char * );


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
time_t fmrtEncodeTimeStamp(char * );


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
void fmrtDecodeTimeStamp(time_t , char * );

#endif /* FMRT_H_ */