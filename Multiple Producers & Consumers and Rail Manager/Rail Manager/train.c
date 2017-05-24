/***************************************************************
Operating Systems Lab Assignment 5 : Rail Manager

Name : Kumar Ayush
Roll No. 13CS10058

Name : Varun Rawal
Roll No. 13CS10059

****************************************************************/

// Train Program

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/sem.h>

// IMPLEMENTATION using LINUX/POSIX Sempahores

// the "shared semaphore" word in the above theory is implemented by extracting the sempahores array using a common key_t among different using processes 
// also a record file "matrix.txt" is a shared one

// semaphore concepts :

// A semaphore S = +1 i.e. it is available
// A semaphore S = 0 i.e. it is NOT available, and any processes wanting to use it will be blocked (forced to sleep) on it, until the semaphore is again AVAILABLE (+1)

// I want to use semaphore S -->  use S with sem-op = -1
// I want to make S available --> use S with sem-op = +1

#define MATRIX_FILENAME "matrix.txt"

#define SEMAPHORE_KEY 10

// these are the indices of the sub-semaphores to be used as different mutex locks
#define MATRIX_MUTEX 0
#define JUNCTION_MUTEX 1

#define NORTH_SEMAPHORE 2
#define WEST_SEMAPHORE 3
#define SOUTH_SEMAPHORE 4
#define EAST_SEMAPHORE 5

#define JUNCTION_CROSSING_TIME 2 // seconds 

int semID;
int matrix_index;
char train_direction; // any one of 'N', 'S', 'E' or 'W'

int N,M=4;

char* getDirString(char c)
{
	switch(c)
	{
		case 'N':{return "North";break;}
		case 'W':{return "West";break;}
		case 'E':{return "East";break;}
		case 'S':{return "South";break;}
		default: {printf("Direction Invalid ! exit..\n"); exit(1);}
	}
}

int getDirectionSemaphore(char dir)
{
	switch(dir)
	{
		case 'N': return NORTH_SEMAPHORE;
		case 'W': return WEST_SEMAPHORE;
		case 'E': return EAST_SEMAPHORE;
		case 'S': return SOUTH_SEMAPHORE;
		default: 
		printf("Invalid Direction !\n");
		exit(0);
	}
}

void updateMatrixFile(char direction,int entry)
{
	/*
		The matrix will be in following format :

				N 	W  S  E 
		t0 : 	-   -  -  -  
		t1 :	-   -  -  -  
		|
		|
		|
	*/
		const static int north = 0,west = 1,south = 2,east = 3;

		int dir_index;

		switch(direction)
		{
			case 'N': {dir_index=north; break;}
			case 'W': {dir_index=west; break;}
			case 'S': {dir_index=south; break;}
			case 'E': {dir_index=east; break;}
			default: {printf("Invalid Direction !\n"); exit(1);}
		}

		int matrix[N][M];
		FILE *fp = fopen(MATRIX_FILENAME, "r+");

		int i,j;
		for(i=0;i< N ;i++)
		{
			for(j=0;j< M ;j++)
			{
				fscanf(fp, "%d", &matrix[i][j]);
			}
		}
		fclose(fp);

		matrix[matrix_index][dir_index] = entry;

		fp = fopen(MATRIX_FILENAME, "w+");

		for(i=0;i< N ;i++)
		{
			for(j=0;j< M ;j++)
			{
				fprintf(fp, "%d ", matrix[i][j]);
			}

			fprintf(fp, "\n");
		}

		fclose(fp);

}

void acquireMutexLock(int sub_sema_index)
{
	struct sembuf sem_operator;

	sem_operator.sem_num = sub_sema_index;
	sem_operator.sem_op = -1;
	sem_operator.sem_flg = 0; // blocking wait, if mutex already acquired

	semop(semID,&sem_operator,1);
}

void releaseMutexLock(int sub_sema_index)
{
	struct sembuf sem_operator;

	sem_operator.sem_num = sub_sema_index;
	sem_operator.sem_op = +1;
	sem_operator.sem_flg = 0;

	semop(semID,&sem_operator,1);
}

void wait(int sub_sema_index)
{
	struct sembuf sem_operator;

	sem_operator.sem_num = sub_sema_index;
	sem_operator.sem_op = -1;
	sem_operator.sem_flg = 0; // blocking wait, if already 0

	semop(semID,&sem_operator,1);
}

void signal(int sub_sema_index)
{
	struct sembuf sem_operator;

	sem_operator.sem_num = sub_sema_index;
	sem_operator.sem_op = +1;
	sem_operator.sem_flg = 0;

	semop(semID,&sem_operator,1);
}

