/***************************************************************
Operating Systems Lab Assignment 5 : MULTIPLE PRODUCERS AND CONSUMERS

Name : Kumar Ayush
Roll No. 13CS10058

Name : Varun Rawal
Roll No. 13CS10059

****************************************************************/

// Consumer Program

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/sem.h>

#define MAX_SIZE 4

#define MATRIX_FILENAME "matrix.txt"

#define NUM_PRODUCERS 5
#define NUM_CONSUMERS 5
#define NUM_QUEUES 2

#define SEMAPHORE_KEY 10

// these are the indices of the sub-semaphores to be used as different mutex locks
#define MATRIX_MUTEX 0
#define QUEUE_0_MUTEX 1
#define QUEUE_1_MUTEX 2
#define EMPTY_SLOT_COUNTER_Q0 3
#define FULL_SLOT_COUNTER_Q0 4

#define EMPTY_SLOT_COUNTER_Q1 5
#define FULL_SLOT_COUNTER_Q1 6

#define CONSUMER_RECEIVE_CODE 100

#define MIN_RESOURCE_ACQUIRE_TIME 2 // in seconds

int semID;
int mssgQ0_ID,mssgQ1_ID;
int matrix_index;
int delete_counter = 0;

struct mssg // message template for the message Queue
{
	long mtype;
	char mtext[MAX_SIZE];
};

// semaphore concepts :

// A semaphore S = +1 i.e. it is available
// A semaphore S = 0 i.e. it is NOT available, and any processes wanting to use it will be blocked (forced to sleep) on it, until the semaphore is again AVAILABLE (+1)

// I want to use semaphore S -->  use S with sem-op = -1
// I want to make S available --> use S with sem-op = +1

void updateMatrixFile(int queue_index,int entry)
{
	/*
		The matrix will be in following format :

				P1 P2 P3 P4 P5 C1 C2 C3 C4 C5
		Q0 : 	-   -  -  -  -  -  -  -  -  - 
		Q1 :	-   -  -  -  -  -  -  -  -  - 
	*/
	int matrix[NUM_QUEUES][NUM_PRODUCERS+NUM_CONSUMERS];

	FILE *fp = fopen(MATRIX_FILENAME, "r+");

	printf("\n\n");

	int i,j;
	for(i=0;i< NUM_QUEUES ;i++)
	{
		for(j=0;j< NUM_PRODUCERS ;j++)
		{
			fscanf(fp, "%d", &matrix[i][j]);
		}

		for(j=0;j< NUM_CONSUMERS ;j++)
		{
			fscanf(fp, "%d", &matrix[i][NUM_PRODUCERS+j]);
		}

		//char _n;
		//fscanf(fp, "%c", &_n); // for consuming \n
	}
/*
	for(i=0;i<NUM_QUEUES;i++)
	{
		for(j=0;j<10;j++)
			printf("%d ",matrix[i][j] );
		printf("\n");
	}
*/
	fclose(fp);

	//int myindex = getpid() - getppid() - 1;
	//printf("Before : %d\n",matrix[queue_index][matrix_index] );
	matrix[queue_index][matrix_index] = entry;	
	//printf("After : %d\n",matrix[queue_index][matrix_index] );

	//printf("my index : %d \n",myindex );
	//printf("matrix index : %d\n",matrix_index );

	
/*
	printf("\nchanged to\n\n");

	for(i=0;i<NUM_QUEUES;i++)
	{
		for(j=0;j<10;j++)
			printf("%d ",matrix[i][j] );
		printf("\n");
	}
*/
	// reading till now... now writing

	fp = fopen(MATRIX_FILENAME, "w+");

	for(i=0;i< NUM_QUEUES ;i++)
	{
		for(j=0;j< NUM_PRODUCERS ;j++)
		{
			fprintf(fp, "%d ", matrix[i][j]);
		}

		for(j=0;j< NUM_CONSUMERS ;j++)
		{
			fprintf(fp, "%d ", matrix[i][NUM_PRODUCERS+j]);
		}

		fprintf(fp, "\n");
	}


	fclose(fp);

}

