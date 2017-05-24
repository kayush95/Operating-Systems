/***************************************************************
Operating Systems Lab Assignment 2 : Problem 1

Name : Kumar Ayush
Roll No. 13CS10058

Name : Varun Rawal
Roll No. 13CS10059

****************************************************************/

// DISTRIBUTED PRIME GENERATION

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

#define AVAILABLE SIGUSR1
#define BUSY SIGUSR2

int insert(int,int*,int);

void signalSender(int,int,int);
void signalHandler(int,siginfo_t*,void*);
void parentFunction(int,int,int**,int**,int*);
void childFunction(int,int,int);

int child_status[MAX_SIZE],parent_PID;

void main(int argc,char* argv[]) 	// main's running process is the parent process
{


	/* INSTALL THE SIGNAL_ACTION HANDLER FUNCTION***/
    struct sigaction siga;
    siga.sa_sigaction = signalHandler;
    siga.sa_flags = SA_SIGINFO; // get detailed info

    
    //sigemptyset(&siga.sa_mask);

    // change signal action,
    if(sigaction(AVAILABLE, &siga, NULL) == -1) {
        printf("error in sigaction()");
        return;
    }
    if(sigaction(BUSY, &siga, NULL) == -1) {
        printf("error in sigaction()");
        return;
    }



	parent_PID=getpid();

	int N,K;	// N = no. of primes, K = no. of child processes

	if(argc!=3)
	{
		printf("ERROR : Please provide 2 arguments N and K\nTerminating...\n");
		exit(1);
	}

	sscanf(argv[1],"%d",&N);
	sscanf(argv[2],"%d",&K);

	int i;

	for(i=0;i<K;i++)
		child_status[i]=AVAILABLE;

	int** fd_c2p;
	int** fd_p2c;

	fd_c2p=(int**)malloc(K*sizeof(int[2]));	// child to parent
	fd_p2c=(int**)malloc(K*sizeof(int[2]));	// perent to child

	for(i=0;i<K;i++){
		fd_c2p[i]=(int*)malloc(2*sizeof(int));
		fd_p2c[i]=(int*)malloc(2*sizeof(int));
	}

	int* pid=(int*)malloc(K*sizeof(int));

	for(i=0;i<K;i++)
	{
		pipe(fd_p2c[i]);	// to ith child
		pipe(fd_c2p[i]);	// from ith child
		
		if((pid[i]=fork())==0)
		{
			// child process
			close(fd_c2p[i][0]);	// child won't use the Read END of child-to-parent pipe
			close(fd_p2c[i][1]);	// child won't use the Write END of parent-to-child pipe
			childFunction(K,fd_c2p[i][1],fd_p2c[i][0]);
		}

		close(fd_c2p[i][1]);	// parent won't use the Write END of child-to-parent pipe
		close(fd_p2c[i][0]);	// parent won't use the Read END of parent-to-child pipe
			
	}

	printf("Calling parent FUNCTION...\n");

	parentFunction(K,N,fd_p2c,fd_c2p,pid);

	printf("\n\nParent exiting ....\n");
	free(fd_c2p);
	free(fd_p2c);
}

void childFunction(int K,int fd_write,int fd_read)
{
	char mssg[MAX_SIZE];
	int check;
	int i;

/*
	sprintf(mssg,"%d ",0);	// send 0 as a _FAKE_INITIATOR_MESSAGE
    write(fd_write, mssg, strlen(mssg));
*/
	while(1)	// runs an infinite loop until killed by parent
	{

		signalSender(parent_PID,0,AVAILABLE);	// send AVAILABLE signal to sender

		sleep(1);

		
			int red = read(fd_read, mssg, MAX_SIZE);
			//sscanf(mssg,"%d ",&check[i]);

			printf("\n\n@C child[%d] received message %s from parent \n\n",getpid()-parent_PID-1,mssg );

			signalSender(parent_PID,0,BUSY);	// send BUSY signal to sender

			sleep(1);

			char* arg=strtok(mssg," \t\n");

			int count=0,primeCount=0;

			char send[MAX_SIZE]=" ";

			while(arg!=NULL)
			{
				
				sscanf(arg,"%d ",&check);
				count++;
				printf("%d th number : %d\n", count,check);
				if(isPrime(check)) 
				{
					primeCount++;
					sprintf(mssg,"  %d  ",check); 
					strcat(send,mssg);
    			}
				arg=strtok(NULL," \t\n");

			}

			write(fd_write, send, strlen(send));	// queue the prime to write to parent
			printf("\n\nChild[%d] just sent a list of %d primes, %s to parent\n\n", getpid()-parent_PID-1,primeCount,send);
				

			if(primeCount==0)
			{
				// send FAKE IGNORE MESSAGE: 0
				sprintf(mssg,"  %d  ",0); 
    			write(fd_write, mssg, strlen(mssg));	// queue the prime to write to parent
				printf("\n\nChild[%d] just sent a _FAKE_IGNORE_MESSAGE, %d to parent\n\n", getpid()-parent_PID-1,0);
				

			}

			if(count!=K)
				printf("\n\nERROR : parent didn't send K (= %d) numbers ! \n",K );

	}
}

