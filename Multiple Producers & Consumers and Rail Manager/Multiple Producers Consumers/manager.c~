/***************************************************************
Operating Systems Lab Assignment 5 : MULTIPLE PRODUCERS AND CONSUMERS

Name : Kumar Ayush
Roll No. 13CS10058

Name : Varun Rawal
Roll No. 13CS10059

****************************************************************/

// Manager Program : creates 5 Producer processes and 5 Consumer Processes

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

#include <termios.h>
#include <ctype.h>

static struct termios old;
static struct termios _new;


/* Initialize _new terminal i/o settings */
void initTermios(int echo) 
{
  tcgetattr(0, &old); /* grab old terminal i/o settings */
  _new = old; /* make _new settings same as old settings */
  _new.c_lflag &= ~ICANON; /* disable buffered i/o */
  _new.c_lflag &= echo ? ECHO : ~ECHO; /* set echo mode */
  tcsetattr(0, TCSANOW, &_new); /* use these _new terminal i/o settings now */
}

/* Restore old terminal i/o settings */


void resetTermios(void) 
{
  tcsetattr(0, TCSANOW, &old);
}

/* Read 1 character - echo defines echo mode */
char getch_(int echo) 
{
  char ch;
  initTermios(echo);
  ch = getchar();
  resetTermios();
  return ch;
}

/* Read 1 character without echo */
char getch(void) 
{
  return getch_(0);
}

/* Read 1 character with echo */
char getche(void) 
{
  return getch_(1);
}
/*

INITIALIZATION:

	shared binary semaphore mutex = 1;
	shared counting semaphore empty = MAX;
	shared counting semaphore full = 0;
	shared anytype buffer[MAX];
	shared int in, out, count;


PRODUCER :

	anytype item;
	
	// here, P() --> sleepOn() /wait()  and V() --> release() / signal()

	repeat {

		// produce something 
		item = produce();

		// wait for an empty space 
		P(empty);

		// store the item 
		P(mutex);
		buffer[in] = item;
		in = in + 1 mod MAX;
		count = count + 1;
		V(mutex);

		// report the new full slot 
		V(full);

	} until done;




CONSUMER:

	anytype item;

	repeat {

		// wait for a stored item
		P(full);

		// remove the item 
		P(mutex);
		item = buffer[out];
		out = out + 1 mod MAX;
		count = count - 1;
		V(mutex);

		// report the new empty slot 
		V(empty);

		// consume it 
		consume(item);

	} until done;

	*/

// IMPLEMENTATION using LINUX/POSIX Sempahores

// the "shared semaphore" word in the above theory is implemented by extracting the sempahores array using a common key_t among different using processes 
// the "shared buffer and items" word in the above theory is implemented by extracting the message queue using a common key_t among different using processes 
// also a record file "matrix.txt" is a shared one

// So, 3 mutex locks required : one for each of the Queues, one for the matrix.txt file

// semaphore concepts :

// A semaphore S = +1 i.e. it is available
// A semaphore S = 0 i.e. it is NOT available, and any processes wanting to use it will be blocked (forced to sleep) on it, until the semaphore is again AVAILABLE (+1)

// I want to use semaphore S -->  use S with sem-op = -1
// I want to make S available --> use S with sem-op = +1

#define MATRIX_FILENAME "matrix.txt"
#define NUM_PRODUCERS 5
#define NUM_CONSUMERS 5
#define NUM_QUEUES 2
#define MSSG_Q0_KEY 10
#define MSSG_Q1_KEY 20

#define SEMAPHORE_KEY 10

// these are the indices of the sub-semaphores to be used as different mutex locks
#define MATRIX_MUTEX 0
#define QUEUE_0_MUTEX 1
#define QUEUE_1_MUTEX 2
#define EMPTY_SLOT_COUNTER_Q0 3
#define FULL_SLOT_COUNTER_Q0 4

#define EMPTY_SLOT_COUNTER_Q1 5
#define FULL_SLOT_COUNTER_Q1 6

#define MAX_SIZE 4

int semID;

struct mssg // message template for the message Queue
{
	long mtype;
	char mtext[MAX_SIZE];
};

int MAX_SLOTS = 10;


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



void initializeAllSubSemaphoreValues()
{
	semctl(semID,MATRIX_MUTEX,SETVAL,1);
	semctl(semID,QUEUE_0_MUTEX,SETVAL,1);
	semctl(semID,QUEUE_1_MUTEX,SETVAL,1);

	semctl(semID,EMPTY_SLOT_COUNTER_Q0,SETVAL,MAX_SLOTS);
	semctl(semID,FULL_SLOT_COUNTER_Q0,SETVAL,0);

	semctl(semID,EMPTY_SLOT_COUNTER_Q1,SETVAL,MAX_SLOTS);
	semctl(semID,FULL_SLOT_COUNTER_Q1,SETVAL,0);
}
//semctl(semID,0,GETVAL,0);

