#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/resource.h>
#include <errno.h>
#include <wait.h>
#include <unistd.h>

/* Threads to create and control vehiclepthreads */
pthread_t vehicleCreationThread;
pthread_t vehicleThread[300]; // need many of these to carry all the vihicles
pthread_t captainThread;
int threadCounter = 0;

/* Counters and their corresponding Mutexes */
pthread_mutex_t protectCarsQueued;
int carsQueuedCounter = 0;
pthread_mutex_t protectTrucksQueued;
int trucksQueuedCounter = 0;
pthread_mutex_t protectCarsUnloaded;
int carsUnloadedCounter = 0;
pthread_mutex_t protectTrucksUnloaded;
int trucksUnloadedCounter = 0;

/* Counting semaphores to manage cars and trucks */
sem_t carsQueued;
sem_t trucksQueued;
sem_t carsLoaded;
sem_t trucksLoaded;
sem_t carsUnloaded;
sem_t trucksUnloaded;
sem_t vehiclesSailing;
sem_t vehiclesArrived;
sem_t waitUnload;
sem_t readyUnload;
sem_t waitToExit;


#define spaces_on_ferry 6

//Constants declaration
struct timeval startTime;


//Functions declaration
void init();
void* vehicle_spawn();
void* truck();
void* car();
void capitain_yagooar();
void clean();
int timeChange( const struct timeval startTime );
int ferryIsFull = 0;



int maxTimeToNextVehicleArrival;

int trackChance; 

//Threads functions declaration
int sem_waitChecked(sem_t *semaphoreID);
int sem_postChecked(sem_t *semaphoreID);
int sem_initChecked(sem_t *semaphoreID, int pshared, unsigned int value);
int sem_destroyChecked(sem_t *semaphoreID);
int pthread_mutex_lockChecked(pthread_mutex_t *mutexID);
int pthread_mutex_unlockChecked(pthread_mutex_t *mutexID);
int pthread_mutex_initChecked(pthread_mutex_t *mutexID,
                              const pthread_mutexattr_t *attrib);
int pthread_mutex_destroyChecked(pthread_mutex_t *mutexID);
int timeChange( const struct timeval startTime );
int askUserValue(char *text);