void parentFunction(int K,int N,int* fd_p2c[2],int* fd_c2p[2],int* pid)
{
	int numprime=0,begflag=1;

	int* primes=(int*) malloc(N * sizeof(int));

	int j;

	for(j=0;j<N;j++)
		primes[j]=-1;

	srand((unsigned int)time(NULL));	// seeds random generator with time value
    int rndm;

    printf("Parent Entering into infi-LOOP\n");

	int limit = (N>(2*K)) ? (2*K) : N;
    while(numprime<limit)
    {
    	sleep(2);
    	
    	//pause();	// wait for some child to send "AVAILABLE"
    
    	int k,n;

    	for(j=0;j<K;j++)
    	{
    		if(child_status[j]==AVAILABLE)
    		{
    			char mssg[MAX_SIZE];
    			//int count=0;

    			if(begflag==1) // @ the very beginning
    				{
    					
    					char send[MAX_SIZE]=" ";
    					for(k=0;k<K;k++)
    					{
    						rndm=1 + rand()%30000; // generates random no. from 1 to 30,000
    						sprintf(mssg,"%d ",rndm);
    						strcat(send,mssg);
    					}
    					printf("\n\nParent Constructed & sending message for THE FIRST TIME to child[%d]: %s\n\n\n", j,send);
    					write(fd_p2c[j][1], send, MAX_SIZE);	
    				}
    		else{
    			char mssg[MAX_SIZE]="";
    			strcpy(mssg,"		");
    			read(fd_c2p[j][0], mssg, MAX_SIZE);	// receive message from child
    			printf("\n\n@A parent has received message %s from child\n\n\n",mssg );

    			char* arg=strtok(mssg," \t\n");

    			while(arg!=NULL)
    			{
    				sscanf(arg,"%d ",&n); 

    				arg=strtok(NULL," \t\n");

    				if(n==0)	// ignore the IGNORE MESSAGE
    					continue;

    				printf("\n\nChild has sent me back another prime : %d\n\n\n", n);

    				if(insert(n,primes,N)!=-1)
    					numprime++;

    				if(numprime>N)
    					printf("NOT POSSIBLE !!!\n");
    				else if (numprime==N)
    					break;

    			}
    			//sleep(1);
    			char send[MAX_SIZE]=" ";
    			for(k=0;k<K;k++)
    			{
    				rndm=1 + rand()%30000; // generates random no. from 1 to 30,000
    				sprintf(mssg,"%d ",rndm);
    				strcat(send,mssg);
    			}
    			printf("\n\nParent Constructed & sending message : %s\n\n\n", send);
    			write(fd_p2c[j][1], send, MAX_SIZE);

    			}
    		}
    		if(numprime>=N)
    			break;
    	}
    	begflag=0;

    }

    printf("\n\nN(=%d) primes collected & PrimeList populated ! \n\n Killing all my children...\n\n",N);

    for(j=0;j<K;j++)
    	kill(pid[j],SIGKILL);

    printf("\n\nHere are %d primes : \n\n\n",limit );

    for(j=0;j<limit;j++)
    	printf("%d ",primes[j] );

    free(primes);

}

void signalSender(int pid,int n,int sig)
{
	union sigval obj;
	obj.sival_int=0;

	//printf("sending SIGINT signal to %d......\n",pid);

   	if(sigqueue(pid,sig,obj) == -1)
   	{
    	perror("Signal NOT SENT...sigqueue error ! Terminating...");
    	exit(EXIT_FAILURE);
   	}			
}

void signalHandler(int sig, siginfo_t *siginfo, void *context) 
{
    // get pid of sender,
    pid_t sender_pid = siginfo->si_pid;

    if(sig == AVAILABLE) 
    {
    	child_status[(int)sender_pid - parent_PID -1]=AVAILABLE;
        printf("\n\nsignal AVAILABLE sent from child[%d]\n\n", (int)sender_pid - parent_PID -1);
        return;
    } 
    else if(sig == BUSY) 
    {
       child_status[(int)sender_pid - parent_PID -1]=BUSY;
        printf("\n\nsignal BUSY sent from child[%d]\n\n", (int)sender_pid - parent_PID -1);
        return;
    }

    return;
}

int insert(int num,int *arr,int N)
{
	int i;
	for(i=0;arr[i]!=-1;i++)
		if(arr[i]==num)
			return -1;
	arr[i]=num;
	return 0;
}

int isPrime(int a)
{
	int i;
	for(i=2;i<a;i++)
		if(a%i==0)
			return 0;

	return 1;
}