void initializeMatrixFile()
{
	/*
		The matrix will be in following format :

				P1 P2 P3 P4 P5 C1 C2 C3 C4 C5
		Q0 : 	-   -  -  -  -  -  -  -  -  - 
		Q1 :	-   -  -  -  -  -  -  -  -  - 
	*/

	int matrix[NUM_QUEUES][NUM_PRODUCERS+NUM_CONSUMERS];

	int i,j;

	for(i=0;i<NUM_QUEUES;i++)
	{
		for(j=0;j<NUM_PRODUCERS + NUM_CONSUMERS;j++)
			matrix[i][j] = 0;
//		printf("\n");
	}

	FILE *fp = fopen(MATRIX_FILENAME, "w+");

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

int checkForDeadlock(int* m, int* n)
{
	int matrix[NUM_QUEUES][NUM_PRODUCERS+NUM_CONSUMERS];

	FILE *fp = fopen(MATRIX_FILENAME, "r+");

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
	}
	fclose(fp);

	int flag1 = 0,flag2 = 0;

	for(i=0;i<NUM_PRODUCERS+NUM_CONSUMERS;i++)
	{
		if(matrix[0][i] == 2 && matrix[1][i] == 1)
		{
			flag1 = 1;
			*m= i - NUM_PRODUCERS;
			if(*m < 0)
			{
				printf("Error ! Producers can't be requiring 2 resources @ once !\n");
				exit(1);
			}
			break;
		}
	}
	if(flag1)
	{
		for(i=0;i<NUM_PRODUCERS+NUM_CONSUMERS;i++)
		{
			if(matrix[0][i] == 1 && matrix[1][i] == 2)
			{
				flag2 = 1;
				*n = i - NUM_PRODUCERS;
				if(*n < 0)
				{
					printf("Error ! Producers can't be requiring 2 resources @ once !\n");
					exit(1);
				}
				break;
			}
		}
	}
	else
		return 0;

	if(flag1 && flag2)
		return 1;
	else
		return 0;


}

void appendResults(float probability)
{
	int insert_counter_total,delete_counter_total,insert_counter,delete_counter;
	FILE *fp;

	int x;
	for(x=0;x<NUM_PRODUCERS;x++)
	{
		char filename[50];
		strcpy(filename,"insert_temp_");

		char index_str[5];
		sprintf(index_str,"%d",x);

		strcat(filename,index_str);
		strcat(filename,".txt");

		fp = fopen(filename,"r+");
		fscanf(fp,"%d",&insert_counter);
		insert_counter_total += insert_counter;
		fclose(fp);

	}
	
	system("rm insert_temp_*");

	for(x=0;x<NUM_CONSUMERS;x++)
	{
		char filename[50];
		strcpy(filename,"delete_temp_");

		char index_str[5];
		sprintf(index_str,"%d",x+NUM_PRODUCERS);

		strcat(filename,index_str);
		strcat(filename,".txt");

		fp = fopen(filename,"r+");
		fscanf(fp,"%d",&delete_counter);
		delete_counter_total += delete_counter;
		fclose(fp);

	}
	
	system("rm delete_temp_*");

	fp = fopen("results.txt", "a+");
	fprintf(fp, "\n\nProbability : %f\n", probability);	
	fprintf(fp, "Total Inserts : %d\n", insert_counter_total);
	fprintf(fp, "Total Deletes : %d\n", delete_counter_total);
	fclose(fp);
}

void killAllProcesses()
{
	system("pkill xterm");
}

void main(int argc,char* argv[])
{
	int DPP;
	float probability = 0.2;

	if(argc!=3)
	{
		printf("Please run the program with 2 arguments : <DPP_toggle (0 or 1)> <probability> \n"); // DPP --> Deadlock Prevention Protocol
		exit(1);
	}
	else
	{
		sscanf(argv[1],"%d",&DPP);
		sscanf(argv[2],"%f",&probability);
		if((DPP != 0 && DPP != 1)||(probability<0 || probability >1))
		{
			printf("Please run the program with valid arguments : <DPP_toggle (0 or 1)> <probability>\n"); // DPP --> Deadlock Prevention Protocol
			exit(1);
		}
	}

	semID = semget((key_t)SEMAPHORE_KEY,7,IPC_CREAT|0666);

	initializeAllSubSemaphoreValues();

	acquireMutexLock(MATRIX_MUTEX);
	initializeMatrixFile();
	releaseMutexLock(MATRIX_MUTEX);

	int mssgQ0_ID=msgget((key_t)MSSG_Q0_KEY,IPC_CREAT|0666);
	int mssgQ1_ID=msgget((key_t)MSSG_Q1_KEY,IPC_CREAT|0666);

	int i;
	for(i=0;i<NUM_PRODUCERS;i++)
	{
		char command[50];
		sprintf(command,"xterm -hold -e ./producer %d %d %d &",MSSG_Q0_KEY,MSSG_Q1_KEY,i);
		system(command);
		sleep(1);	// otherwise the item to be first inserted becomes the same for all the Queues due to the timeseeding : srand(time(NULL)) 
	}

	for(i=0;i<NUM_CONSUMERS;i++)
	{
		char command[50];
		sprintf(command,"xterm -hold -e ./consumer %d %d %d %f %d &",MSSG_Q0_KEY,MSSG_Q1_KEY,NUM_PRODUCERS+i,probability,DPP);
		system(command);
	}


	while(1)
	{
		int m,n;	// if Deadlock is found, then Consumer m (Cm) and Consumer n (Cn) are such that Q0 --> Cm --> Q1 --> Cn --> Q0, in the Resource Allocation Graph 

		int found_deadlock = 0;
		printf("Checking for Deadlock...\n");

		acquireMutexLock(MATRIX_MUTEX);
		found_deadlock = checkForDeadlock(&m,&n);
		releaseMutexLock(MATRIX_MUTEX);

		if(found_deadlock)
		{
			printf("\n\nDeadlock !!!\n\nDeadlocked Cycle : Q0 --> C%d --> Q1 --> C%d --> Q0 \n\nKilling all Processes...",m,n);
			printf("\n\nPress a key to kill all Processes\n");
			getch();
			killAllProcesses();
			appendResults(probability);
			exit(0);
		}
		sleep(2);
	}
}