void* captain_yagooar()
{
    int localThreadId;
    localThreadId = (int )pthread_self();
    {
        /* Counting variables for cars and trucks to determine when ferry is full */
        int currentLoad = 0;
        int numberOfCarsQueued = 0;
        int numberOfTrucksQueued = 0;
        int numberOfTrucksLoaded = 0;
        int numberOfSpacesFilled = 0;
        int numberOfSpacesEmpty = 0;
        int numberOfVehicles = 0;
        int counter = 0;
        printf("Captain process %d started \n", localThreadId);
        while (currentLoad < 11)
        {

            numberOfTrucksLoaded = 0;
            numberOfSpacesFilled = 0;
            numberOfVehicles = 0;

            while(numberOfSpacesFilled < 6)
            {
                // transfer from comming to
                pthread_mutex_lockChecked(&protectTrucksQueued);
                pthread_mutex_lockChecked(&protectCarsQueued);
                numberOfTrucksQueued = trucksQueuedCounter;
                numberOfCarsQueued = carsQueuedCounter;
                
                pthread_mutex_unlockChecked(&protectCarsQueued);
                pthread_mutex_unlockChecked(&protectTrucksQueued);
                
                while(numberOfTrucksQueued > 0 && numberOfSpacesFilled < 5 && numberOfTrucksLoaded < 2)
                {
                    pthread_mutex_lockChecked(&protectTrucksQueued);
                    trucksQueuedCounter--;
                    printf("Captain signalled Truck to leave the queue \n");
                    sem_postChecked(&trucksQueued);
                    
                    pthread_mutex_unlockChecked(&protectTrucksQueued);
                    numberOfTrucksQueued--;
                    numberOfTrucksLoaded++;
                    numberOfSpacesFilled+=2;
                    numberOfVehicles++;
                }
                while(numberOfCarsQueued > 0 && numberOfSpacesFilled < 6)
                {
                    pthread_mutex_lockChecked(&protectCarsQueued);
                    carsQueuedCounter--;
                    printf("Captain signalled Car to leave the queue\n");
                    sem_postChecked(&carsQueued);
                    pthread_mutex_unlockChecked(&protectCarsQueued);
                    numberOfCarsQueued--;
                    numberOfSpacesFilled++;
                    numberOfVehicles++;
                }
            }
            for(counter = 0; counter < numberOfTrucksLoaded; counter++)
            {
                sem_waitChecked(&trucksLoaded);
                printf("Captain acknowledges that Truck loaded\n");
            }
            for(counter = 0; counter < numberOfVehicles - numberOfTrucksLoaded;
                counter++) {
                sem_waitChecked(&carsLoaded);
                printf("Captain acknowledges that Car loaded\n");
            }
            printf("Captain announced that ferry is full and is about to sail\n");

            for(counter = 0; counter < numberOfVehicles; counter++) {
                printf("Captain knows vehicle %d has acknowledged it is sailing\n",counter);
                sem_postChecked(&vehiclesSailing);
            }
            printf("Captain announced that all vehicles sailing\n");

            for(counter = 0; counter < numberOfVehicles; counter++)
            {
                printf("Captain acknowledges that vehicle %d has arrived and knows about it\n", counter);
                sem_postChecked(&vehiclesArrived);
            }

            for(counter = 0; counter < numberOfVehicles; counter++)
            {
                sem_waitChecked(&readyUnload);
            }
            printf("Ferry arrived at destination and docked\n");

            for(counter = 0; counter < numberOfVehicles; counter++)
            {
                sem_postChecked(&waitUnload);
            }

            numberOfSpacesEmpty = 0;
            while(numberOfSpacesEmpty < 6)
            {
                pthread_mutex_lockChecked(&protectCarsUnloaded);
                if( carsUnloadedCounter > 0 )
                {
                    sem_waitChecked(&carsUnloaded);
                    carsUnloadedCounter--;
                    numberOfSpacesEmpty++;
                    printf("Captain acknowledges that car has unloaded from the ferry\n");
                }
                pthread_mutex_unlockChecked(&protectCarsUnloaded);
                pthread_mutex_lockChecked(&protectTrucksUnloaded);
                if( trucksUnloadedCounter > 0 )
                {
                    sem_waitChecked(&trucksUnloaded);
                    trucksUnloadedCounter--;
                    numberOfSpacesEmpty+=2;
                    printf("Captain acknowledges that truck has unloaded from the ferry\n");
                }
                pthread_mutex_unlockChecked(&protectTrucksUnloaded);
            }
            printf("Captain acknowledges that unloading is complete\n");
            for(counter = 0; counter < numberOfVehicles; counter++)
            {
                printf("Captain acknowledges that unloaded vehicle %d is about to exit\n", counter);
                sem_post(&waitToExit);
            }
            printf("\n\n\n\n\n");
            printf("Captain arrived at Home dock\n\n");
            currentLoad++;
            if(currentLoad >= 11)
                printf("Captain announces that load %d/%d has started. Terminating simulation!\n", currentLoad, 11);
        }
        exit(0);
    }
}

void* truck()
{
    int localThreadId;
    localThreadId = (int )pthread_self();
    // adding truck
    
    pthread_mutex_lockChecked(&protectTrucksQueued);
    trucksQueuedCounter++;
    pthread_mutex_unlockChecked(&protectTrucksQueued);
    
    printf("Truck %d arrives at the ferry dock \n", localThreadId);
    sem_waitChecked(&trucksQueued);
    printf("Truck %d is leaving queue to load\n", localThreadId);
    // ack that it was loaded
    printf("Truck %d is onboard of the ferry \n", localThreadId);
    sem_postChecked(&trucksLoaded);
    // waiting for captain signal
    sem_waitChecked(&vehiclesSailing);
    printf("Truck %d is sailing\n", localThreadId);
    // waiting for arival
    sem_waitChecked(&vehiclesArrived);
    printf("Truck %d has arrived at destination\n", localThreadId);
    // ready to unload
    sem_postChecked(&readyUnload);
    // waiting for command to unload
    sem_waitChecked(&waitUnload);
    // increment trucksUnloaded counter
    pthread_mutex_lockChecked(&protectTrucksUnloaded);
    trucksUnloadedCounter++;
    pthread_mutex_unlockChecked(&protectTrucksUnloaded);
    printf("Truck %d unloaded from the ferry\n", localThreadId);
    // waiting till get unloaded
    sem_postChecked(&trucksUnloaded);
    sem_waitChecked(&waitToExit);
    //Leave here
    printf("Truck %d exits\n", localThreadId);
    pthread_exit(0);
}