char rightDirectionOf(char myDirection)
{
	switch(myDirection)
	{
		case 'N': return 'W';
		case 'W': return 'S';
		case 'S': return 'E';
		case 'E': return 'N';
		default : 
		printf("Invalid Direction !\n");
		exit(0);
	}
}

void crossJunction()
{
	/*********** OWN DIRECTION SEMAPHORE *******************************/

	// trying to acquire lock on 'own direction' semaphore
	// update the Matrix File accordingly
	acquireMutexLock(MATRIX_MUTEX);
	updateMatrixFile(train_direction,1); // entry should be either 0,1 or 2
	releaseMutexLock(MATRIX_MUTEX);

	printf("Train %d: requests for %s-Lock\n",getpid(),getDirString(train_direction));

	// wait for 'own direction' semaphore
	wait(getDirectionSemaphore(train_direction));	


	// reached here --> lock acquired on 'own direction' semaphore
	// update the Matrix File accordingly
	acquireMutexLock(MATRIX_MUTEX);
	updateMatrixFile(train_direction,2); // entry should be either 0,1 or 2
	releaseMutexLock(MATRIX_MUTEX);

	printf("Train %d: acquires %s-Lock\n",getpid(),getDirString(train_direction));

	/****************************************************************/




	/*********** RIGHT DIRECTION SEMAPHORE ****************************/

	// trying to acquire lock on 'right direction' semaphore
	// update the Matrix File accordingly
	acquireMutexLock(MATRIX_MUTEX);
	updateMatrixFile(rightDirectionOf(train_direction),1); // entry should be either 0,1 or 2
	releaseMutexLock(MATRIX_MUTEX);

	printf("Train %d: requests for %s-Lock\n",getpid(),getDirString(rightDirectionOf(train_direction)));

	// wait for 'right direction' semaphore
	wait(getDirectionSemaphore(rightDirectionOf(train_direction)));	


	// reached here --> lock acquired on 'right direction' semaphore
	// update the Matrix File accordingly
	acquireMutexLock(MATRIX_MUTEX);
	updateMatrixFile(rightDirectionOf(train_direction),2); // entry should be either 0,1 or 2
	releaseMutexLock(MATRIX_MUTEX);

	printf("Train %d: acquires %s-Lock\n",getpid(),getDirString(rightDirectionOf(train_direction)));

	/****************************************************************/




	printf("Train %d: requests for Junction-Lock\n",getpid());
	acquireMutexLock(JUNCTION_MUTEX);
	printf("Train %d: acquires Junction-Lock; Passing Junction\n",getpid());
	
	sleep(JUNCTION_CROSSING_TIME);	// JUST for simulating Deadlock Conditions by prolonging the time of crossing the junction

	releaseMutexLock(JUNCTION_MUTEX);
	printf("Train %d: releases Junction-Lock\n",getpid());






	/*********** OWN DIRECTION SEMAPHORE *******************************/

	// trying to release lock on 'own direction' semaphore

	// signal 'own direction' semaphore
	signal(getDirectionSemaphore(train_direction));	


	// reached here --> lock released on 'own direction' semaphore
	// update the Matrix File accordingly
	acquireMutexLock(MATRIX_MUTEX);
	updateMatrixFile(train_direction,0); // entry should be either 0,1 or 2
	releaseMutexLock(MATRIX_MUTEX);
	printf("Train %d: releases %s-Lock\n",getpid(),getDirString(train_direction));

	/****************************************************************/




	/*********** RIGHT DIRECTION SEMAPHORE ****************************/

	// trying to release lock on 'right direction' semaphore

	// signal for 'right direction' semaphore
	signal(getDirectionSemaphore(rightDirectionOf(train_direction)));	


	// reached here --> lock released on 'right direction' semaphore
	// update the Matrix File accordingly
	acquireMutexLock(MATRIX_MUTEX);
	updateMatrixFile(rightDirectionOf(train_direction),0); // entry should be either 0,1 or 2
	releaseMutexLock(MATRIX_MUTEX);
	printf("Train %d: releases %s-Lock\n",getpid(),getDirString(rightDirectionOf(train_direction)));

	/****************************************************************/

	
}


void main(int argc,char* argv[])
{
	semID = semget((key_t)SEMAPHORE_KEY,6,IPC_CREAT|0666);
	
	if(argc!=4)
	{
		printf("Please run the program with 3 arguments : <num-of-trains(N)> <train_direction> <matrix_index> \n");
		exit(1);
	}
	else
	{
		sscanf(argv[1],"%d",&N);
		sscanf(argv[2],"%c",&train_direction);
		sscanf(argv[3],"%d",&matrix_index);
	}
	//printf("I, pid : %d hav been spawned by manager ! %d\n",getpid(), matrix_index );

	crossJunction();
}