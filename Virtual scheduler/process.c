#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <time.h>
#include <termios.h>
#include <string.h>

#define KEY 1234
#define CREATE 10
#define IO_END 20
#define SCHEDULER 30

int pauser = 0;

struct timeval res1, res2, turn1, turn2, wait1, wait2, tot_wait;

typedef struct mssgbuff 
{
    long mtype;
    int priority;   
    int pid;
} 
message_buf;

int connectQueue()
{
    int msqid;
    key_t key = KEY;  
    if ((msqid = msgget(key, 0666)) < 0) 
    {
        perror("msgget");
        exit(1);
    }
    else 
	printf("Successfully connected to msqid = %d\n", msqid);
    return msqid;
}

void notify()
{	
	printf("Notified\n");
}

void suspend()
{
	printf("Suspended\n");
	pauser = 1;
}

void process(int NOI, int priority, double sleep_probability, int sleep_time)
{
	
	srand(time(NULL));
	int i, msqid, pid, scheduler_pid;
	double r;
	message_buf sbuf, rbuf;

	signal(SIGUSR1, suspend);
	signal(SIGUSR2, notify);


	pid = getpid();
    	msqid = connectQueue();
	
	sbuf.mtype = CREATE;
	sbuf.pid = pid;
	sbuf.priority = priority;

	
	if(msgsnd(msqid, &sbuf, sizeof(message_buf) - sizeof(long), 0) < 0)
		perror("msgsnd error\n");
	else
		printf("Success!\n");

	gettimeofday(&res1, NULL);
	gettimeofday(&wait1, NULL);
	gettimeofday(&turn1, NULL);

	pause();

	gettimeofday(&res2, NULL);
	gettimeofday(&wait2, NULL);

	tot_wait.tv_sec = (wait2.tv_sec - wait1.tv_sec);
	tot_wait.tv_usec = (wait2.tv_usec - wait1.tv_usec);  	

	struct msqid_ds buffer;
	msgctl(msqid, IPC_STAT, &buffer);
	scheduler_pid = buffer.msg_lrpid;	

	for(i = 0; i < NOI; i++)
	{
		if(pauser == 1)
		{
			gettimeofday(&wait1, NULL);
			pause();
			gettimeofday(&wait2, NULL);
			tot_wait.tv_sec = tot_wait.tv_sec + (wait2.tv_sec - wait1.tv_sec);
			tot_wait.tv_usec = tot_wait.tv_usec + (wait2.tv_usec - wait1.tv_usec);  	
	
		}

		pauser = 0;

		printf("PID : %d Loop Counter : %d\n", pid, i);
		r = (double)rand() / (double)((double)RAND_MAX + 1);
		//r = 0.5;

		if(r < sleep_probability)
		{			
			kill(scheduler_pid, SIGUSR1);     //send IO signal to schedular

			printf("PID : %d Going for IO\n", pid);
			sleep(sleep_time);
			printf("PID : %d Came back from IO\n", pid);

			//inform schedular via MSG Q
			message_buf buf;
			buf.mtype = IO_END;
			buf.pid = pid;
			buf.priority = priority;
			if(msgsnd(msqid, &buf, sizeof(message_buf) - sizeof(long), 0) < 0)
				perror("msgsnd error\n");
			else
				printf("Success!\n");
			gettimeofday(&wait1, NULL);
			pause();
			gettimeofday(&wait2, NULL);
			tot_wait.tv_sec = tot_wait.tv_sec + (wait2.tv_sec - wait1.tv_sec);
			tot_wait.tv_usec = tot_wait.tv_usec + (wait2.tv_usec - wait1.tv_usec);  	
		}
		
	}	
	kill(scheduler_pid, SIGUSR2);
	gettimeofday(&turn2, NULL);
}

int main(int argc, char* argv[])
{
	int NOI = atoi(argv[1]);
	int priority = atoi(argv[2]);
	double sleep_probability = atof(argv[3]);
	int sleep_time = atoi(argv[4]);
	process(NOI, priority, sleep_probability, sleep_time);


	char c[20];
	sprintf(c, "%d", getpid());
	strcat(c,".txt");	
	FILE *	fptr = fopen(c , "w");

        if(fptr==NULL)
	{
		printf("Error Creating file!\n");
		exit(1);
        }

	fprintf(fptr,"Response Time for PID <%d> : seconds = %lf/useconds = %lf\n", getpid(), (double)(res2.tv_sec-res1.tv_sec), (double)(res2.tv_usec-res1.tv_usec));
	fprintf(fptr,"Turnaround Time for PID <%d> : seconds = %lf\n", getpid(), (double)(turn2.tv_sec-turn1.tv_sec));
	fprintf(fptr,"Total Wait Time for PID <%d> : seconds = %lf/useconds = %lf\n", getpid(), (double)(tot_wait.tv_sec), (double)(tot_wait.tv_usec));

	return 0;
}

