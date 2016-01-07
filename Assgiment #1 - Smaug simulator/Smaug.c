#include <errno.h> 
#include <wait.h> 
#include <stdlib.h> 
#include <stdio.h>
#include <unistd.h>
//#include <curses.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/resource.h> 

/* Define semaphores to be placed in a single semaphore set */
/* Numbers indicate index in semaphore set for named semaphore */
#define SEM_COWSINGROUP 0
#define SEM_PCOWSINGROUP 1
#define SEM_COWSWAITING 2
#define SEM_PCOWSEATEN 3
#define SEM_COWSEATEN 4
#define SEM_COWSDEAD 5


#define SEM_DRAGONEATING 6
#define SEM_DRAGONFIGHTING 7
#define SEM_DRAGONSLEEPING 8

#define SEM_PMEALWAITINGFLAG 9

#define SEM_PHUNTERCOUNT 10
#define SEM_HUNTERSWAITING 11
#define SEM_HUNTERFINISH 12

#define SEM_PTHIEFCOUNT 13
#define SEM_THIEVESWAITING 14
#define SEM_THIEFFINISH 15


#define SEM_PTERMINATE 16

#define SEM_SHEEPSINGROUP 17
#define SEM_PSHEEPSINGROUP 18
#define SEM_SHEEPSWAITING 19
#define SEM_PSHEEPSEATEN 20
#define SEM_SHEEPSEATEN 21
#define SEM_SHEEPSDEAD 22

#define SEM_PSHEEPMEALWAITINGFLAG 23

/* System constants used to control simulation termination */
#define MAX_COWS_EATEN 24
#define MAX_SHEEPS_EATEN 36
#define MAX_THEIFES_WON 48
#define MAX_HUNTERS_WON 36
#define MAX_TREASURE_IN_HOARD 1000
#define MIN_TREASURE_IN_HOARD 0
#define INITIAL_TREASURE_IN_HOARD 500
#define SMAUG_NAP_LENGTH 1000000  // equal to 1 sec

/* System constants to specify size of groups of cows*/
#define COWS_IN_GROUP 2
#define SHEEPS_IN_GROUP 3

/* CREATING YOUR SEMAPHORES */
int semID; 

union semun
{
	int val;
	struct semid_ds *buf;
	ushort *array;
} seminfo;

struct timeval startTime;

/*  Pointers and ids for shared memory segments */
int *terminateFlagp = NULL;
int *cowCounterp = NULL;
int *cowsEatenCounterp = NULL;
int *mealWaitingFlagp = NULL;
int *mealSheepWaitingFlagp = NULL;
int *sheepCounterp = NULL;
int *sheepsEatenCounterp = NULL;

int terminateFlag = 0;

int cowCounter = 0;
int cowsEatenCounter = 0;

int sheepCounter = 0;
int sheepsEatenCounter = 0;

int mealWaitingFlag = 0;
int mealSheepWaitingFlag = 0;

/* Group IDs for managing/removing processes */
int smaugProcessID = -1;
int cowProcessGID = -1;
int sheepProcessGID = -1;
int parentProcessGID = -1;
int parentProcessID = -1;

// thief stuff
int *thiefCounterp = NULL;
int thiefCounter = 0;
int thiefProcessGID = -1;

// hunter stuff
int *hunterCounterp = NULL;
int hunterCounter = 0;
int hunterProcessGID = -1;

/* Define the semaphore operations for each semaphore */
/* Arguments of each definition are: */
/* Name of semaphore on which the operation is done */
/* Increment (amount added to the semaphore when operation executes*/
/* Flag values (block when semaphore <0, enable undo ...)*/

/*Number in group semaphores*/
struct sembuf WaitCowsInGroup={SEM_COWSINGROUP, -1, 0};
struct sembuf SignalCowsInGroup={SEM_COWSINGROUP, 1, 0};

struct sembuf WaitSheepsInGroup={SEM_SHEEPSINGROUP, -1, 0};
struct sembuf SignalSheepsInGroup={SEM_SHEEPSINGROUP, 1, 0};

/*Number in group mutexes*/
struct sembuf WaitProtectCowsInGroup={SEM_PCOWSINGROUP, -1, 0};
struct sembuf SignalProtectCowsInGroup={SEM_PCOWSINGROUP, 1, 0};

struct sembuf WaitProtectMealWaitingFlag={SEM_PMEALWAITINGFLAG, -1, 0};
struct sembuf SignalProtectMealWaitingFlag={SEM_PMEALWAITINGFLAG, 1, 0};

struct sembuf WaitProtectSheepMealWaitingFlag={SEM_PSHEEPMEALWAITINGFLAG, -1, 0};
struct sembuf SignalProtectSheepMealWaitingFlag={SEM_PSHEEPMEALWAITINGFLAG, 1, 0};

struct sembuf WaitProtectSheepsInGroup={SEM_PSHEEPSINGROUP, -1, 0};
struct sembuf SignalProtectSheepsInGroup={SEM_PSHEEPSINGROUP, 1, 0};

struct sembuf WaitProtectHunterCount={SEM_PHUNTERCOUNT, -1, 0};
struct sembuf SignalProtectHunterCount={SEM_PHUNTERCOUNT, 1, 0};

struct sembuf WaitProtectThiefCount={SEM_PTHIEFCOUNT, -1, 0};
struct sembuf SignalProtectThiefCount={SEM_PTHIEFCOUNT, 1, 0};

/*Number waiting sempahores*/
struct sembuf WaitCowsWaiting={SEM_COWSWAITING, -1, 0};
struct sembuf SignalCowsWaiting={SEM_COWSWAITING, 1, 0};

struct sembuf WaitSheepsWaiting={SEM_SHEEPSWAITING, -1, 0};
struct sembuf SignalSheepsWaiting={SEM_SHEEPSWAITING, 1, 0};

struct sembuf WaitThievesWaiting={SEM_THIEVESWAITING, -1, 0};
struct sembuf SignalThievesWaiting={SEM_THIEVESWAITING, 1, 0};

struct sembuf WaitHuntersWaiting={SEM_HUNTERSWAITING, -1, 0};
struct sembuf SignalHuntersWaiting={SEM_HUNTERSWAITING, 1, 0};

/*Number eaten or fought semaphores*/
struct sembuf WaitCowsEaten={SEM_COWSEATEN, -1, 0};
struct sembuf SignalCowsEaten={SEM_COWSEATEN, 1, 0};
struct sembuf WaitSheepsEaten={SEM_SHEEPSEATEN, -1, 0};
struct sembuf SignalSheepsEaten={SEM_SHEEPSEATEN, 1, 0};

