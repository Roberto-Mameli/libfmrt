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
 * FILE:        CountWordsOccurrence.c                                         *
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
#define TABLEID         4   /* Arbitrary value between 0 and 255 */
#define LENGTH         32   /* Max length of a word              */
#define MAXWORDS   120000   /* Max number of words               */
#define MAXLINE       256   /* Max length of a single line       */


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
            printf ("The specified word already exists\n");
            break;
        }
        case FMRTNOTEMPTY:
        {
            printf ("Table is not empty\n");
            break;
        }
        case FMRTNOTFOUND:
        {
            printf ("Word has not been found\n");
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
    printf ("\t(1) - Count words from txt file\n");
    printf ("\t(2) - Search a word and print number of occurrences\n");
    printf ("\t(3) - Count distinct words\n");
    printf ("\t(4) - Export all words to file in ascending order\n");
    printf ("\t(5) - Export all words to file in optimized order\n");
    printf ("\t(6) - Export range of words in ascending order\n");
    printf ("\t(7) - Display Memory Footprint\n");
    printf ("\t(0) - Exit\n\n");

    printf ("\tEnter the selected choice: ");
    while ( (c = getchar())!= EOF )
    {
        if ( (c<'0') || (c>'7') )
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
                Key[LENGTH+1];
    time_t      start,end;
    FILE        *fptr;

    /* Invoke fmrtDefineTable() library call to define general table parameters */
    /* TableId arbitrar */
    if ( (res=fmrtDefineTable(TABLEID,"WordCount",MAXWORDS)) != FMRTOK)
    {
        printFmrtLibError (res);
        exit(0);
    }

    /* Now invoke fmrtDefineKey() to define key parameter */
    if ( (res=fmrtDefineKey(TABLEID,"Word",FMRTSTRING,LENGTH)) != FMRTOK)
    {
        printFmrtLibError (res);
        exit(0);
    }

    /* Now invoke fmrtDefineFields() to define as unique field the number of occurrences */
    if ( (res=fmrtDefineFields(TABLEID,1,"Frequency",FMRTINT)) != FMRTOK)
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
            {   /* Count words from txt file */
                int    frequency, lines=0;
                char   *p, *q, linestring[MAXLINE+1];

                system ("clear");
                printf ("I'm going to import and count words from an input txt file...\n");
                printf ("Please insert file name: ");
                scanf ("%s",filename);
                fptr = fopen (filename,"r");
                if (fptr==NULL)
                    printf ("Not able to read input txt file...\n");
                else
                {
                    /* Read the txt file line by line, isolate words */
                    /* and update number of occurrences in the table */
                    time (&start);  /* evaluate start time*/
                    while ( fgets(linestring,MAXLINE,fptr) )
                    {
                        lines++;
                        /* skip empty lines */
                        if (linestring[0] == '\0')
                            continue;
                        p = linestring;
                        while ( (q=strtok(p," .,:;!?()'\"\n\t<>[]{}+-^*$£%&")) )
                        {
                            p=NULL;
                            if ( fmrtRead(TABLEID,q,&frequency)==FMRTOK )
                                fmrtModify (TABLEID,1,q,++frequency);
                            else
                                fmrtCreate (TABLEID,q,1);
                        }   /* while ( (q=strtok(p,"... */
                    }   /* while ( fgets(linestring,MAXLINE,fptr) ) */
                    time (&end);    /* evaluate end time*/
                    printf ("Finished reading txt file... %d lines read in %d seconds\n\n",lines,(int)difftime(end,start));
                    fclose (fptr);
                }   /* else if (fptr==NULL) */
                waitEnterKey();
                break;
            }   /* case 1 */
            case 2:
            {   /* Search a word and print number of occurrences */
                int    frequency;

                system ("clear");
                printf ("Enter word to search:\n\tWord? ");
                scanf ("%s",Key);
                res = fmrtRead (TABLEID, Key, &frequency);
                printFmrtLibError (res);
                if (res==FMRTOK)
                    printf ("Word %s is present and occurs %d times\n",Key,frequency);
                waitEnterKey();
                break;
            }   /* case 2 */
            case 3:
            {   /* Count distinct words */
                system ("clear");
                num = fmrtCountEntries(TABLEID);
                printf ("The table contains %d distinct words\n",num);
                waitEnterKey();
                break;
            }   /* case 3 */
            case 4:
            {   /* Export all words to file in ascending order */
                system ("clear");
                printf ("I'm going to export words stored in memory onto an output file...\n");
                printf ("I will print them in ascending order...\n");
                printf ("Please insert file name (* for stdout): ");
                scanf ("%s",filename);
                if (strcmp(filename,"*")==0)
                    fptr = NULL;
                else
                    fptr = fopen (filename,"w");
                time (&start);  /* evaluate start time*/
                res=fmrtExportTableCsv(TABLEID,fptr,',',FMRTASCENDING);
                time (&end);    /* evaluate end time*/
                printf ("Finished exporting file... elapsed time %d seconds\n\n",(int)difftime(end,start));
                if (fptr!=NULL)
                    fclose (fptr);
                printFmrtLibError (res);
                waitEnterKey();
                break;
            }   /* case 4 */
            case 5:
            {   /* Export all words to file in optimized order */
                system ("clear");
                printf ("I'm going to export words stored in memory onto an output file...\n");
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
            }   /* case 5 */
            case 6:
            {   /* Export range of words in ascending order */
                char min[50], max[50];

                system ("clear");
                printf ("I'm going to export a range  of words stored in memory onto an output file...\n");
                printf ("I will print them in ascending order...\n");
                printf ("Please insert file name (* for stdout): ");
                scanf ("%s",filename);
                if (strcmp(filename,"*")==0)
                    fptr = NULL;
                else
                    fptr = fopen (filename,"w");
                printf ("Insert first word: ");
                scanf ("%s",min);
                printf ("Insert second word: ");
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
            }   /* case 6 */
            case 7:
            {   /* Display Memory Footprint */
                system ("clear");
                printf ("The whole table occupies %ld bytes in the internal memory ...\n",fmrtGetMemoryFootPrint(TABLEID));
                waitEnterKey();
                break;
            }   /* case 7 */
            default:
                break;
        }   /* switch (choice) */
    }   /* while ( (choice=PrintMenu()) != 0) */

    fmrtClearTable(TABLEID);
    exit(0);

}