void* car()
{
    int localThreadId;
    localThreadId = (int)pthread_self();
    // adding car
    pthread_mutex_lockChecked(&protectCarsQueued);
    carsQueuedCounter++;
    pthread_mutex_unlockChecked(&protectCarsQueued);
    
    printf("Car %d arrives at the ferry dock\n", localThreadId);
    sem_waitChecked(&carsQueued);
    printf("Car %d is leaving the queue to load\n", localThreadId);
    // ack that we got loaded
    printf("Car %d is onboard of the ferry \n", localThreadId);
    sem_postChecked(&carsLoaded);
    // waiting for captain to signal
    sem_waitChecked(&vehiclesSailing);
    printf("Car %d traveling\n", localThreadId);
    // waiting for arrival at destination dock
    sem_waitChecked(&vehiclesArrived);
    printf("Car %d has arrived at destination\n", localThreadId);
    // ready to unload
    sem_postChecked(&readyUnload);
    // waiting for comand to unload
    sem_waitChecked(&waitUnload);
    // increment carsUnloaded counter
    pthread_mutex_lockChecked(&protectCarsUnloaded);
    carsUnloadedCounter++;
    pthread_mutex_unlockChecked(&protectCarsUnloaded);
    printf("Car %d unloaded from the ferry\n", localThreadId);
    // wait till it unloads
    sem_postChecked(&carsUnloaded);
    sem_waitChecked(&waitToExit);
    
    //Leave  destination dock here
    printf("Car %d exits\n", localThreadId);
    pthread_exit(0);
}

void init()
{
    
    sem_initChecked(&carsQueued, 0, 0);
    sem_initChecked(&trucksQueued, 0, 0);
    sem_initChecked(&carsLoaded, 0, 0);
    sem_initChecked(&trucksLoaded, 0, 0);
    sem_initChecked(&vehiclesSailing, 0, 0);
    sem_initChecked(&vehiclesArrived, 0, 0);
    sem_initChecked(&waitUnload, 0, 0);
    sem_initChecked(&readyUnload, 0, 0);
    sem_initChecked(&carsUnloaded, 0, 0);
    sem_initChecked(&trucksUnloaded, 0, 0);
    sem_initChecked(&waitToExit, 0, 0);
    printf("Initialization complite\n");
    
}

int main()
{
    
    init();
    
    printf("Enter integer values for the following variables\n");
    
    // Set  truck arrivial probability
    trackChance=askUserValue("Enter the percent probability that the next vehicle is a truck (0..100):\n");
    
    while(trackChance < 0 || trackChance > 100)
    {
        printf("Probability must be between 0 and 100:\n");
        trackChance=askUserValue("Enter the percent probability that the next vehicle is a truck (0..100):\n");
    }
    
    // Set vehicle interval time
    maxTimeToNextVehicleArrival=askUserValue("Enter the maximum length of the interval between vehicles (100..MAX_INT):\n");
    
    while(maxTimeToNextVehicleArrival < 100)
    {
        printf("Interval must be greater than %d:\n", 100);
        maxTimeToNextVehicleArrival=askUserValue("Enter the maximum length of the interval between vehicles (100..MAX_INT):\n");
    }
    
    pthread_create(&vehicleCreationThread, NULL, vehicle_spawn, NULL);
    pthread_create(&captainThread, NULL, captain_yagooar, NULL);
    pthread_mutex_initChecked(&protectCarsQueued, NULL);
    pthread_mutex_initChecked(&protectTrucksQueued, NULL);
    pthread_mutex_initChecked(&protectCarsUnloaded, NULL);
    pthread_mutex_initChecked(&protectTrucksUnloaded, NULL);
    //finishes execution and waits for everyone
    pthread_join(vehicleCreationThread, NULL);
    pthread_join(captainThread, NULL);
    clean();
    return 0;
}


void* vehicle_spawn()
{
    int localThreadId;
    localThreadId = (int)pthread_self();
    // Time at the start of process creation
    struct timeval startTime;
    // Time since process creation to current time
    int elapsed = 0;
    // Time at which last vehicle arrived
    int lastArrivalTime = 0;
    
    printf(" VehicleCreationProcess created. PID is: %d \n", localThreadId);
    gettimeofday(&startTime, NULL);
    elapsed = timeChange(startTime);
    srand (elapsed);
    while(1)
    {
        
        elapsed = timeChange(startTime);
        if(elapsed >= lastArrivalTime)
        {
            printf("Elapsed time: %d Arrival time: %d\n", elapsed, lastArrivalTime);
            if(lastArrivalTime > 0 )
            {
                if(rand() % 100 < trackChance )
                {
                    //truck
                    printf("Created a truck process\n");
                    pthread_create(&(vehicleThread[threadCounter]), NULL, truck, NULL);
                }
                else
                {
                    //car
                    printf("Created a car process\n");
                    pthread_create(&(vehicleThread[threadCounter]), NULL, car, NULL);
                }
            }
            lastArrivalTime += rand()% maxTimeToNextVehicleArrival;
            printf("Present time %d, next arrival time %d\n", elapsed, lastArrivalTime);
        }
        
    }
    
    printf("VehicleCreationProcess ended\n");
    return 0;
}