struct sembuf WaitThiefFinish={SEM_THIEFFINISH, -1, 0};
struct sembuf SignalThiefFinish={SEM_THIEFFINISH, 1, 0};

struct sembuf WaitHunterFinish={SEM_HUNTERFINISH, -1, 0};
struct sembuf SignalHunterFinish={SEM_HUNTERFINISH, 1, 0};

/*Number eaten or fought mutexes*/
struct sembuf WaitProtectCowsEaten={SEM_PCOWSEATEN, -1, 0};
struct sembuf SignalProtectCowsEaten={SEM_PCOWSEATEN, 1, 0};
struct sembuf WaitProtectSheepsEaten={SEM_PSHEEPSEATEN, -1, 0};
struct sembuf SignalProtectSheepsEaten={SEM_PSHEEPSEATEN, 1, 0};

/*Number Dead semaphores*/
struct sembuf WaitCowsDead={SEM_COWSDEAD, -1, 0};
struct sembuf SignalCowsDead={SEM_COWSDEAD, 1, 0};
struct sembuf WaitSheepsDead={SEM_SHEEPSDEAD, -1, 0};
struct sembuf SignalSheepsDead={SEM_SHEEPSDEAD, 1, 0};

/*Dragon Semaphores*/
struct sembuf WaitDragonEating={SEM_DRAGONEATING, -1, 0};
struct sembuf SignalDragonEating={SEM_DRAGONEATING, 1, 0};
struct sembuf WaitDragonFighting={SEM_DRAGONFIGHTING, -1, 0};
struct sembuf SignalDragonFighting={SEM_DRAGONFIGHTING, 1, 0};
struct sembuf WaitDragonSleeping={SEM_DRAGONSLEEPING, -1, 0};
struct sembuf SignalDragonSleeping={SEM_DRAGONSLEEPING, 1, 0};

/*Termination Mutex*/
struct sembuf WaitProtectTerminate={SEM_PTERMINATE, -1, 0};
struct sembuf SignalProtectTerminate={SEM_PTERMINATE, 1, 0};

// FUNCTION DECLARATION
double timeChange( struct timeval starttime );
void initialize();
void smaug(int smaugWinChance);
void cow(int startTimeN);
void thief(int startTimeN);
void hunter(int startTimeN);
void sheep(int startTimeN);
void terminateSimulation();
void releaseSemandMem();
void semopChecked(int semaphoreID, struct sembuf *operation, unsigned something); 
void semctlChecked(int semaphoreID, int semNum, int flag, union semun seminfo); 

