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
 * FILE:        Televoting.c                                         *
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
#include <pthread.h>


/*********************
 * libfmrt header    *
 *********************/
#include "fmrt.h"


/***************
 * Definitions *
 ***************/
#define VOTESTABLEID           4   /* Arbitrary value between 0 and 255               */
#define EVENTSTABLEID         12   /* Arbitrary value between 0 and 255               */
#define PHONENOLEN            15   /* Max length in DB of a phone number              */
#define MAXPHONENUMBERS  1000000   /* Size of Votes Table (observe that we have one   *
                                      million numbers, ie. trail digits 000000-999999)*/
#define EVENTSNUMBERS     500000   /* Size of Events Table                            */
#define EVENTLENSTR           48   /* String Lentgth for Events                       */
#define MAXPREF               20   /* Max value for random preferences                */
#define MAXNUMTHREADS          4   /* Max number of cuncurrent threads                */
#define MAXVOTESPERTHREAD 300000   /* Max number of preferences generated per thread  */


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
            printf ("The specified Number already exists\n");
            break;
        }
        case FMRTNOTEMPTY:
        {
            printf ("Table is not empty\n");
            break;
        }
        case FMRTNOTFOUND:
        {
            printf ("Number has not been found\n");
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

/***************************************
 * Wait until the ENTER key is pressed *
 ***************************************/
void waitEnterKey (void)
{
    printf ("\n\tPress the ENTER key to continue...\n");
    system ("read");
    return;
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
    printf ("\t(1) - Display Number of Elements, Size and Memory Footprint of Tables\n");
    printf ("\t(2) - Search and Print Vote Expressed by Input Phone Number\n");
    printf ("\t(3) - Export Preferences to File\n");
    printf ("\t(4) - Export Events to File\n");
    printf ("\t(0) - Exit\n\n");

    printf ("\tEnter the selected choice: ");
    while ( (c = getchar())!= EOF )
    {
        if ( (c<'0') || (c>'4') )
            continue;
        break;
    }
    return (c-(int)'0');

}

/**************************************************************
 * This function generates a random phone Number in the form  *
 * +39301xx..xx and a random preference between 1 and MAXPREF *
 * The number of digits xx..xx is defined by DIGITS constant  *
 **************************************************************/
void generateRandomVote(int *pref, char *phoneNo)
{
    int   trail;

    trail = rand()%MAXPHONENUMBERS;
    sprintf (phoneNo,"+39301%06d",trail);

    *pref = 1+rand()%MAXPREF;

    return;
}

/****************************************************************
 * This function simulates votesPerThread votes and stores them *
 * into the Votes Table. In case of errors, logs the event into *
 * the events table along with the associated timestamp         *
 ****************************************************************/
void *emulateVotingThread(void *argument)
{
    int        i, pref,
               votesPerThread;
    char       phoneNo[PHONENOLEN+1],
               event[EVENTLENSTR];
    fmrtResult res;
    time_t     currentTime;

    votesPerThread = *((int *)argument);
    printf ("Thread (id %x) started (simulating %d votes)...\n",(int) pthread_self(),votesPerThread);
    for (i=0; i<votesPerThread; i++)
    {
        generateRandomVote (&pref, phoneNo);
        res=fmrtCreate(VOTESTABLEID,phoneNo,pref);
        switch (res)
        {
            case FMRTOK:
            {
                break;
            }
            case FMRTDUPLICATEKEY:
            {
                sprintf (event,"%s attempted to vote again",phoneNo);
                time (&currentTime);
                if (fmrtCreate(EVENTSTABLEID,currentTime,event)!=FMRTOK)
                    fmrtCreateModify(EVENTSTABLEID,1,currentTime,"Multiple Occurrences of repeated votes");
                break;
            }
            case FMRTOUTOFMEMORY:
            {
                sprintf (event,"No more space left in Votes Table");
                time (&currentTime);
                if (fmrtCreate(EVENTSTABLEID,currentTime,event)!=FMRTOK)
                    fmrtCreateModify(EVENTSTABLEID,1,currentTime,"Multiple Occurrences of Out of Memory");
                break;
            }
            default:
            {
                sprintf (event,"Unexpected error inserting entry in Votes Table");
                time (&currentTime);
                if (fmrtCreate(EVENTSTABLEID,currentTime,event)!=FMRTOK)
                    fmrtCreateModify(EVENTSTABLEID,1,currentTime,"Multiple Unexpected Error Occurrences");
                break;
            }
        }   /* switch (res) */
    }   /* for (i=0; i<votesPerThread; i++) */

    printf ("Thread (id %x) completed\n",(int) pthread_self());

    return (NULL);

}


/*****************
 * Main Function *
 *****************/
int main(int argc, char *argv[], char *envp[])
{
    /* Local variables */
    int         num, threadNo, VoteNo, choice, pref;
    fmrtResult  res;
    time_t      start, end, currentTime;
    char        Key[PHONENOLEN+1],
                filename[128];
    pthread_t   tid[MAXNUMTHREADS];
    FILE        *fptr;

    /* Invoke fmrtDefineTable() library call to define general table parameters */
    if ( (res=fmrtDefineTable(VOTESTABLEID,"Votes",MAXPHONENUMBERS)) != FMRTOK)
    {
        printFmrtLibError (res);
        exit(0);
    }
    if ( (res=fmrtDefineTable(EVENTSTABLEID,"LoggedEvents",EVENTSNUMBERS)) != FMRTOK)
    {
        printFmrtLibError (res);
        exit(0);
    }

    /* Now invoke fmrtDefineKey() to define key parameters */
    if ( (res=fmrtDefineKey(VOTESTABLEID,"PhoneNo",FMRTSTRING,PHONENOLEN)) != FMRTOK)
    {
        printFmrtLibError (res);
        exit(0);
    }
    if ( (res=fmrtDefineKey(EVENTSTABLEID,"TimeStamp",FMRTTIMESTAMP,0)) != FMRTOK)
    {
        printFmrtLibError (res);
        exit(0);
    }

    /* Now invoke fmrtDefineFields() to define attributes in the tables */
    if ( (res=fmrtDefineFields(VOTESTABLEID,1,"Preference",FMRTINT)) != FMRTOK)
    {
        printFmrtLibError (res);
        exit(0);
    }
    if ( (res=fmrtDefineFields(EVENTSTABLEID,1,"Event",FMRTSTRING,EVENTLENSTR)) != FMRTOK)
    {
        printFmrtLibError (res);
        exit(0);
    }

    /* Use raw time format to collect events */
    if( (res=fmrtDefineTimeFormat("")) != FMRTOK)
    {
        printFmrtLibError (res);
        exit(0);
    }

    /* Initialize Random Seed Generator */
    time (&currentTime);
    srand ( (unsigned int) currentTime);

    /* Main */
    system("clear");
    printf ("This program launches several threads, each one simulating the reception\n");
    printf ("of a number of televotes from random generated telephone numbers.\n");
    printf ("Data are collected into a table (Votes), while events (e.g. duplicated\n");
    printf ("votes) are stored into another table (LoggedEvents).\n\n");
    do
    {
        printf ("Enter the number of threads (1-%d)? ",MAXNUMTHREADS);
        scanf ("%d",&threadNo);
    } while ( (threadNo<1) || (threadNo>MAXNUMTHREADS) );
    do
    {
        printf ("Enter the number of votes per thread (1-%d)? ",MAXVOTESPERTHREAD);
        scanf ("%d",&VoteNo);
    } while ( (VoteNo<1) || (VoteNo>MAXVOTESPERTHREAD) );
    waitEnterKey();

    system("clear");
    printf ("Televoting in progress...\n\n");
    time (&start);
    for (num=0; num <threadNo; num++)
        pthread_create(&tid[num], NULL, &emulateVotingThread, (void*)(&VoteNo));
    for (num=0; num <threadNo; num++)
        pthread_join (tid[num],NULL);
    time (&end);
    printf ("\nTelevoting operations ended... elapsed time %d seconds\n\n",(int)difftime(end,start));
    waitEnterKey();

    /* Use local time format to print events into output */
    if( (res=fmrtDefineTimeFormat("%c")) != FMRTOK)
    {
        printFmrtLibError (res);
        exit(0);
    }

    while ( (choice=printMenu()) != 0)
    {
        switch (choice)
        {
            case 1:
            {   /* Display Number of Elements, Size and Memory Footprint of Tables */
                system("clear");
                printf ("Table Votes:\n");
                printf ("\tTable Id:         %d\n",VOTESTABLEID);
                printf ("\tTable Size:       %d\n",MAXPHONENUMBERS);
                printf ("\tNumber of Votes:  %d\n",fmrtCountEntries(VOTESTABLEID));
                printf ("\tMemory Size (KB): %.2f\n\n",fmrtGetMemoryFootPrint(VOTESTABLEID)/1024.0);
                printf ("Table LoggedEvents:\n");
                printf ("\tTable Id:         %d\n",EVENTSTABLEID);
                printf ("\tTable Size:       %d\n",EVENTSNUMBERS);
                printf ("\tNumber of Events: %d\n",fmrtCountEntries(EVENTSTABLEID));
                printf ("\tMemory Size (KB): %.2f\n\n",fmrtGetMemoryFootPrint(EVENTSTABLEID)/1024.0);
                waitEnterKey();
                break;
            }   /* case 1 */
            case 2:
            {   /* Search and Print Vote Expressed by Input Phone Number */
                system ("clear");
                printf ("Enter phone number to search (e.g. +39301123456)? ");
                scanf ("%s",Key);
                res = fmrtRead (VOTESTABLEID, Key, &pref);
                printFmrtLibError (res);
                if (res==FMRTOK)
                    printf ("Phone Number %s expressed the following vote: %d\n",Key,pref);
                waitEnterKey();
                break;
            }   /* case 2 */
            case 3:
            {   /* Export Preferences to File */
                system ("clear");
                printf ("I'm going to export all preferences stored in memory onto an output file...\n");
                printf ("Please insert file name (* for stdout): ");
                scanf ("%s",filename);
                if (strcmp(filename,"*")==0)
                    fptr = NULL;
                else
                    fptr = fopen (filename,"w");
                time (&start);  /* evaluate start time */
                res=fmrtExportTableCsv(VOTESTABLEID,fptr,',',FMRTASCENDING);
                time (&end);    /* evaluate end time*/
                printf ("Finished exporting file... elapsed time %d seconds\n\n",(int)difftime(end,start));
                if (fptr!=NULL)
                    fclose (fptr);
                printFmrtLibError (res);
                waitEnterKey();
                break;
            }   /* case 3 */
            case 4:
            {   /* Export Events to File */
                system ("clear");
                printf ("I'm going to export all events stored in memory onto an output file...\n");
                printf ("Please insert file name (* for stdout): ");
                scanf ("%s",filename);
                if (strcmp(filename,"*")==0)
                    fptr = NULL;
                else
                    fptr = fopen (filename,"w");
                time (&start);  /* evaluate start time */
                res=fmrtExportTableCsv(EVENTSTABLEID,fptr,',',FMRTASCENDING);
                time (&end);    /* evaluate end time*/
                printf ("Finished exporting file... elapsed time %d seconds\n\n",(int)difftime(end,start));
                if (fptr!=NULL)
                    fclose (fptr);
                printFmrtLibError (res);
                waitEnterKey();
                break;
            }   /* case 4 */
        }   /* switch (choice) */
    }   /* while ( (choice=PrintMenu()) != 0) */

    /* Finished - Clear all and exit */
    fmrtClearTable(VOTESTABLEID);
    fmrtClearTable(EVENTSTABLEID);
    exit(0);
}

