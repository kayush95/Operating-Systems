/***************************************************************
Operating Systems Lab Assignment 5 : Rail Manager

Name : Kumar Ayush
Roll No. 13CS10058

Name : Varun Rawal
Roll No. 13CS10059

****************************************************************/

// Manager Program : creates and ensures Synchronization between trains crossing the '#' Junction 

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


// IMPLEMENTATION using LINUX/POSIX Sempahores

// the "shared semaphore" word in the above theory is implemented by extracting the sempahores array using a common key_t among different using processes 
// also a record file "matrix.txt" is a shared one

// semaphore concepts :

// A semaphore S = +1 i.e. it is available
// A semaphore S = 0 i.e. it is NOT available, and any processes wanting to use it will be blocked (forced to sleep) on it, until the semaphore is again AVAILABLE (+1)

// I want to use semaphore S -->  use S with sem-op = -1
// I want to make S available --> use S with sem-op = +1

#define MATRIX_FILENAME "matrix.txt"
#define INPUT_FILE "sequence.txt"

#define SEMAPHORE_KEY 10

// these are the indices of the sub-semaphores to be used as different mutex locks
#define MATRIX_MUTEX 0
#define JUNCTION_MUTEX 1

#define NORTH_SEMAPHORE 2
#define WEST_SEMAPHORE 3
#define SOUTH_SEMAPHORE 4
#define EAST_SEMAPHORE 5

#define MAX_SEQUENCE_LENGTH 100

int semID;

int pid_arr[MAX_SEQUENCE_LENGTH];
char* direction_arr[MAX_SEQUENCE_LENGTH];


	
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
	semctl(semID,JUNCTION_MUTEX,SETVAL,1);

	semctl(semID,NORTH_SEMAPHORE,SETVAL,1);
	semctl(semID,WEST_SEMAPHORE,SETVAL,1);
	semctl(semID,SOUTH_SEMAPHORE,SETVAL,1);
	semctl(semID,EAST_SEMAPHORE,SETVAL,1);
}
//semctl(semID,0,GETVAL,0);

void initializeMatrixFile(int N,int M)	// for writing NxM matrix
{
	acquireMutexLock(MATRIX_MUTEX);
	/*
		The matrix will be in following format :

				N 	W  S  E 
		t0 : 	-   -  -  -  
		t1 :	-   -  -  -  
		|
		|
		|
	*/

	int matrix[N][M];

	int i,j;

	for(i=0;i<N;i++)
	{
		for(j=0;j<M;j++)
			matrix[i][j] = 0;
//		printf("\n");
	}

	FILE *fp = fopen(MATRIX_FILENAME, "w+");

	for(i=0;i< N ;i++)
	{
		for(j=0;j< M ;j++)
		{
			fprintf(fp, "%d ", matrix[i][j]);
		}

		fprintf(fp, "\n");
	}

	fclose(fp);

	releaseMutexLock(MATRIX_MUTEX);

}


void killAllProcesses()
{
	system("pkill train");
}

int checkForDeadlock(int N,int M)
{
	acquireMutexLock(MATRIX_MUTEX);

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
	releaseMutexLock(MATRIX_MUTEX);

	int pos[N];
	for(i=0;i<N;i++)pos[i]=-1;
	int flag=-1;

	printf("\nChecking for Deadlock...\n\n");

	j=0;
	int k=0;
	int found=0;
	while(k<N && !found)
	{
		if(flag==-1)j=0;
		for(i=0;i<N;i++)
			if(matrix[i][j]==2) {pos[k]=i; break;}
		if(pos[k]==-1) return 0;
		if(pos[k]==pos[0] && k!=0) {found=1; break;}
		flag=-1;
		for(j=0;j<M;j++)
			if(matrix[pos[k]][j]==1) {flag=j; break;}
		if(flag==-1) return 0;
		k++;
	}


	if(found)
	{
		printf("\n\nDeadlock !!!\n\nDeadlocked Cycle : \n");
		//printf("k = %d\n",k );
		for(i=0;i<k;i++)
				printf("Train %d from %s is waiting for Train %d from %s\n",pid_arr[pos[i]], direction_arr[pos[i]] ,pid_arr[pos[i+1]] , direction_arr[pos[i+1]] );
		
		printf("\n\nPress a key to kill all Processes and exit !\n");
		getch();
		killAllProcesses();
		//appendResults(probability);
		return 1;
	}
	else return 0;

}


int parseInputFile(char* s)
{
	FILE* fp = fopen(INPUT_FILE, "r");
	if(fp==NULL)return -1;
	fscanf(fp,"%s",s);
	fclose(fp);
	return strlen(s);
}

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

void main(int argc,char* argv[])
{
	semID = semget((key_t)SEMAPHORE_KEY,6,IPC_CREAT|0666);
	float probability;

	if(argc!=2)
	{
		printf("Please provide probability as argument ! \n");
		exit(1);
	}
	else
	{
		sscanf(argv[1],"%f",&probability);
		if(probability < 0 || probability > 1)
		{
			printf("Please run the program with valid probability value : [0-1] \n");
			exit(1);
		}
	}

	char train_sequence[MAX_SEQUENCE_LENGTH];

	int N,M=4;

	if((N=parseInputFile(train_sequence))==-1)
	{
		printf("Error : Cannot open sequence.txt !\n");
		exit(0);
	}

	printf("The sequence scanned from file : %s, N = %d\n",train_sequence,N);

	initializeAllSubSemaphoreValues();

	initializeMatrixFile(N,M);

	srand(time(NULL));

	int sequence_counter = 0,flag=0;

	while(1)
	{
		if(flag)
		{
			sleep(1);
			if(checkForDeadlock(N,M))
				break;
			continue;
		}

		if( ( (float)(rand()%1000) / 1000.0) < probability )
		{
			if(checkForDeadlock(N,M))
				break;
		}
		else
		{
			if(sequence_counter==N)
				flag = 1;
			else
			{
				char c = train_sequence[sequence_counter];
				sequence_counter++;
				if(c!='N' && c!='S' && c!='W' && c!='E')
				{
					printf("Invalid Sequence in sequence.txt !\n");
					exit(1);
				}
				else
				{
					/*
					char command[50];
					sprintf(command,"./train %c %d &",c,sequence_counter-1);
					pid_arr[sequence_counter-1] = pid;
					direction_arr[sequence_counter-1] = getDirString(c);
					system(command);
					*/
					
					int pid = fork();
					if(pid == 0)
					{
						//char command[50];
						//sprintf(command,"./train %d %c %d",N,c,sequence_counter-1);
						
						char** argv = malloc(4*sizeof(char*));
						int y;
						for(y=0;y<4;y++) argv[y] = malloc(10*sizeof(char));
						strcpy(argv[0],"./train");
						sprintf(argv[1],"%d",N);
						sprintf(argv[2],"%c",c);
						sprintf(argv[3],"%d",sequence_counter-1);

						int retv = execvp(*argv,argv);
						if(retv == -1) {perror("Error in execvp ! \n"); exit(1);}
						exit(0);
					}
					printf("Train %d: %s Train started\n",pid,getDirString(c));
					pid_arr[sequence_counter-1] = pid;
					//printf("pid_arr[%d] = %d\n",sequence_counter-1,pid );
					direction_arr[sequence_counter-1] = getDirString(c);
					//printf("direction_arr[%d] = %s\n",sequence_counter-1,getDirString(c) );
					
				}
			}
			
		}
	}

}