void smaug(int smaugWinChance)//
{
 
 int k;
 int localpid;
 double elapsedTime;
 
 /* local counters used only for smaug routine */
 int numJewels = INITIAL_TREASURE_IN_HOARD;

 int cowsEatenTotal = 0;
 int sheepsEatenTotal = 0;
 int thiefsWonTotal = 0;
 int huntersWonTotal = 0;


 int sleepThisIteration = 1;
 
 	/* Initialize random number generator*/
	/* Random numbers are used to determine the time between successive beasts */
	smaugProcessID = getpid();
	printf("SMAUGSMAUGSMAUGSMAUGSMAU   PID is %d \n", smaugProcessID );
	localpid = smaugProcessID;
	while (*terminateFlagp==0) 
	{			 
 	  // Smaug goes to sleep if nothing happens and sleepThisIteration is 1
 	  
 	  
 	  
 	 if(sleepThisIteration == 1) 
 	 {
		 
 	 	 printf("SMAUGSMAUGSMAUGSMAUGSMAU Smaug has gone to sleep\n" );
 	 	 // We must reset the semaphore to prevent smaug waking up when there's no need to
 	 	 seminfo.val = 0;
 	 	 semctlChecked(semID, SEM_DRAGONSLEEPING, SETVAL, seminfo);
 	 	 semopChecked(semID, &WaitDragonSleeping, 1);
 	 	 printf("SMAUGSMAUGSMAUGSMAUGSMAU Smaug sniffs his valey\n" );
 	 	 printf("SMAUGSMAUGSMAUGSMAUGSMAU Smaug has woken up \n" );
 	 } 
 	 else 
 	 {
 	 	 sleepThisIteration = 1;
 	 }	 
	 semopChecked(semID, &WaitProtectMealWaitingFlag, 1);
	 semopChecked(semID, &WaitProtectSheepMealWaitingFlag, 1);
	 
	 if(*mealWaitingFlagp >= 1 && *mealSheepWaitingFlagp >= 1)
	 {
		while( *mealWaitingFlagp >= 1 && *mealSheepWaitingFlagp >= 1  ) 
		{
			*mealWaitingFlagp = *mealWaitingFlagp - 1;
			*mealSheepWaitingFlagp = *mealSheepWaitingFlagp - 1;
			
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   signal cow meal flag %d\n", *mealWaitingFlagp);
			semopChecked(semID, &SignalProtectMealWaitingFlag, 1);
			
			
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug is eating a meal\n");
			for( k = 0; k < COWS_IN_GROUP; k++ ) {
				semopChecked(semID, &SignalCowsWaiting, 1);
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   A cow is ready to eat\n");
			}
			
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   signal sheep meal flag %d\n", *mealSheepWaitingFlagp);
			semopChecked(semID, &SignalProtectSheepMealWaitingFlag, 1);
			
			for( k = 0; k < SHEEPS_IN_GROUP; k++ ) {
				semopChecked(semID, &SignalSheepsWaiting, 1);
				printf("SMAUGSMAUGSMAUGSMAUGSMAU  A sheep is ready to eat\n");
			}

			/*Smaug waits to eat*/
			semopChecked(semID, &WaitDragonEating, 1);
			for( k = 0; k < COWS_IN_GROUP; k++ ) 
			{
				semopChecked(semID, &SignalCowsDead, 1);
				cowsEatenTotal++;
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug finished eating a cow\n");
			}
			for( k = 0; k < SHEEPS_IN_GROUP; k++ ) 
			{
				semopChecked(semID, &SignalSheepsDead, 1);
				sheepsEatenTotal++;
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug finished eating a sheep\n");
			}
			printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has finished a meal\n");
			
			if( (cowsEatenTotal >= MAX_COWS_EATEN) && (sheepsEatenTotal >= MAX_SHEEPS_EATEN) ) 
			{
				printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has eaten the allowed number of cows and sheeps\n");
				*terminateFlagp= 1;	
				break; 
			}		
			printf("SMAUGSMAUGSMAUGSMAUGSMAU Smaug takes a nap for %d us\n", SMAUG_NAP_LENGTH);
 	 	 	usleep(SMAUG_NAP_LENGTH);
 	 	 	printf("SMAUGSMAUGSMAUGSMAUGSMAU Smaug takes a deep breath\n");
 	 	 	
			/* Smaug check to see if another snack is waiting */
			
			semopChecked(semID, &WaitProtectMealWaitingFlag, 1);
			semopChecked(semID, &WaitProtectSheepMealWaitingFlag, 1);
			
			if( *mealWaitingFlagp > 0 && *mealSheepWaitingFlagp > 0  ) 
			{
				printf("SMAUGSMAUGSMAUGSMAUGSMAU  %d  Smaug eats again\n", localpid);
				continue;
			}
		 }
		
 	  }
 	  semopChecked(semID, &SignalProtectMealWaitingFlag, 1); // release the protection
 	  semopChecked(semID, &SignalProtectSheepMealWaitingFlag, 1); // release the protection
 	  
 	  semopChecked(semID, &WaitProtectThiefCount, 1);
	  if(*thiefCounterp > 0) 
		{			

			*thiefCounterp = *thiefCounterp - 1;
			semopChecked(semID, &SignalProtectThiefCount, 1);
 		 	 	 
 	 	 	 // Wake thief from wander state for interaction
 	 	 	 	 semopChecked(semID, &SignalThievesWaiting, 1);
 	 	 	 	 printf("SMAUGSMAUGSMAUGSMAUGSMAU Smaug is playing with a thief\n");
 	 	 	 	 if( rand() % 100 <= smaugWinChance ) 
 	 	 	 	 {
 	 	 	 	 	 numJewels += 20;
 	 	 	 	 	 thiefsWonTotal++;
 	 	 	 	 	 printf("SMAUGSMAUGSMAUGSMAUGSMAU Smaug has defeated a thief\n");
 	 	 	 	 	 printf("SMAUGSMAUGSMAUGSMAUGSMAU Smaug has gained a treasure (%d jewels). He now has %d jewels.\n", 20, numJewels);
 	 	 	 	 } 
 	 	 	 	 else 
 	 	 	 	 {
 	 	 	 	 	 numJewels -= 8;
 	 	 	 	 	 printf("SMAUGSMAUGSMAUGSMAUGSMAU Smaug has been defeated by a thief\n");
 	 	 	 	 	 printf("SMAUGSMAUGSMAUGSMAUGSMAU Smaug has lost a treasure (%d jewels). He now has %d jewels.\n", 8, numJewels);
 	 	 	 	 }
 	 	 	  	
 	 	 	 	 printf("SMAUGSMAUGSMAUGSMAUGSMAU Smaug has finished a game (1 thief process has been terminated)\n");
				if(thiefsWonTotal >= MAX_THEIFES_WON ) 
				{
					printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has won the allowed number of thieves\n");
					 *terminateFlagp= 1;	
					break; 
				}		 	 
 	 	 	 	 if( numJewels >= MAX_TREASURE_IN_HOARD) 
 	 	 	 	 {
 	 	 	 	 	 
 	 	 	 	 	 printf("SMAUGSMAUGSMAUGSMAUGSMAU Smaug has %d, so he is richer then everyone so he leaves.\n", numJewels);
					 *terminateFlagp= 1;	
 	 	 	 	 	 break;
 	 	 	 	 }
 	 	 	 	 if( numJewels <= MIN_TREASURE_IN_HOARD) 
 	 	 	 	 {	 	 	 	 	 
 	 	 	 	 	 printf("SMAUGSMAUGSMAUGSMAUGSMAU Smaug has %d, so is very poor and decide to leave\n", numJewels);
					 *terminateFlagp= 1;	
 	 	 	 	 	 break;
				 }
				semopChecked(semID, &SignalThiefFinish, 1);
		}
		semopChecked(semID, &SignalProtectThiefCount, 1); //release 	

	  semopChecked(semID, &WaitProtectHunterCount, 1);
	  if(*hunterCounterp > 0) 
		{						
			// check thift
			*hunterCounterp = *hunterCounterp - 1;
			semopChecked(semID, &SignalProtectHunterCount, 1);
 		 	 	 
 	 	 	 	 semopChecked(semID, &SignalHuntersWaiting, 1);
 	 	 	 	 printf("SMAUGSMAUGSMAUGSMAUGSMAU Smaug is fighting with a hunter\n");
 	 	 	 	 if( rand() % 100 <= smaugWinChance ) 
 	 	 	 	 {
					 huntersWonTotal++;
 	 	 	 	 	 numJewels += 5;
 	 	 	 	 	 printf("SMAUGSMAUGSMAUGSMAUGSMAU Smaug has defeated a hunter\n");
 	 	 	 	 	 printf("SMAUGSMAUGSMAUGSMAUGSMAU Smaug has gained a treasure (%d jewels). He now has %d jewels.\n", 10, numJewels);
 	 	 	 	 } 
 	 	 	 	 else 
 	 	 	 	 {
 	 	 	 	 	 numJewels -= 10;
 	 	 	 	 	 printf("SMAUGSMAUGSMAUGSMAUGSMAU Smaug has been defeated by a hunter\n");
 	 	 	 	 	 printf("SMAUGSMAUGSMAUGSMAUGSMAU Smaug has lost a treasure (%d jewels). He now has %d jewels.\n", 5, numJewels);
 	 	 	 	 }
 	 	 
 	 	 	 	printf("SMAUGSMAUGSMAUGSMAUGSMAU Smaug has finished a fight(1 hunter process has been terminated)\n");
				if(huntersWonTotal >= MAX_HUNTERS_WON ) 
				{
					printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug has won the allowed number of hunters\n");
					   *terminateFlagp= 1;	
					break; 
				}	
 	 	 	 	 
 	 	 	 	 if( numJewels >= MAX_TREASURE_IN_HOARD) 
 	 	 	 	 {
 	 	 	 	 	 
 	 	 	 	 	 printf("SMAUGSMAUGSMAUGSMAUGSMAU Smaug has %d, so he is richer then everyone so he leaves.\n", numJewels);
					   *terminateFlagp= 1;	
 	 	 	 	 	 break;
 	 	 	 	 }
 	 	 	 	 if( numJewels <= MIN_TREASURE_IN_HOARD) 
 	 	 	 	 {	 	 	 	 	 
 	 	 	 	 	 printf("SMAUGSMAUGSMAUGSMAUGSMAU Smaug has %d, so is very poor and decide to leave\n", numJewels);
					   *terminateFlagp= 1;			
 	 	 	 	 	 break;
				 }
				 semopChecked(semID, &SignalHunterFinish, 1);
	
		}
		semopChecked(semID, &SignalProtectHunterCount, 1); //release 
		
		
		printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug sleeps again\n");
		semopChecked(semID, &WaitDragonSleeping, 1);
		printf("SMAUGSMAUGSMAUGSMAUGSMAU   Smaug is awake again\n");
		sleepThisIteration = 0;

	}
	
}