void clean()
{
    
    pthread_mutex_destroyChecked(&protectTrucksQueued);
    pthread_mutex_destroyChecked(&protectCarsQueued);
    pthread_mutex_destroyChecked(&protectCarsUnloaded);
    pthread_mutex_destroyChecked(&protectTrucksUnloaded);
    sem_destroyChecked(&carsQueued);
    sem_destroyChecked(&trucksQueued);
    sem_destroyChecked(&carsLoaded);
    sem_destroyChecked(&trucksLoaded);
    sem_destroyChecked(&vehiclesSailing);
    sem_destroyChecked(&vehiclesArrived);
    sem_destroyChecked(&waitUnload);
    sem_destroyChecked(&readyUnload);
    sem_destroyChecked(&carsUnloaded);
    sem_destroyChecked(&trucksUnloaded);
    sem_destroyChecked(&waitToExit);
    printf("Cleaning Complite\n");
}

int askUserValue(char *text)
{
    printf("Enter the value for %s: ", text);
    int input = 0;
    scanf("%d", &input); // take input as an int value
    return input;
}

int timeChange( const struct timeval startTime )
{
    struct timeval nowTime;
    long int elapsed;
    int elapsedTime;
    gettimeofday(&nowTime, NULL);
    elapsed = (nowTime.tv_sec - startTime.tv_sec) * 1000000
    + (nowTime.tv_usec - startTime.tv_usec);
    elapsedTime = elapsed / 1000;
    return elapsedTime;
}

int sem_waitChecked(sem_t *semaphoreID)
{
    int returnValue;
    returnValue = sem_wait(semaphoreID);
    if (returnValue == -1 ) {
        printf("Semaphore wait failed: simulation terminating\n");
        //terminateSimulation();
        exit(0);
    }
    return returnValue;
}

int sem_postChecked(sem_t *semaphoreID)
{
    int returnValue;
    returnValue = sem_post(semaphoreID);
    if (returnValue < 0 ) {
        printf("Semaphore post operation failed: simulation terminating\n");
        //terminateSimulation();
        exit(0);
    }
    return returnValue;
}

int sem_initChecked(sem_t *semaphoreID, int pshared, unsigned int value)
{
    int returnValue;
    returnValue = sem_init(semaphoreID, pshared, value);
    if (returnValue < 0 ) {
        printf("Semaphore init operation failed: simulation terminating\n");
        //terminateSimulation();
        exit(0);
    }
    return returnValue;
}

int sem_destroyChecked(sem_t *semaphoreID)
{
    int returnValue;
    returnValue = sem_destroy(semaphoreID);
    if (returnValue < 0 ) {
        printf("Semaphore destroy operation failed: simulation terminating\n");
        //terminateSimulation();
        exit(0);
    }
    return returnValue;
}

int pthread_mutex_lockChecked(pthread_mutex_t *mutexID)
{
    int returnValue;
    returnValue = pthread_mutex_lock(mutexID);
    if (returnValue < 0 ) {
        printf("pthread mutex lock operation failed: terminating\n");
        //terminateSimulation();
        exit(0);
    }
    return returnValue;
}

int pthread_mutex_unlockChecked(pthread_mutex_t *mutexID)
{
    int returnValue;
    returnValue = pthread_mutex_unlock(mutexID);
    if (returnValue < 0 ) {
        printf("pthread mutex unlock operation failed: terminating\n");
        //terminateSimulation();
        exit(0);
    }
    return returnValue;
}

int pthread_mutex_initChecked(pthread_mutex_t *mutexID,
                              const pthread_mutexattr_t *attrib)
{
    int returnValue;
    returnValue = pthread_mutex_init(mutexID, attrib);
    if (returnValue < 0 ) {
        printf("pthread init operation failed: simulation terminating\n");
        //terminateSimulation();
        exit(0);
    }
    return returnValue;
}

int pthread_mutex_destroyChecked(pthread_mutex_t *mutexID)
{
    int returnValue;
    returnValue = pthread_mutex_destroy(mutexID);
    if (returnValue < 0 ) {
        printf("pthread destroy failed: simulation terminating\n");
        //terminateSimulation();
        exit(0);
    }
    return returnValue;
}