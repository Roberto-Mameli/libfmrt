/*******************************************************************************
 * ---------------------------------------------------                         *
 * C/C++ Fast Memory Resident Tables Library (libfmrt)                         *
 * ---------------------------------------------------                         *
 * Copyright 2022 Roberto Mameli                                               *
 *                                                                             *
 * Licensed under the Apache License, Version 2.0 (the "License");             *
 * you may not use this file except in compliance with the License.            *
 * You may obtain a copy of the License at                                     *
 *                                                                             *
 *     http://www.apache.org/licenses/LICENSE-2.0                              *
 *                                                                             *
 * Unless required by applicable law or agreed to in writing, software         *
 * distributed under the License is distributed on an "AS IS" BASIS,           *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.    *
 * See the License for the specific language governing permissions and         *
 * limitations under the License.                                              *
 * --------------------------------------------------------------------------  *
 *                                                                             *
 * FILE:        BarCodeCache.c                                                 *
 * VERSION:     1.0.0                                                          *
 * AUTHOR(S):   Roberto Mameli                                                 *
 * PRODUCT:     Library libfmrt examples                                       *
 * DESCRIPTION: Example file that makes use of the libfmrt library             *
 *                                                                             *
 * -------------------------------------------------------------------------   *
 * DISCLAIMER                                                                  *
 * -------------------------------------------------------------------------   *
 * Example files are provided only as an example of development of a working   *
 * software program using libfmrt libraries. The source code provided is not   *
 * written as an example of a released, production level application, it is    *
 * intended only to demonstrate usage of the API functions used herein.        *
 * The Author provides the source code examples "AS IS" without warranty of    *
 * any kind, either expressed or implied, including, but not limited to the    *
 * implied warranties of merchantability and fitness for a particular purpose. *
 * The entire risk as to the quality and performance of the source code        *
 * examples is with you. Should any part of the source code examples prove     *
 * defective you (and not the Author) assume the entire cost of all necessary  *
 * servicing, repair or correction. In no event shall the Author be liable for *
 * damages of any kind, including direct, indirect, incidental, consequential, *
 * special, exemplary or punitive, even if it has been advised of the          *
 * possibility of such damage.                                                 *
 * The Author does not warrant that the contents of the source code examples,  *
 * whether will meet your requirements or that the source code examples are    *
 * error free.                                                                 *
 *******************************************************************************/


/**********************
 * Linux system files *
 **********************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/*********************
 * libfmrt header    *
 *********************/
#include "fmrt.h"


/***************
 * Definitions *
 ***************/
#define TABLEID                    1   /* Arbitrary value between 0 and 255 */
#define BARCODELEN                13   /* Barcode Fixed Length              */
#define SIZEFORMATLEN             24   /* Max Size of the Size/Format Field */
#define DESCRIPTIONLEN            48   /* Max Size of the Description Field */
#define MAXBARCODES          1300000   /* Max number of barcodes            */
#define TABLENAME         "BarCodes"   /* Table Name                        */
#define KEYNAME            "BarCode"   /* Key Name                          */
#define FIELD1NAME     "Size/Format"   /* Field no. 1 Name                  */
#define FIELD2NAME     "Description"   /* Field no. 1 Name                  */


/**********************
 * Internal functions *
 **********************/


/***************************
 * Used to print the error *
 ***************************/