void thief(int startTimeN)
{
	int localpid;
	localpid = getpid();
    printf("THIFT %8d THIFT A thief arrived outside the valley\n", localpid);
    if( startTimeN > 0) 
    {
 	   if( usleep( startTimeN) == -1)
 	   {
 	 	 /* exit when usleep interrupted by kill signal */
 	 	 if(errno==EINTR)exit(4);
 	 }
 	if( *terminateFlagp == 1 ) {
 	 printf("THIFT %8d THIFT thift has found the magical path after we've been told to terminate\n", localpid);
 	 semopChecked(semID, &SignalProtectTerminate, 1);
 	 kill(localpid, SIGKILL);
 	 return;
 } else {
 	 printf("THIFT %8d THIFT thift has found the magical path in %d ms\n", localpid, startTimeN);
 	 semopChecked(semID, &SignalProtectTerminate, 1);
 }		 
 	 printf("THIFT %8d THIFT A thief was looking for path for a %f ms\n", localpid, startTimeN/1000.0);
 	 
	semopChecked(semID, &WaitProtectThiefCount, 1);
	*thiefCounterp = *thiefCounterp + 1;
	semopChecked(semID, &SignalProtectThiefCount, 1);
	printf("THIFT %8d THIFT thief is under smaug's spell and is waiting to be interacted with\n", localpid);
	printf("THIFT %8d THIFT thief wakes smaug\n", localpid);
	semopChecked(semID, &SignalDragonSleeping, 1);
	semopChecked(semID, &WaitThievesWaiting, 1);
	printf("THIFT %8d THIFT thief enters smaug's cave\n", localpid);
	printf("THIFT %8d THIFT thief plays with smaug\n", localpid);
	semopChecked(semID, &WaitThiefFinish, 1);
	printf("THIFT %8d THIFT thief leaves cave and goes home\n", localpid);
	kill(localpid, SIGKILL); // KILL AT THE END
 }
   
}

void hunter(int startTimeN)
{
	int localpid;
	localpid = getpid();
    printf("HUNTER %8d HUNTER A hunter arrived outside the valley\n", localpid);
    if( startTimeN > 0) 
    {
 	   if( usleep( startTimeN) == -1)
 	   {
 	 	 /* exit when usleep interrupted by kill signal */
 	 	 if(errno==EINTR)exit(4);
		}	
	  // Terminate check
 semopChecked(semID, &WaitProtectTerminate, 1);
 if( *terminateFlagp == 1 ) {
 	 printf("HUNTER %8d HUNTER hunter has found the magical path after we've been told to terminate\n", localpid);
 	 semopChecked(semID, &SignalProtectTerminate, 1);
 	 kill(localpid, SIGKILL);
 	 return;
 } else {
 	 printf("HUNTER %8d HUNTER hunter has found the magical path in %d ms\n", localpid, startTimeN);
 	 semopChecked(semID, &SignalProtectTerminate, 1);
 }	
		 
 	 printf("HUNTER %8d HUNTER A hunter was looking for path for a %f ms\n", localpid, startTimeN/1000.0);
 	 
	semopChecked(semID, &WaitProtectHunterCount, 1);
	*hunterCounterp = *hunterCounterp + 1;
	semopChecked(semID, &SignalProtectHunterCount, 1);
	printf("HUNTER %8d HUNTER hunter is under smaug's spell and is waiting to be interacted with\n", localpid);
	printf("HUNTER %8d HUNTER hunter wakes smaug\n", localpid);
	semopChecked(semID, &SignalDragonSleeping, 1);
	semopChecked(semID, &WaitHuntersWaiting, 1);
	printf("HUNTER %8d HUNTER hunter enters smaug's cave\n", localpid);
	printf("HUNTER %8d HUNTER hunter fights with smaug\n", localpid);
	semopChecked(semID, &WaitHunterFinish, 1);
	printf("HUNTER %8d HUNTER hunter leaves cave and goes home\n", localpid);
	kill(localpid, SIGKILL); // KILL AT THE END
	}
	
}

