/***************************************************************
Operating Systems Lab Assignment 4 : VIRTUAL SCHEDULER

Name : Kumar Ayush
Roll No. 13CS10058

Name : Varun Rawal
Roll No. 13CS10059

****************************************************************/

// Scheduler program : schedules to select one among many processes, for running it on the CPU

// compute statistics and record into file

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

int timeval_add (struct timeval *result, struct timeval *x, struct timeval *y)
{

  result->tv_sec = x->tv_sec + y->tv_sec;
  result->tv_usec = x->tv_usec + y->tv_usec;

  if(result->tv_usec >= 1000000)
  	result->tv_sec += result->tv_usec / 1000000;
  else
  	result->tv_usec %= 1000000;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

typedef struct node
{
	int PID;
	int priority;
	struct timeval running_time, response_time, turnaround_time;

	// waiting_time @ last = turnaround_time - running_time

	struct timeval start_time;	// used in computing turnaround time

}	Q_node;	// node in the Ready 'Q'

#define QUEUE_MAX_NODES 1000

Q_node ReadyQueue[QUEUE_MAX_NODES];	// array implementation of the Ready Queue
int front=-1,rear=-1;

int processCompleteCounter=0;


Q_node savelist[QUEUE_MAX_NODES]; // array for saving the details of all the processes
int saveList_end_index=-1;

void insertIntoSavingList(Q_node node)
{
	saveList_end_index++;

	int i = saveList_end_index;

	savelist[i].PID = node.PID;
	savelist[i].priority = node.priority;
	savelist[i].running_time = node.running_time;
	savelist[i].response_time = node.response_time;
	savelist[i].turnaround_time = node.turnaround_time;
	savelist[i].start_time = node.start_time;

}

Q_node getFromSavingList(int pid)
{
	int i;
	int found=0;

	for(i=0; i<=saveList_end_index ;i++)
	{
		if(savelist[i].PID == pid)
		{
			found=1;
			return savelist[i];
		}
		
	}

	if(!found)
		{
			printf("Error @ getFromSavingList :  Not found in saving list !\n");
			exit(1);
		}
}

void updateSavingList(Q_node node)
{
	int i;
	int found=0;

	for(i=0; i<=saveList_end_index ;i++)
	{
		if(savelist[i].PID == node.PID)
		{
			found=1;

			savelist[i].running_time = node.running_time;
			savelist[i].response_time = node.response_time;
			savelist[i].turnaround_time = node.turnaround_time;
		}
		
	}

	if(!found)
		{
			printf("Error @ updateSavingList : Not found in saving list !\n");
			printf("end : %d\n",saveList_end_index );
			exit(1);
		}

}

#define RECORD_FILE_NAME "result.txt"

#define N 4

#define RR 0
#define PR 1

#define SCHEDULER_SENSE_CODE 2
    
#define MAX_SIZE 5
#define MSSG_Q_KEY 200

#define NOTIFY SIGUSR1 // when sent by scheduler to process
#define SUSPEND SIGUSR2	// when sent by scheduler to process

#define IO_REQUEST SIGUSR1 // when sent by process to scheduler
#define TERMINATE SIGUSR2 // when sent by process to scheduler

void clearRecordFile();
void appendResultsintoFile(Q_node);
void appendAverageStatisticstoFile();

void insertIntoReadyQueue(int,int);
void insert_QNode_IntoReadyQueue(int);
Q_node removeFromReadyQueue();
void checkMessageQueue();

int IO_request_flag,terminated_flag;
Q_node removeNode;

struct timeval run_start_moment,run_stop_moment;

void signalSender(int pid,int n,int sig)
{
	union sigval obj;
	obj.sival_int=0;

	//printf("sending SIGINT signal to %d......\n",pid);

   	if(sigqueue(pid,sig,obj) == -1)
   	{
    	perror("Signal NOT SENT...sigqueue error ! Returning...");
    	//exit(EXIT_FAILURE);
    	return;
   	}			
}

void signalHandler(int sig, siginfo_t *siginfo, void *context) 
{
    // get pid of sender,
    pid_t sender_pid = siginfo->si_pid;

    if(sig == IO_REQUEST) 
    {
    	printf("PID : %d, has requested for IO task...\n", sender_pid);
    	
    	if(removeNode.PID!=sender_pid)
    	{
    		printf("Rare condition ! Impossible Error ! Exiting...\n\n");
    		printf("PID removed : %d , Signal Sender PID : %d\n",removeNode.PID, sender_pid );
    		exit(1);
    	}
    	IO_request_flag=1;

    	struct timeval diff_time;
    	timeval_subtract(&diff_time,&run_stop_moment,&run_start_moment);

    	timeval_add(&removeNode.running_time,&removeNode.running_time,&diff_time);

    	//removeNode.running_time += difftime(run_stop_moment,run_start_moment);

    	updateSavingList(removeNode);

        return;
    } 
    else if(sig == TERMINATE) 
    {
    	printf("PID : %d, terminated.\n", sender_pid);
       processCompleteCounter++;
      
       if(removeNode.PID!=sender_pid)
    	{
    		printf("Rare condition ! Impossible Error ! Exiting...\n\n");
    		printf("PID removed : %d , Signal Sender PID : %d\n",removeNode.PID, sender_pid );
    		exit(1);
    	}
       terminated_flag=1;

       struct timeval diff_time;
    	timeval_subtract(&diff_time,&run_stop_moment,&run_start_moment);

    	timeval_add(&removeNode.running_time,&removeNode.running_time,&diff_time);

    	//removeNode.running_time += difftime(run_stop_moment,run_start_moment);

    	struct timeval current;
    	gettimeofday(&current,NULL);

       timeval_subtract(&removeNode.turnaround_time,&current,&removeNode.start_time);// = difftime(time(0),removeNode.start_time);

    	updateSavingList(removeNode);

    	appendResultsintoFile(removeNode);

       return;
    }

    return;
}


int scheduling_policy;
int time_quantum; // no. of scheduler iterations

struct mssg // message template for the message Queue
{
	long mtype;
	char mtext[MAX_SIZE];
};

void RoundRobinScheduler();

void main(int argc, char* argv[])
{

	clearRecordFile();
	/* INSTALL THE SIGNAL_ACTION HANDLER FUNCTION***/
    struct sigaction siga;
    siga.sa_sigaction = signalHandler;
    siga.sa_flags = SA_SIGINFO; // get detailed info

    
    //sigemptyset(&siga.sa_mask);

    // change signal action,
    if(sigaction(IO_REQUEST, &siga, NULL) == -1) {
        printf("error in sigaction()");
        return;
    }
    if(sigaction(TERMINATE, &siga, NULL) == -1) {
        printf("error in sigaction()");
        return;
    }


	if(argc!=2)
	{
		printf("Please enter an argument : RR or PR !\n");
		exit(0);
	}
	else 
	{
		if(strcmp(argv[1],"RR")==0)
			scheduling_policy=RR;
		else if(strcmp(argv[1],"PR")==0)
			scheduling_policy=PR;
		else
		{
			printf("Please enter a valid argument : either RR or PR !\n");
			exit(0);
		}
	}

	time_quantum = scheduling_policy==RR ? 1000 : 2000;	// no. of iterations 

	printf("Virtual Scheduler STARTED ....\n");

	while(processCompleteCounter<N)
	{
		//printf("Check 1\n");
		checkMessageQueue();
		//printf("Check 2\n");
		RoundRobinScheduler();
	}

	appendAverageStatisticstoFile();

	printf("All %d processes Completed...all results have been recorded to the file %s ! Exiting..\n", processCompleteCounter , RECORD_FILE_NAME);


}




void RoundRobinScheduler()
{
	removeNode = removeFromReadyQueue(); // a global variable for VERIFICATION CHECK @ recieving Signals ( for IO Request / Termination)

	int scheduledPID = removeNode.PID;
	
	printf("PID : %d scheduled for running...\n",scheduledPID );

	int iter;
	// entering the for loop which will simulate the time quantum timer :
	IO_request_flag=0;
	terminated_flag=0;

	gettimeofday(&run_start_moment,NULL);// = time(0);

	if(removeNode.response_time.tv_usec == 0)// == (struct timeval){0})
	{
		timeval_subtract(&removeNode.response_time,&run_start_moment,&removeNode.start_time);
		//removeNode.response_time = difftime(run_start_moment,removeNode.start_time);
	}
	updateSavingList(removeNode);
	signalSender(scheduledPID,0,NOTIFY);
	usleep(500);	// to ensure that the suspended process ACTUALLY resumes !
	
	for(iter=0;iter<time_quantum;iter++)
	{
		if(IO_request_flag ==1 || terminated_flag==1)
			return;
	}

	if(IO_request_flag==0 && terminated_flag==0)
	{
		// indicates normal exit from the loop, i.e. time quantum had expired 
		
		signalSender(scheduledPID,0,SUSPEND);
		usleep(500);	// to ensure that the suspended process ACTUALLY suspends !
		gettimeofday(&run_stop_moment,NULL);// = time(0);

		if(IO_request_flag ==1 || terminated_flag==1)
			return;

		printf("PID : %d suspended...\n",scheduledPID );

		struct timeval diff_time;
    	timeval_subtract(&diff_time,&run_stop_moment,&run_start_moment);

    	timeval_add(&removeNode.running_time,&removeNode.running_time,&diff_time);

    	//removeNode.running_time += difftime(run_stop_moment,run_start_moment);

		updateSavingList(removeNode);
		insert_QNode_IntoReadyQueue(removeNode.PID);
	}

	//printf("Check : %d\n", front == (rear+1)%QUEUE_MAX_NODES );
	
	return;
}

void checkMessageQueue()
{
	int mssgQ_ID=msgget((key_t)MSSG_Q_KEY,IPC_CREAT|0666);

	//printf("mssgQ_ID : %d\n",mssgQ_ID );

	while(1) // as long as there are "newly Created" process or "Ready" (returned after IO completion) processes, continue the loop and add them all to the Ready Queue
	{
		struct mssg buffer;

		int rcvVal=-1;

		if (front == - 1 || front == (rear+1)%QUEUE_MAX_NODES) // when the Ready Queue is empty, then the wait should be a blocking one
			rcvVal = msgrcv(mssgQ_ID,&buffer,sizeof(buffer),SCHEDULER_SENSE_CODE,0); // blocking receive / wait
		else
			rcvVal = msgrcv(mssgQ_ID,&buffer,sizeof(buffer),SCHEDULER_SENSE_CODE,IPC_NOWAIT); // non-blocking receive / wait
			
		if(rcvVal==-1) // this means that no new process or ready process has arrived
			break;
		//	perror("Error in receiving !\n");

		//usleep(20000);

		if(rcvVal>0) // this means that a new process or a ready process has arrived
		{
			int pid,prior;

			struct msqid_ds qstat;
			msgctl(mssgQ_ID,IPC_STAT,&qstat);
			pid = qstat.msg_lspid ;

			char* first = strtok(buffer.mtext," \t\n");

			char* prio_text = strtok(NULL," \t\n");
			sscanf(prio_text,"%d",&prior);

			if(strcmp(first,"CREATED")==0)
			{
				
				printf("new process PID : %d , Priority : %d arrived\n",pid,prior);

				IO_request_flag=terminated_flag=0;

				insertIntoReadyQueue(pid,prior);

				sprintf(buffer.mtext,"%d",getpid());
				buffer.mtype=pid;
				int retv = msgsnd(mssgQ_ID,&buffer,sizeof(buffer),0);	// blocking send

				if(retv == -1)
				{
					printf("Error : Message Queue unable to buffer messages !\n");
					exit(1);
				}
				else
				{
					//printf("Send confirmation to the process PID : %d\n", pid);
				}

			}
			else if(strcmp(first,"READY")==0)
			{
				printf("process PID : %d, completed IO, READY..\n",pid);

				IO_request_flag=terminated_flag=0;
				insert_QNode_IntoReadyQueue(pid);
			}
				
		}
		
	}
}


void clearRecordFile()
{
	FILE *fp;

   fp = fopen(RECORD_FILE_NAME, "w+");
   fclose(fp);
}

void appendResultsintoFile(Q_node node)
{
	FILE *fp;

   fp = fopen(RECORD_FILE_NAME, "a+");
   fprintf(fp,"\n");
   fprintf(fp,"PID : %d\n",node.PID);
   fprintf(fp,"response time : %f sec, %f usec\n",(double)node.response_time.tv_sec,(double)node.response_time.tv_usec);
  	
  	struct timeval waiting_time;

  	timeval_subtract(&waiting_time,&node.turnaround_time,&node.running_time);

   fprintf(fp,"waiting time :  %f sec, %f usec\n",(double)waiting_time.tv_sec,(double) waiting_time.tv_usec);
   fprintf(fp,"turnaround time :  %f sec, %f usec\n",(double)node.turnaround_time.tv_sec,(double)node.turnaround_time.tv_usec);
   fprintf(fp,"\n");
   fclose(fp);
}

void appendAverageStatisticstoFile()
{
	FILE *fp;

	Q_node avg_value;
	avg_value.response_time=(struct timeval){0};
	avg_value.running_time=(struct timeval){0};
	avg_value.turnaround_time=(struct timeval){0};

	if(saveList_end_index!=N-1)
	{
		printf("Error ! not all processes have been saved !\n");
		printf("%d %d \n",saveList_end_index,N );
		exit(1);
	}

	int i;
	for(i=0;i<N;i++)
	{
		timeval_add(&avg_value.response_time,&avg_value.response_time,&savelist[i].response_time);
		timeval_add(&avg_value.running_time,&avg_value.running_time,&savelist[i].running_time);
		timeval_add(&avg_value.turnaround_time,&avg_value.turnaround_time,&savelist[i].turnaround_time);
	}
	avg_value.response_time.tv_sec/=N;
	avg_value.response_time.tv_usec/=N;

	avg_value.running_time.tv_sec/=N;
	avg_value.running_time.tv_usec/=N;

	avg_value.turnaround_time.tv_sec/=N;
	avg_value.turnaround_time.tv_usec/=N;

   fp = fopen(RECORD_FILE_NAME, "a+");
   fprintf(fp,"\n\n");
   fprintf(fp,"Average Stats :\n");
   fprintf(fp,"response time : %f sec, %f usec\n",(double)avg_value.response_time.tv_sec,(double)avg_value.response_time.tv_usec);
  
   struct timeval waiting_time;

  	timeval_subtract(&waiting_time,&avg_value.turnaround_time,&avg_value.running_time);

   fprintf(fp,"waiting time :  %f sec, %f usec\n",(double)waiting_time.tv_sec,(double) waiting_time.tv_usec);
   fprintf(fp,"turnaround time :  %f sec, %f usec\n",(double)avg_value.turnaround_time.tv_sec,(double)avg_value.turnaround_time.tv_usec);
   fprintf(fp,"\n");


   fclose(fp);
}

int getPositive_decrement_MODULUS(int n,int mod)
{
	n = (n-1)%mod;
	if(n<0)
		n+=mod;

	return n;
}


void insertIntoReadyQueue(int PID, int prio)
{
	
	if(IO_request_flag == 1 || terminated_flag == 1)
		return;

		if(scheduling_policy == RR )
		{
			if (front == - 1)   // If queue is initially empty 
	   	     front = 0;

	        rear = (rear+1) % QUEUE_MAX_NODES;

	        ReadyQueue[rear].PID = PID;
	        ReadyQueue[rear].priority = prio;
	        ReadyQueue[rear].running_time = (struct timeval){0};
	        ReadyQueue[rear].turnaround_time = ReadyQueue[rear].response_time = (struct timeval){0};
	        gettimeofday(&ReadyQueue[rear].start_time,NULL);

	        insertIntoSavingList(ReadyQueue[rear]);
		}
		else if (scheduling_policy == PR)
		{
			if (front == - 1)   // If queue is initially empty 
	   	     {
	   	     	if(rear!=-1)
	   	     	{
	   	     		printf("Impossible error !\n");
	   	     		exit(1);
	   	     	}
	   	     	else
	   	     	{
	   	     		front++;
	   	     		rear++;

	   	     		ReadyQueue[rear].PID = PID;
			        ReadyQueue[rear].priority = prio;
			        ReadyQueue[rear].running_time =(struct timeval){0};
			        ReadyQueue[rear].turnaround_time = ReadyQueue[rear].response_time = (struct timeval){0};
			        gettimeofday(&ReadyQueue[rear].start_time,NULL);

	       			insertIntoSavingList(ReadyQueue[rear]);
	   	     	}
	   	     }
	   	     else
	   	     {
	   	     	int j=rear;
	   	     	for(j=rear; ReadyQueue[j].priority > prio ; j = getPositive_decrement_MODULUS(j,QUEUE_MAX_NODES) )
	   	     	{
	   	     		ReadyQueue[j+1].PID = ReadyQueue[j].PID;
	   	     		ReadyQueue[j+1].priority = ReadyQueue[j].priority;
	   	     		ReadyQueue[j+1].running_time = ReadyQueue[j].running_time;
	   	     		ReadyQueue[j+1].turnaround_time = ReadyQueue[j].turnaround_time;
	   	     		ReadyQueue[j+1].response_time = ReadyQueue[j].response_time;
	   	     		ReadyQueue[j+1].start_time = ReadyQueue[j].start_time;
	   	     	}

	   	     	ReadyQueue[j+1].PID = PID;
		        ReadyQueue[j+1].priority = prio;
		        ReadyQueue[j+1].running_time = (struct timeval){0};
		        ReadyQueue[j+1].turnaround_time = (struct timeval){0};
		        gettimeofday(&ReadyQueue[rear].start_time,NULL);

	     		insertIntoSavingList(ReadyQueue[j+1]);
	   	     }

		}
		else
		{
			printf("Impossible Error !\n");
			exit(1);
		}
	
}


void insert_QNode_IntoReadyQueue(int pid)
{
	
	if(IO_request_flag == 1 || terminated_flag == 1)
		return;

		if(scheduling_policy == RR )
		{
			if (front == - 1)   // If queue is initially empty 
	   	     front = 0;

	        rear = (rear+1) % QUEUE_MAX_NODES;

	        ReadyQueue[rear].PID = pid;

	        Q_node getNode = getFromSavingList(pid);

	        ReadyQueue[rear].priority = getNode.priority;
	        ReadyQueue[rear].running_time = getNode.running_time;
	        ReadyQueue[rear].turnaround_time = getNode.turnaround_time;
	        ReadyQueue[rear].response_time = getNode.response_time;
	        ReadyQueue[rear].start_time = getNode.start_time;

		}
		else if (scheduling_policy == PR)
		{
			if (front == - 1)   // If queue is initially empty 
	   	     {
	   	     	if(rear!=-1)
	   	     	{
	   	     		printf("Impossible error !\n");
	   	     		exit(1);
	   	     	}
	   	     	else
	   	     	{
	   	     		front++;
	   	     		rear++;

	   	     		ReadyQueue[rear].PID = pid;

			        Q_node getNode = getFromSavingList(pid);

			        ReadyQueue[rear].priority = getNode.priority;
			        ReadyQueue[rear].running_time = getNode.running_time;
			        ReadyQueue[rear].turnaround_time = getNode.turnaround_time;
			        ReadyQueue[rear].response_time = getNode.response_time;
			        ReadyQueue[rear].start_time = getNode.start_time;
	   	     	}
	   	     }
	   	     else
	   	     {
	   	     		

	   	     	int j=rear;

			        Q_node getNode = getFromSavingList(pid);

	   	     	for(j=rear; ReadyQueue[j].priority > getNode.priority ; j = getPositive_decrement_MODULUS(j,QUEUE_MAX_NODES) )
	   	     	{
	   	     		ReadyQueue[j+1].PID = ReadyQueue[j].PID;
	   	     		ReadyQueue[j+1].priority = ReadyQueue[j].priority;
	   	     		ReadyQueue[j+1].running_time = ReadyQueue[j].running_time;
	   	     		ReadyQueue[j+1].turnaround_time = ReadyQueue[j].turnaround_time;
	   	     		ReadyQueue[j+1].response_time = ReadyQueue[j].response_time;
	   	     	}

	   	     		ReadyQueue[j+1].PID = pid;

			        getNode = getFromSavingList(pid);

			        ReadyQueue[j+1].priority = getNode.priority;
			        ReadyQueue[j+1].running_time = getNode.running_time;
			        ReadyQueue[j+1].turnaround_time = getNode.turnaround_time;
			        ReadyQueue[j+1].response_time = getNode.response_time;
			        ReadyQueue[j+1].start_time = getNode.start_time;
	   	     }

		}
		else
		{
			printf("Impossible Error !\n");
			exit(1);
		}
	
}



Q_node removeFromReadyQueue()
{

	if (front == - 1 || front == (rear+1)%QUEUE_MAX_NODES)
    {
        printf("Ready Queue Underflow \n");
        exit(1);
    }
    else
    {
		int savFront = front;
        
        front = (front+1)%QUEUE_MAX_NODES;

        return ReadyQueue[savFront];
    }

}



/*

 // C Program to Implement a Queue using an Array
  
#define MAX 50
int queue_array[MAX];
int rear = - 1;
int front = - 1;

void insert()
{
    int add_item;
    if (rear == MAX - 1)
    printf("Queue Overflow \n");
    else
    {
        if (front == - 1)
        // If queue is initially empty 
        front = 0;
        printf("Insert the element in queue : ");
        scanf("%d", &add_item);
        rear = rear + 1;
        queue_array[rear] = add_item;
    }
} // End of insert()
 
void delete()
{
    if (front == - 1 || front > rear)
    {
        printf("Queue Underflow \n");
        return ;
    }
    else
    {
        printf("Element deleted from queue is : %d\n", queue_array[front]);
        front = front + 1;
    }
} // End of delete() 
void display()
{
    int i;
    if (front == - 1)
        printf("Queue is empty \n");
    else
    {
        printf("Queue is : \n");
        for (i = front; i <= rear; i++)
            printf("%d ", queue_array[i]);
        printf("\n");
    }
} // End of display() 

    */