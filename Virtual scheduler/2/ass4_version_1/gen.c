/***************************************************************
Operating Systems Lab Assignment 4 : VIRTUAL SCHEDULER

Name : Kumar Ayush
Roll No. 13CS10058

Name : Varun Rawal
Roll No. 13CS10059

****************************************************************/

// Generator program : Generates N processes at regular intervals of t seconds

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_SIZE 1024

#define N 4
#define t 1

typedef struct process_type
{
	int prio,NOI,sleeptime;
	float sleepprob;
} proc_type;

proc_type CPU_bound = {10,2000,1,0.01};
proc_type IO_bound = {5,100,3,0.2};

void main()
{

	int i;

	for(i=0;i<N;i++)
	{
		int pid=fork();

		if(pid==0)
		{
			char command[50];
			if(i<2)
				sprintf(command,"./process %d %d %d %f",CPU_bound.prio,CPU_bound.NOI,CPU_bound.sleeptime,CPU_bound.sleepprob);
			else
				sprintf(command,"./process %d %d %d %f",IO_bound.prio,IO_bound.NOI,IO_bound.sleeptime,IO_bound.sleepprob);

			int rtv=execlp("xterm","xterm","-hold","-e",command,(const char*)NULL);

   			if(rtv==-1)
   				error("Error in executing xterm !\n\n");

   			exit(1);
		}

		sleep(t);	// sleep for t seconds before generating another process
	}
}