void cow(int startTimeN)
{
	int localpid;
	int k;
	localpid = getpid();

	/* graze */
	printf("CCCCCCC %8d CCCCCCC   A cow is born\n", localpid);
	if( startTimeN > 0) {
		if( usleep( startTimeN) == -1){
			/* exit when usleep interrupted by kill signal */
			if(errno==EINTR)exit(4);
		}	
	}
	printf("CCCCCCC %8d CCCCCCC   cow grazes for %f ms\n", localpid, startTimeN/1000.0);
	
	 semopChecked(semID, &WaitProtectTerminate, 1);
 if( *terminateFlagp == 1 ) 
 {
 	 printf("CCCCCCC %8d CCCCCCC cow has found the magical path after we've been told to terminate\n", localpid);
 	 semopChecked(semID, &SignalProtectTerminate, 1);
 	 kill(localpid, SIGKILL);
 	 return;
 } 
 else 
 {
 	 printf("CCCCCCC %d CCCCCCC cow has found the magical path in %d ms\n", localpid, startTimeN);
 	 semopChecked(semID, &SignalProtectTerminate, 1);
 }	

	/* does this beast complete a group of BEASTS_IN_GROUP ? */
	/* if so wake up the dragon */
	semopChecked(semID, &WaitProtectCowsInGroup, 1);
	semopChecked(semID, &SignalCowsInGroup, 1);
	*cowCounterp = *cowCounterp + 1;
	printf("CCCCCCC %8d CCCCCCC   %d  cows have been enchanted \n", localpid, *cowCounterp );
	if( ( *cowCounterp  >= COWS_IN_GROUP )) 
	{
		*cowCounterp = *cowCounterp - COWS_IN_GROUP;
		for (k=0; k<COWS_IN_GROUP; k++)
		{
			semopChecked(semID, &WaitCowsInGroup, 1);
		}
		printf("CCCCCCC %8d CCCCCCC   The last cow is waiting\n", localpid);
		semopChecked(semID, &SignalProtectCowsInGroup, 1);
		semopChecked(semID, &WaitProtectMealWaitingFlag, 1);
		*mealWaitingFlagp = *mealWaitingFlagp + 1;
		printf("CCCCCCC %8d CCCCCCC   signal meal flag %d\n", localpid, *mealWaitingFlagp);
		semopChecked(semID, &SignalProtectMealWaitingFlag, 1);
		
		
		
		semopChecked(semID, &WaitProtectSheepMealWaitingFlag, 1);
		if(*mealSheepWaitingFlagp >= 1 )
		{
			semopChecked(semID, &SignalDragonSleeping, 1);
		}
		semopChecked(semID, &SignalProtectSheepMealWaitingFlag, 1);
		
		printf("CCCCCCC %8d CCCCCCC   last cow  wakes the dragon \n", localpid);
	}
	else
	{
		semopChecked(semID, &SignalProtectCowsInGroup, 1);
	}

	semopChecked(semID, &WaitCowsWaiting, 1);

	/* have all the beasts in group been eaten? */
	/* if so wake up the dragon */
	semopChecked(semID, &WaitProtectCowsEaten, 1);
	semopChecked(semID, &SignalCowsEaten, 1);
	*cowsEatenCounterp = *cowsEatenCounterp + 1;
	if( ( *cowsEatenCounterp >= COWS_IN_GROUP )) {
		*cowsEatenCounterp = *cowsEatenCounterp - COWS_IN_GROUP;
		for (k=0; k<COWS_IN_GROUP; k++){
       		        semopChecked(semID, &WaitCowsEaten, 1);
		}
		printf("CCCCCCC %8d CCCCCCC   The last cow has been eaten\n", localpid);
		semopChecked(semID, &SignalProtectCowsEaten, 1);
		semopChecked(semID, &SignalDragonEating, 1);
	}
	else
	{
		semopChecked(semID, &SignalProtectCowsEaten, 1);
		printf("CCCCCCC %8d CCCCCCC   A cow is waiting to be eaten\n", localpid);
	}
	semopChecked(semID, &WaitCowsDead, 1);

	printf("CCCCCCC %8d CCCCCCC   cow  dies\n", localpid);
	kill(localpid, SIGKILL); // KILL AT THE END
}
	
void sheep(int startTimeN)
{
	int localpid;
	int k;
	localpid = getpid();

	/* graze */
	printf("SHEEP %8d SHEEP   A Sheep is born\n", localpid);
	if( startTimeN > 0) {
		if( usleep( startTimeN) == -1){
			/* exit when usleep interrupted by kill signal */
			if(errno==EINTR)exit(4);
		}	
	}
	printf("SHEEP %8d SHEEP  Sheep grazes for %f ms\n", localpid, startTimeN/1000.0);
	
	 semopChecked(semID, &WaitProtectTerminate, 1);
 if( *terminateFlagp == 1 ) 
 {
 	 printf("SHEEP %8d SHEEP Sheep has found the magical path after we've been told to terminate\n", localpid);
 	 semopChecked(semID, &SignalProtectTerminate, 1);
 	 kill(localpid, SIGKILL);
 	 return;
 } 
 else 
 {
 	 printf("SHEEP %d SHEEP Sheep has found the magical path in %d ms\n", localpid, startTimeN);
 	 semopChecked(semID, &SignalProtectTerminate, 1);
 }	

	/* does this beast complete a group of BEASTS_IN_GROUP ? */
	/* if so wake up the dragon */
	semopChecked(semID, &WaitProtectSheepsInGroup, 1);
	semopChecked(semID, &SignalSheepsInGroup, 1);
	*sheepCounterp = *sheepCounterp + 1;
	printf("SHEEP %8d SHEEP   %d  Sheep have been enchanted \n", localpid, *sheepCounterp );
	if( ( *sheepCounterp >= SHEEPS_IN_GROUP )) 
	{
		*sheepCounterp = *sheepCounterp - SHEEPS_IN_GROUP;
		for (k=0; k<SHEEPS_IN_GROUP; k++)
		{
			semopChecked(semID, &WaitSheepsInGroup, 1);
		}
		printf("SHEEP %8d SHEEP   The last sheep is waiting\n", localpid);
		semopChecked(semID, &SignalProtectSheepsInGroup, 1);
		semopChecked(semID, &WaitProtectSheepMealWaitingFlag, 1); //
		*mealSheepWaitingFlagp = *mealSheepWaitingFlagp + 1;
		printf("SHEEP %8d SHEEP  signal meal flag %d\n", localpid, *mealSheepWaitingFlagp);
		semopChecked(semID, &SignalProtectSheepMealWaitingFlag, 1); //
		
		semopChecked(semID, &WaitProtectMealWaitingFlag, 1);
		if(*mealSheepWaitingFlagp >= 1 && *mealWaitingFlagp >= 1 )
		{
			printf("!!!!!!!!!!!!  signal cow meal flag %d \n", *mealWaitingFlagp);
			printf("!!!!!!!!!!!!   signal sheep meal flag %d \n", *mealSheepWaitingFlagp);
			semopChecked(semID, &SignalDragonSleeping, 1);
		}
		semopChecked(semID, &SignalProtectMealWaitingFlag, 1);
		printf("SHEEP %8d SHEEP  last sheep wakes the dragon \n", localpid);
	}
	else
	{
		semopChecked(semID, &SignalProtectSheepsInGroup, 1);
	}

	semopChecked(semID, &WaitSheepsWaiting, 1);

	/* have all the beasts in group been eaten? */
	/* if so wake up the dragon */
	semopChecked(semID, &WaitProtectSheepsEaten, 1);
	semopChecked(semID, &SignalSheepsEaten, 1);
	*sheepsEatenCounterp = *sheepsEatenCounterp + 1;
	if( ( *sheepsEatenCounterp >= SHEEPS_IN_GROUP )) {
		*sheepsEatenCounterp = *sheepsEatenCounterp - SHEEPS_IN_GROUP;
		for (k=0; k<SHEEPS_IN_GROUP; k++){
       		        semopChecked(semID, &WaitSheepsEaten, 1);
		}
		printf("SHEEP %8d SHEEP   The last cow has been eaten\n", localpid);
		semopChecked(semID, &SignalProtectSheepsEaten, 1);
		semopChecked(semID, &SignalDragonEating, 1);
	}
	else
	{
		semopChecked(semID, &SignalProtectSheepsEaten, 1);
		printf("SHEEP %8d SHEEP   A sheep is waiting to be eaten\n", localpid);
	}
	semopChecked(semID, &WaitSheepsDead, 1);

	printf("SHEEP %8d SHEEP sheep dies\n", localpid);
	kill(localpid, SIGKILL); // KILL AT THE END
}
	