void printFmrtLibError(fmrtResult result)
{
    switch (result)
    {
        case FMRTOK:
        {
            printf ("Operation Succeeded\n");
            break;
        }
        case FMRTKO:
        {
            printf ("Operation failed\n");
            break;
        }
        case FMRTIDALREADYEXISTS:
        {
            printf ("TableId already defined\n");
            break;
        }
        case FMRTIDNOTFOUND:
        {
            printf ("TableId not found\n");
            break;
        }
        case FMRTMAXTABLEREACHED:
        {
            printf ("Max number of tables reached\n");
            break;
        }
        case FMRTMAXFIELDSINVALID:
        {
            printf ("Specified number of fields is outside the allowed range\n");
            break;
        }
        case FMRTREDEFPROHIBITED:
        {
            printf ("Key and/or Field Redefinition Prohibited\n");
            break;
        }
        case FMRTDUPLICATEKEY:
        {
            printf ("The specified item already exists\n");
            break;
        }
        case FMRTNOTEMPTY:
        {
            printf ("Table is not empty\n");
            break;
        }
        case FMRTNOTFOUND:
        {
            printf ("Item has not been found\n");
            break;
        }
        case FMRTFIELDTOOLONG:
        {
            printf ("String too long\n");
            break;
        }
        case FMRTOUTOFMEMORY:
        {
            printf ("No more space in table\n");
            break;
        }
        default:
        {
            printf ("Unrecognized error\n");
            break;
        }
    }   /* switch (result) */
}

/**************************************************************************
 * Print available options on the screen and get the choice from the user *
 **************************************************************************/
int printMenu (void)
{
    int     c;

    /* Clear the screen and print the menu of available choices */
    system ("clear");
    printf ("*********************\n");
    printf ("* Available choices *\n");
    printf ("*********************\n\n");
    printf ("\tMenu\n\t----\n\n");
    printf ("\t(1) - Import barcodes from CSV input file\n");
    printf ("\t(2) - Search and Print Barcode\n");
    printf ("\t(3) - Insert a new Barcode in the table\n");
    printf ("\t(4) - Remove Barcode from the table\n");
    printf ("\t(5) - Count Barcodes\n");
    printf ("\t(6) - Export all Barcodes to file in optimized order\n");
    printf ("\t(7) - Export range of Barcodes in ascending order\n");
    printf ("\t(8) - Display Memory Footprint\n");
    printf ("\t(0) - Exit\n\n");

    printf ("\tEnter the selected choice: ");
    while ( (c = getchar())!= EOF )
    {
        if ( (c<'0') || (c>'8') )
            continue;
        break;
    }
    return (c-(int)'0');

}


/***************************************
 * Wait until the ENTER key is pressed *
 ***************************************/
void waitEnterKey (void)
{
    printf ("\n\tPress the ENTER key to continue...\n");
    system ("read");
    return;
}


/*****************
 * Main Function *
 *****************/