void writeCountertofile()
{
	char filename[50];
	strcpy(filename,"delete_temp_");

	char index_str[5];
	sprintf(index_str,"%d",matrix_index);

	strcat(filename,index_str);
	strcat(filename,".txt");

	FILE* fp = fopen(filename, "w+");
	fprintf(fp, "%d", delete_counter);	
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

void down(int sub_sema_index)
{
	struct sembuf sem_operator;

	sem_operator.sem_num = sub_sema_index;
	sem_operator.sem_op = -1;
	sem_operator.sem_flg = 0; // blocking wait, if already 0

	semop(semID,&sem_operator,1);
}

void up(int sub_sema_index)
{
	struct sembuf sem_operator;

	sem_operator.sem_num = sub_sema_index;
	sem_operator.sem_op = +1;
	sem_operator.sem_flg = 0;

	semop(semID,&sem_operator,1);
}

void consumeFromQueue(int queue_index)
{
	// trying to acquire lock
	// update the Matrix File accordingly
	acquireMutexLock(MATRIX_MUTEX);
	updateMatrixFile(queue_index,1); // entry should be either 0,1 or 2
	releaseMutexLock(MATRIX_MUTEX);

	// wait for a full slot if none exists
	int FULL_SLOT_COUNTER = queue_index==0 ? FULL_SLOT_COUNTER_Q0 : FULL_SLOT_COUNTER_Q1;
	down(FULL_SLOT_COUNTER);	

	int mssgQ_ID = queue_index==0 ? mssgQ0_ID : mssgQ1_ID;

	if(queue_index==0)
		acquireMutexLock(QUEUE_0_MUTEX);
	else if(queue_index==1)
		acquireMutexLock(QUEUE_1_MUTEX);
	else
	{
		printf("Impossible Error !\n");
		exit(1);
	}

	// reached here --> lock acquired
	// update the Matrix File accordingly
	acquireMutexLock(MATRIX_MUTEX);
	updateMatrixFile(queue_index,2); // entry should be either 0,1 or 2
	releaseMutexLock(MATRIX_MUTEX);






	// consuming a message item from the given Message Queue
	struct mssg buffer;

	int rcvVal = msgrcv(mssgQ_ID,&buffer,sizeof(buffer),CONSUMER_RECEIVE_CODE,0); // blocking receive 
	if(rcvVal == -1)
	{
		perror("Error in mssg Q \n");
		exit(0);
	}
	else 
	{
		int consumed;
		sscanf(buffer.mtext,"%d",&consumed); // extract item from the message
		printf("PID %d succesfully consumed %d from Queue %d\n",getpid(),consumed,queue_index);
	}
	delete_counter++;
	writeCountertofile();

	
	sleep(MIN_RESOURCE_ACQUIRE_TIME);	// JUST for simulating Deadlock Conditions by prolonging the time of resource acquire




	if(queue_index==0)
		releaseMutexLock(QUEUE_0_MUTEX);
	else if(queue_index==1)
		releaseMutexLock(QUEUE_1_MUTEX);
	else
	{
		printf("Impossible Error !\n");
		exit(1);
	}

	// lock released 
	// update the Matrix File accordingly
	acquireMutexLock(MATRIX_MUTEX);
	updateMatrixFile(queue_index,0); // entry should be either 0,1 or 2
	releaseMutexLock(MATRIX_MUTEX);

	int EMPTY_SLOT_COUNTER = queue_index==0 ? EMPTY_SLOT_COUNTER_Q0 : EMPTY_SLOT_COUNTER_Q1;
	up(EMPTY_SLOT_COUNTER);

	
}


void consumeFromBothQueues(int queue_index_1 , int queue_index_2)
{
	/*********************** REQUEST LOCK ON FIRST QUEUE *************************************************/

	// trying to acquire lock
	// update the Matrix File accordingly
	acquireMutexLock(MATRIX_MUTEX);
	updateMatrixFile(queue_index_1,1); // entry should be either 0,1 or 2
	releaseMutexLock(MATRIX_MUTEX);


	// wait for a full slot if none exists
	int FULL_SLOT_COUNTER = queue_index_1==0 ? FULL_SLOT_COUNTER_Q0 : FULL_SLOT_COUNTER_Q1;
	down(FULL_SLOT_COUNTER);	

	int mssgQ_ID = queue_index_1==0 ? mssgQ0_ID : mssgQ1_ID;

	//printf("Going 4 1st lock...\n");

	if(queue_index_1==0)
		acquireMutexLock(QUEUE_0_MUTEX);
	else if(queue_index_1==1)
		acquireMutexLock(QUEUE_1_MUTEX);
	else
	{
		printf("Impossible Error !\n");
		exit(1);
	}

	// reached here --> lock acquired
	// update the Matrix File accordingly
	acquireMutexLock(MATRIX_MUTEX);
	updateMatrixFile(queue_index_1,2); // entry should be either 0,1 or 2
	releaseMutexLock(MATRIX_MUTEX);

	/*****************************************************************************************************/




	/*********************** CONSUME FROM FIRST QUEUE *************************************************/

	// consuming a message item from the given Message Queue
	struct mssg buffer;

	int rcvVal = msgrcv(mssgQ_ID,&buffer,sizeof(buffer),CONSUMER_RECEIVE_CODE,0); // blocking receive 
	if(rcvVal == -1)
	{
		perror("Error in mssg Q \n");
		exit(0);
	}
	else 
	{
		int consumed;
		sscanf(buffer.mtext,"%d",&consumed); // extract item from the message
		printf("PID %d succesfully consumed %d from First Queue : Q%d\n",getpid(),consumed,queue_index_1);
		delete_counter++;
		writeCountertofile();
	}
	/*****************************************************************************************************/
	



	/*********************** REQUEST LOCK ON SECOND QUEUE *************************************************/

	// trying to acquire lock
	// update the Matrix File accordingly
	acquireMutexLock(MATRIX_MUTEX);
	updateMatrixFile(queue_index_2,1); // entry should be either 0,1 or 2
	releaseMutexLock(MATRIX_MUTEX);


	// wait for a full slot if none exists
	FULL_SLOT_COUNTER = queue_index_2==0 ? FULL_SLOT_COUNTER_Q0 : FULL_SLOT_COUNTER_Q1;
	down(FULL_SLOT_COUNTER);	

	//printf("Reached @ 1\n");
	mssgQ_ID = queue_index_2==0 ? mssgQ0_ID : mssgQ1_ID;

	//printf("Going 4 2nd lock...\n");

	if(queue_index_2==0)
		acquireMutexLock(QUEUE_0_MUTEX);
	else if(queue_index_2==1)
		acquireMutexLock(QUEUE_1_MUTEX);
	else
	{
		printf("Impossible Error !\n");
		exit(1);
	}

	// reached here --> lock acquired
	// update the Matrix File accordingly
	acquireMutexLock(MATRIX_MUTEX);
	updateMatrixFile(queue_index_2,2); // entry should be either 0,1 or 2
	releaseMutexLock(MATRIX_MUTEX);

	/*****************************************************************************************************/




	/*********************** CONSUME FROM SECOND QUEUE *************************************************/

	// consuming a message item from the given Message Queue
	//struct mssg buffer;

	rcvVal = msgrcv(mssgQ_ID,&buffer,sizeof(buffer),CONSUMER_RECEIVE_CODE,0); // blocking receive 
	if(rcvVal == -1)
	{
		perror("Error in mssg Q \n");
		exit(0);
	}
	else 
	{
		int consumed;
		sscanf(buffer.mtext,"%d",&consumed); // extract item from the message
		printf("PID %d succesfully consumed %d from Second Queue : Q%d\n",getpid(),consumed,queue_index_2);
		delete_counter++;
		writeCountertofile();
	}
	/*****************************************************************************************************/



	
	sleep(MIN_RESOURCE_ACQUIRE_TIME);	// JUST for simulating Deadlock Conditions by prolonging the time of resource acquire



	/*********************** RELEASE LOCK ON FIRST QUEUE ***************/
	if(queue_index_1==0)
		releaseMutexLock(QUEUE_0_MUTEX);
	else if(queue_index_1==1)
		releaseMutexLock(QUEUE_1_MUTEX);
	else
	{
		printf("Impossible Error !\n");
		exit(1);
	}

	// lock released 
	// update the Matrix File accordingly
	acquireMutexLock(MATRIX_MUTEX);
	updateMatrixFile(queue_index_1,0); // entry should be either 0,1 or 2
	releaseMutexLock(MATRIX_MUTEX);

	int EMPTY_SLOT_COUNTER = queue_index_1==0 ? EMPTY_SLOT_COUNTER_Q0 : EMPTY_SLOT_COUNTER_Q1;
	up(EMPTY_SLOT_COUNTER);
	/*********************************************************************/



	/*********************** RELEASE LOCK ON SECOND QUEUE ***************/
	if(queue_index_2==0)
		releaseMutexLock(QUEUE_0_MUTEX);
	else if(queue_index_2==1)
		releaseMutexLock(QUEUE_1_MUTEX);
	else
	{
		printf("Impossible Error !\n");
		exit(1);
	}

	// lock released 
	// update the Matrix File accordingly
	acquireMutexLock(MATRIX_MUTEX);
	updateMatrixFile(queue_index_2,0); // entry should be either 0,1 or 2
	releaseMutexLock(MATRIX_MUTEX);

	EMPTY_SLOT_COUNTER = queue_index_2==0 ? EMPTY_SLOT_COUNTER_Q0 : EMPTY_SLOT_COUNTER_Q1;
	up(EMPTY_SLOT_COUNTER);
	/*********************************************************************/
	
}


void main(int argc,char* argv[])
{
	int DPP; // Deadlock Prevention Protocol = 1 (DPP is followed) / = 0 (not followed)
	float p; // probability

	semID = semget((key_t)SEMAPHORE_KEY,7,IPC_CREAT|0666);

	int MSSG_Q0_KEY,MSSG_Q1_KEY;
	
	if(argc!=6)
	{
		printf("Please run the program with 5 arguments : <MSSG_Q0_KEY> <MSSG_Q1_KEY> <probability> <matrix_index> <DPP_toggle>\n");
		exit(1);
	}
	else
	{
		sscanf(argv[1],"%d",&MSSG_Q0_KEY);
		sscanf(argv[2],"%d",&MSSG_Q1_KEY);
		sscanf(argv[3],"%d",&matrix_index);
		sscanf(argv[4],"%f",&p);
		sscanf(argv[5],"%d",&DPP);
	}

	mssgQ0_ID=msgget((key_t)MSSG_Q0_KEY,IPC_CREAT|0666);
	mssgQ1_ID=msgget((key_t)MSSG_Q1_KEY,IPC_CREAT|0666);

	srand(time(NULL));

	while(1)
	{
		int random_integer = rand()%1000;

		float prob = (float)random_integer / 1000.0;

		if(prob < p) // where p is as is mentioned in the Assignment Problem
		{
			int random_queue = rand()%2;

			printf("PID %d trying to consume from Queue %d\n",getpid(),random_queue );
			
			consumeFromQueue(random_queue);
		}
		else if(prob >= 1-p )
		{
			int queue_first,queue_second;

			if(DPP == 0)
			{
				//  NO deadlock avoidance scheme...
				queue_first = rand()%2;
				queue_second = 1 - queue_first;
			}
			else if(DPP == 1)
			{
				//  NO deadlock avoidance scheme...
				queue_first = 0;
				queue_second = 1;
			}
			else
			{
				printf("Error ! value of DPP neither 0 nor 1 !\n");
				exit(1);
			}


			printf("PID %d trying to consume from both the Queues %d and %d\n",getpid(),queue_first,queue_second);
			
			consumeFromBothQueues(queue_first,queue_second);
		}

		

	/*	int random_sleep_time = rand()%5 + 1; // sleeps randomly for 1 - 5 seconds

		sleep(random_sleep_time);
	*/

	}
}