void initialize()
{
	/* Init semaphores */
	semID=semget(IPC_PRIVATE, 25, 0666 | IPC_CREAT);


	/* Init to zero, no elements are produced yet */
	seminfo.val=0;
	semctlChecked(semID, SEM_COWSINGROUP, SETVAL, seminfo);
	semctlChecked(semID, SEM_COWSWAITING, SETVAL, seminfo);
	semctlChecked(semID, SEM_COWSEATEN, SETVAL, seminfo);
	semctlChecked(semID, SEM_COWSDEAD, SETVAL, seminfo);
	
	semctlChecked(semID, SEM_SHEEPSINGROUP, SETVAL, seminfo);
	semctlChecked(semID, SEM_SHEEPSWAITING, SETVAL, seminfo);
	semctlChecked(semID, SEM_SHEEPSEATEN, SETVAL, seminfo);
	semctlChecked(semID, SEM_SHEEPSDEAD, SETVAL, seminfo);
	
	semctlChecked(semID, SEM_THIEVESWAITING, SETVAL, seminfo);
	semctlChecked(semID, SEM_THIEFFINISH, SETVAL, seminfo);
	
	semctlChecked(semID, SEM_HUNTERSWAITING, SETVAL, seminfo);
	semctlChecked(semID, SEM_HUNTERFINISH, SETVAL, seminfo);
	
	semctlChecked(semID, SEM_DRAGONFIGHTING, SETVAL, seminfo);
	semctlChecked(semID, SEM_DRAGONSLEEPING, SETVAL, seminfo);
	semctlChecked(semID, SEM_DRAGONEATING, SETVAL, seminfo);
	printf("!!INIT!!INIT!!INIT!!  semaphores initiialized\n");
	
	/* Init Mutex to one */
	
	seminfo.val=1;
	semctlChecked(semID, SEM_PTERMINATE, SETVAL, seminfo);
	
	semctlChecked(semID, SEM_PCOWSINGROUP, SETVAL, seminfo);
	semctlChecked(semID, SEM_PMEALWAITINGFLAG, SETVAL, seminfo);
	semctlChecked(semID, SEM_PCOWSEATEN, SETVAL, seminfo);
	
	
	
	semctlChecked(semID, SEM_PSHEEPSINGROUP, SETVAL, seminfo);
	semctlChecked(semID, SEM_PSHEEPSEATEN, SETVAL, seminfo);
	semctlChecked(semID, SEM_PSHEEPMEALWAITINGFLAG, SETVAL, seminfo);
	
	
	semctlChecked(semID, SEM_PTHIEFCOUNT, SETVAL, seminfo);
	
	semctlChecked(semID, SEM_PHUNTERCOUNT, SETVAL, seminfo);

	printf("!!INIT!!INIT!!INIT!!  mutexes initiialized\n");


	/* Now we create and attach  the segments of shared memory*/
        if ((terminateFlag = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
                printf("!!INIT!!INIT!!INIT!!  shm not created for terminateFlag\n");
                 exit(1);
        }
        else {
                printf("!!INIT!!INIT!!INIT!!  shm created for terminateFlag\n");
        }
	if ((cowCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for cowCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm created for cowCounter\n");
	}
	if ((mealWaitingFlag = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for mealWaitingFlag\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm created for mealWaitingFlag\n");
	}
	if ((mealSheepWaitingFlag = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for mealWaitingFlag\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm created for mealWaitingFlag\n");
	}
	if ((cowsEatenCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for cowsEatenCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm created for cowsEatenCounter\n");
	}
	
	if ((sheepCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for sheepsCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm created for sheepCounter\n");
	}
	if ((sheepsEatenCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for sheepsEatenCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm created for sheepsEatenCounter\n");
	}
	
	
	if ((thiefCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for thiefCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm created for thiefCounter\n");
	}
	if ((hunterCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
		printf("!!INIT!!INIT!!INIT!!  shm not created for hunterCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm created for hunterCounter\n");
	}


	/* Now we attach the segment to our data space.  */
        if ((terminateFlagp = shmat(terminateFlag, NULL, 0)) == (int *) -1) {
                printf("!!INIT!!INIT!!INIT!!  shm not attached for terminateFlag\n");
                exit(1);
        }
        else {
                 printf("!!INIT!!INIT!!INIT!!  shm attached for terminateFlag\n");
        }

	if ((cowCounterp = shmat(cowCounter, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for cowCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for cowCounter\n");
	}
	if ((sheepCounterp = shmat(cowCounter, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for sheepCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for cowCounter\n");
	}
	if ((mealWaitingFlagp = shmat(mealWaitingFlag, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for mealWaitingFlag\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for mealWaitingFlag\n");
	}
		if ((mealSheepWaitingFlagp = shmat(mealSheepWaitingFlag, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for mealWaitingFlag\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for mealWaitingFlag\n");
	}
	if ((cowsEatenCounterp = shmat(cowsEatenCounter, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for cowsEatenCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for cowsEatenCounter\n");
	}
	if ((sheepsEatenCounterp = shmat(sheepsEatenCounter, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for sheepsEatenCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for sheepsEatenCounter\n");
	}
	if ((thiefCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
 	 printf("!!INIT!!INIT!!INIT!! shm not created for thiefCounter\n");
 	 exit(1);
	}
	else 
	{
 	 printf("!!INIT!!INIT!!INIT!! shm created for thiefCounter\n");
	}
	
	if ((thiefCounterp = shmat(thiefCounter, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for thiefCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for thiefCounter\n");
	}
	printf("!!INIT!!INIT!!INIT!!   initialize end\n");
	
   if ((hunterCounter = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
 	 printf("!!INIT!!INIT!!INIT!! shm not created for hunterCounter\n");
 	 exit(1);
	}
	else 
	{
 	 printf("!!INIT!!INIT!!INIT!! shm created for hunterCounter\n");
	}
	
	if ((hunterCounterp = shmat(hunterCounter, NULL, 0)) == (int *) -1) {
		printf("!!INIT!!INIT!!INIT!!  shm not attached for hunterCounter\n");
		exit(1);
	}
	else {
		printf("!!INIT!!INIT!!INIT!!  shm attached for hunterCounter\n");
	}
	printf("!!INIT!!INIT!!INIT!!   initialize end\n");
}



void terminateSimulation() {
	pid_t localpgid;
	pid_t localpid;
	int w = 0;
	int status;

	localpid = getpid();
	printf("RELEASESEMAPHORES   Terminating Simulation from process %8d\n", localpid);
	if(cowProcessGID != (int)localpgid ){
		if(killpg(cowProcessGID, SIGKILL) == -1 && errno == EPERM) {
			printf("XXTERMINATETERMINATE   COWS NOT KILLED\n");
		}
		printf("XXTERMINATETERMINATE   killed cows \n");
	}
	if(sheepProcessGID != (int)localpgid ){
		if(killpg(sheepProcessGID, SIGKILL) == -1 && errno == EPERM) {
			printf("XXTERMINATETERMINATE   SHEEPS NOT KILLED\n");
		}
		printf("XXTERMINATETERMINATE   killed sheep \n");
	}
	if(thiefProcessGID != (int)localpgid ){
		if(killpg(thiefProcessGID, SIGKILL) == -1 && errno == EPERM) {
			printf("XXTERMINATETERMINATE   thiefs NOT KILLED\n");
		}
		printf("XXTERMINATETERMINATE  kills thief \n");
	}
	
	if(hunterProcessGID != (int)localpgid ){
		if(killpg(hunterProcessGID, SIGKILL) == -1 && errno == EPERM) {
			printf("XXTERMINATETERMINATE   thiefs NOT KILLED\n");
		}
		printf("XXTERMINATETERMINATE kills  hunter \n");
	}
	
	
	if(smaugProcessID != (int)localpgid ) {
		kill(smaugProcessID, SIGKILL);
		printf("XXTERMINATETERMINATE   killed smaug\n");
	}
	while( (w = waitpid( -1, &status, WNOHANG)) > 1){
			printf("REAPED process in terminate %d\n", w);
	}
	releaseSemandMem();
	printf("GOODBYE from terminate\n");
}

void releaseSemandMem() 
{
	pid_t localpid;
	int w = 0;
	int status;

	localpid = getpid();

	//should check return values for clean termination
	semctl(semID, 0, IPC_RMID, seminfo);


	// wait for the semaphores 
	usleep(2000);
	while( (w = waitpid( -1, &status, WNOHANG)) > 1){
			printf("REAPED process in terminate %d\n", w);
	}
	printf("\n");
        if(shmdt(terminateFlagp)==-1) {
                printf("RELEASERELEASERELEAS   terminateFlag share memory detach failed\n");
        }
        else{
                printf("RELEASERELEASERELEAS   terminateFlag share memory detached\n");
        }
        if( shmctl(terminateFlag, IPC_RMID, NULL ))
        {
                printf("RELEASERELEASERELEAS   share memory delete failed %d\n",*terminateFlagp );
        }
        else{
                printf("RELEASERELEASERELEAS   share memory deleted\n");
        }
	if( shmdt(cowCounterp)==-1)
	{
		printf("RELEASERELEASERELEAS   cowCounterp memory detach failed\n");
	}
	else{
		printf("RELEASERELEASERELEAS   cowCounterp memory detached\n");
	}
	if( shmctl(cowCounter, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   cowCounter memory delete failed \n");
	}
	else{
		printf("RELEASERELEASERELEAS   cowCounter memory deleted\n");
	}
	if( shmdt(sheepCounterp)==-1)
	{
		printf("RELEASERELEASERELEAS   sheepCounterp memory detach failed\n");
	}
	else{
		printf("RELEASERELEASERELEAS   sheepCounterp memory detached\n");
	}
	if( shmctl(sheepCounter, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   sheepCounter memory delete failed \n");
	}
	else{
		printf("RELEASERELEASERELEAS   sheepCounter memory deleted\n");
	}
	if( shmdt(mealWaitingFlagp)==-1)
	{
		printf("RELEASERELEASERELEAS   mealWaitingFlagp memory detach failed\n");
	}
	else{
		printf("RELEASERELEASERELEAS   mealWaitingFlagp memory detached\n");
	}
	if( shmctl(mealWaitingFlag, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   mealWaitingFlag share memory delete failed \n");
	}
	else{
		printf("RELEASERELEASERELEAS   mealWaitingFlag share memory deleted\n");
	}
	if( shmdt(cowsEatenCounterp)==-1)
	{
		printf("RELEASERELEASERELEAS   cowsEatenCounterp memory detach failed\n");
	}
	else{
		printf("RELEASERELEASERELEAS   cowsEatenCounterp memory detached\n");
	}
	if( shmctl(cowsEatenCounter, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   cowsEatenCounter memory delete failed \n");
	}
	else{
		printf("RELEASERELEASERELEAS   cowsEatenCounter memory deleted\n");
	}
	
	if( shmdt(sheepsEatenCounterp)==-1)
	{
		printf("RELEASERELEASERELEAS   sheepsEatenCounterp memory detach failed\n");
	}
	else{
		printf("RELEASERELEASERELEAS   sheepsEatenCounterp memory detached\n");
	}
	if( shmctl(sheepsEatenCounter, IPC_RMID, NULL ))
	{
		printf("RELEASERELEASERELEAS   sheepsEatenCounter memory delete failed \n");
	}
	else{
		printf("RELEASERELEASERELEAS   sheepsEatenCounter memory deleted\n");
	}
	
	 if( shmdt(thiefCounterp)==-1)
 {
 	 printf("RELEASERELEASERELEAS thiefCounterp memory detach failed\n");
 }
 else{
 	 printf("RELEASERELEASERELEAS thiefCounterp memory detached\n");
 }
 if( shmctl(thiefCounter, IPC_RMID, NULL ))
 {
 	 printf("RELEASERELEASERELEAS thiefCounterp memory delete failed \n");
 }
 else{
 	 printf("RELEASERELEASERELEAS thiefCounterp memory deleted\n");
 }

if( shmdt(hunterCounterp)==-1)
 {
 	 printf("RELEASERELEASERELEAS hunterCounterp memory detach failed\n");
 }
 else{
 	 printf("RELEASERELEASERELEAS hunterCounterp memory detached\n");
 }
 if( shmctl(hunterCounter, IPC_RMID, NULL ))
 {
 	 printf("RELEASERELEASERELEAS hunterCounter memory delete failed \n");
 }
 else
 {
 	 printf("RELEASERELEASERELEAS hunterCounter memory deleted\n");
 }

}


void semctlChecked(int semaphoreID, int semNum, int flag, union semun seminfo) { 
	/* wrapper that checks if the semaphore control request has terminated */
	/* successfully. If it has not the entire simulation is terminated */

	if (semctl(semaphoreID, semNum, flag,  seminfo) == -1 ) {
		if(errno != EIDRM) {
			printf("semaphore control failed: simulation terminating\n");
			printf("errno %8d \n",errno );
			*terminateFlagp = 1;
			releaseSemandMem();
			exit(2);
		}
		else {
			exit(3);
		}
	}
}

void semopChecked(int semaphoreID, struct sembuf *operation, unsigned something) 
{

	/* wrapper that checks if the semaphore operation request has terminated */
	/* successfully. If it has not the entire simulation is terminated */
	if (semop(semaphoreID, operation, something) == -1 ) {
		if(errno != EIDRM) {
			printf("semaphore operation failed: simulation terminating\n");
			*terminateFlagp = 1;
			releaseSemandMem();
			exit(2);
		}
		else {
			exit(3);
		}
	}
}


double timeChange( const struct timeval startTime )
{
	struct timeval nowTime;
	double elapsedTime;

	gettimeofday(&nowTime,NULL);
	elapsedTime = (nowTime.tv_sec - startTime.tv_sec)*1000.0;
	elapsedTime +=  (nowTime.tv_usec - startTime.tv_usec)/1000.0;
	return elapsedTime;

}	

int askUserValue(char *text); 

int askUserValue(char *text) 
{
 printf("Enter the value for %s: ", text);
 int input = 0;
 scanf("%d", &input); // takeinput as an int value
 return input;
}

int main()
{
 initialize();
 
 printf("Welcome to Smaug World Simulator\n");

 const int seed = 1; //askUserValue("Enter the random value seed");
 const long int maximumCowInterval = 10000000;//askUserValue("Enter maximumCowInterval in (us)");
 const long int maximumThiefInterval = 10000000;//askUserValue("Enter maximumThiefInterval in (us)");
 const long int maximumHunterInterval = 10000000;//askUserValue("Enter maximumHunterInterval in (us)");
 const long int maximumSheepInterval = 10000000;//askUserValue("Enter maximumSheepInterval in (us)");
 const int smaugWinChance = 50; //askUserValue("smaugWinProb (0 to 100)");
 srand(seed);

 
 double cowTimer = 0;
 double sheepTimer = 0;
 double thiefTimer = 0;
 double hunterTimer = 0;
 
 
 parentProcessID = getpid();

 smaugProcessID = -1;
 cowProcessGID = parentProcessID - 1;
 thiefProcessGID = parentProcessID - 2;
 hunterProcessGID = parentProcessID - 3;
 sheepProcessGID = parentProcessID - 4;
  
  pid_t childPID = fork();
  
 if(childPID < 0) {
 	 printf("FORK FAILED\n");
 	 return 1;
 } else if(childPID == 0) {
 	 smaug(smaugWinChance); // run the smaug
 	 return 0;
 }
 
 smaugProcessID = childPID;
 gettimeofday(&startTime, NULL);
 int zombieRemoveCounter = 0; // Variable to kill zombie
 while(*terminateFlagp == 0) 
 {
 	 zombieRemoveCounter++;
 	 double simDuration = timeChange(startTime);
 
 
 	 if(cowTimer - simDuration <= 0) {
 	 	 cowTimer = simDuration + (rand() % maximumCowInterval) / 1000.0;
 	 	 printf("COW CREATED! next cow at: %f\n", cowTimer);
 	 	 int childPID = fork();
 	 	 if(childPID == 0) {
 	 	 	 cow((rand() % maximumCowInterval) / 1000.0);
 	 	 	 return 0;
 	 	 }
 	 }
 	 if(thiefTimer - simDuration <= 0) 
 	 {
 	 	 thiefTimer = simDuration + (rand() % maximumThiefInterval) / 1000.0;
 	 	 printf("THIEF CREATED! next thief at: %f\n", thiefTimer);
 	 	 int childPID = fork();
 	 	 if(childPID == 0) 
 	 	 {
 	 	 	thief((rand() % maximumThiefInterval) / 1000.0);
 	 	 	return 0;
 	 	 }
 	 }
 	 
 	if(hunterTimer - simDuration <= 0) 
 	 {
 	 	 hunterTimer = simDuration + (rand() % maximumHunterInterval) / 1000.0;
 	 	 printf("HUNTER CREATED! next hunter at: %f\n", hunterTimer);
 	 	 int childPID = fork();
 	 	 if(childPID == 0) 
 	 	 {
 	 	 	hunter((rand() % maximumHunterInterval) / 1000.0);
 	 	 	return 0;
 	 	 }
 	 }
 	if(sheepTimer - simDuration <= 0) 
 	 {
 	 	 sheepTimer = simDuration + (rand() % maximumSheepInterval) / 1000.0;
 	 	 printf("SHEEP CREATED! next sheep at: %f\n", sheepTimer);
 	 	 int childPID = fork();
 	 	 if(childPID == 0) 
 	 	 {
 	 	 	sheep((rand() % maximumSheepInterval) / 1000.0);
 	 	 	return 0;
 	 	 }
 	 }

 	 // remove zombie processes once in a 10 runs ~ at most 10 process
 	 if(zombieRemoveCounter % 10 == 0) 
 	 {
 	 	 zombieRemoveCounter -= 10;
 	 	 int w = 0; int status = 0;
 	 	 while( (w = waitpid( -1, &status, WNOHANG)) > 1)
 	 	 {
			printf("REAPED zombie process %d from main loop\n", w);
 	 	 }
 	 }
 }

 terminateSimulation();
 return 0;
}

// bush script to release memory
// ipcs | nawk -v u=`whoami` '/Shared/,/^$/{ if($6==0&&$3==u) print "ipcrm shm",$2,";"}/Semaphore/,/^$/{ if($3==u) print "ipcrm sem",$2,";"}' | /bin/sh