int main(int argc, char *argv[], char *envp[])
{
    /* Local variables */
    int         choice,num;
    fmrtResult  res;
    char        filename[128],
                Key[128],
                Field1[SIZEFORMATLEN],
                Field2[DESCRIPTIONLEN];
    time_t      start,end;
    FILE        *fptr;

    /* Invoke fmrtDefineTable() library call to define general table parameters */
    /* TableId arbitrary assigned */
    if ( (res=fmrtDefineTable(TABLEID,TABLENAME,MAXBARCODES)) != FMRTOK)
    {
        printFmrtLibError (res);
        exit(0);
    }

    /* Now invoke fmrtDefineKey() to define key parameter */
    if ( (res=fmrtDefineKey(TABLEID,KEYNAME,FMRTSTRING,BARCODELEN)) != FMRTOK)
    {
        printFmrtLibError (res);
        exit(0);
    }

    /* Now invoke fmrtDefineFields() to define the two attributes in the table */
    if ( (res=fmrtDefineFields(TABLEID,2,FIELD1NAME,FMRTSTRING,SIZEFORMATLEN,FIELD2NAME,FMRTSTRING,DESCRIPTIONLEN)) != FMRTOK)
    {
        printFmrtLibError (res);
        exit(0);
    }

    /* Menu */
    while ( (choice=printMenu()) != 0)
    {
        switch (choice)
        {
            case 1:
            {   /* Import barcodes from CSV input file */
                system ("clear");
                printf ("I'm going to import barcodes from an input file...\n");
                printf ("Please insert file name: ");
                scanf ("%s",filename);
                fptr = fopen (filename,"r");
                time (&start);  /* evaluate start time */
                res=fmrtImportTableCsv(TABLEID,fptr,',',&num);
                time (&end);    /* evaluate end time*/
                printf ("Finished reading input CSV file... %d lines read in %d seconds\n\n",num,(int)difftime(end,start));
                if (fptr!=NULL)
                    fclose (fptr);
                printFmrtLibError (res);
                printf ("Read %d lines from input file\n",num);
                waitEnterKey();
                break;
            }   /* case 1 */
            case 2:
            {   /* Search and Print Barcode */
                system ("clear");
                printf ("13-char barcode to search? ");
                scanf ("%s",Key);
                res = fmrtRead (TABLEID, Key, Field1, Field2);
                printFmrtLibError (res);
                if (res==FMRTOK)
                    printf ("Barcode: %s -> Size/Format: %s | Description: %s\n",Key,Field1,Field2);
                waitEnterKey();
                break;
            }   /* case 2 */
            case 3:
            {   /* Insert a new Barcode in the table */
                system ("clear");
                printf ("Insert new item:\n\tBarCode? ");
                scanf ("%s",Key);
                printf ("Size/Format? ");
                scanf ("%s",Field1);
                printf ("Description? ");
                scanf ("%s",Field2);
                res = fmrtCreate (TABLEID, Key,Field1,Field2);
                printFmrtLibError (res);
                waitEnterKey();
                break;
            }   /* case 3 */
            case 4:
            {   /* Remove Barcode from the table */
                system ("clear");
                printf ("Delete item:\n\tBarcode? ");
                scanf ("%s",Key);
                res = fmrtDelete (TABLEID, Key);
                printFmrtLibError (res);
                waitEnterKey();
                break;
            }   /* case 4 */
            case 5:
            {   /* Count Barcodes */
                system ("clear");
                num = fmrtCountEntries(TABLEID);
                printf ("The table contains %d items\n",num);
                waitEnterKey();
                break;
            }   /* case 5 */
            case 6:
            {   /* Export all Barcodes to file in optimized order */
                system ("clear");
                printf ("I'm going to export items stored in memory onto an output file...\n");
                printf ("I will print them in optimized order...\n");
                printf ("Please insert file name (* for stdout): ");
                scanf ("%s",filename);
                if (strcmp(filename,"*")==0)
                    fptr = NULL;
                else
                    fptr = fopen (filename,"w");
                time (&start);  /* evaluate start time */
                res=fmrtExportTableCsv(TABLEID,fptr,',',FMRTOPTIMIZED);
                time (&end);    /* evaluate end time*/
                printf ("Finished exporting file... elapsed time %d seconds\n\n",(int)difftime(end,start));
                if (fptr!=NULL)
                    fclose (fptr);
                printFmrtLibError (res);
                waitEnterKey();
                break;
            }   /* case 6 */
            case 7:
            {   /* Export range of words in ascending order */
                char min[50], max[50];

                system ("clear");
                printf ("I'm going to export a range  of Barcodes stored in memory onto an output file...\n");
                printf ("I will print them in ascending order...\n");
                printf ("Please insert file name (* for stdout): ");
                scanf ("%s",filename);
                if (strcmp(filename,"*")==0)
                    fptr = NULL;
                else
                    fptr = fopen (filename,"w");
                printf ("Insert first barcode: ");
                scanf ("%s",min);
                printf ("Insert second barcode: ");
                scanf ("%s",max);
                time (&start);  /* evaluate start time */
                res=fmrtExportRangeCsv(TABLEID,fptr,',',FMRTASCENDING,min,max);
                time (&end);    /* evaluate end time*/
                printf ("Finished exporting file... elapsed time %d seconds\n\n",(int)difftime(end,start));
                if (fptr!=NULL)
                    fclose (fptr);
                printFmrtLibError (res);
                waitEnterKey();
                break;
            }   /* case 7 */
            case 8:
            {   /* Display Memory Footprint */
                system ("clear");
                printf ("The whole table occupies %ld bytes in the internal memory ...\n",fmrtGetMemoryFootPrint(TABLEID));
                waitEnterKey();
                break;
            }   /* case 8 */
            default:
                break;
        }   /* switch (choice) */
    }   /* while ( (choice=PrintMenu()) != 0) */

    fmrtClearTable(TABLEID);
    exit(0);

}