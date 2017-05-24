#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <time.h>
#include <termios.h>
#include <ctype.h>


#define KEY 1234
#define CREATE 10
#define IO_END 20
#define SCHEDULER 30
#define INT_MIN -1

int current_process, current_priority, time_expire, N = 4;
int id[4];


typedef struct mssgbuff 
{
    long mtype;
    int priority;   
    int pid;
} 
message_buf;

int createMssgQueue()
{
	int msqid;
	int msgflg = IPC_CREAT | 0666;
	key_t key = KEY;

	printf("mssgget : Calling msgget(%d, %d)\n", key, msgflg);

	if ((msqid = msgget(key, msgflg )) < 0) 
	{
		perror("msgget");
		exit(1);
	}
	else 
		(void) fprintf(stderr,"msgget: msgget succeeded: msqid = %d\n", msqid);

	return msqid;
}

/*********************************************Regular Round Robin****************************************/
struct Queue
{
    int front, rear, size;
    unsigned capacity;
    int* array;
};
 
// function to create a queue of given capacity. It initializes size of queue as 0
struct Queue* createRR(int capacity)
{
    struct Queue* queue = (struct Queue*) malloc(sizeof(struct Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0; 
    queue->rear = capacity - 1;  // This is important, see the enqueue
    queue->array = (int*) malloc(queue->capacity * sizeof(int));
    return queue;
}
 
// Queue is full when size becomes equal to the capacity 
int isFull(struct Queue* queue)
{  return (queue->size == queue->capacity);  }
 
// Queue is empty when size is 0
int isEmpty(struct Queue* queue)
{  return (queue->size == 0); }
 
// Function to add an item to the queue.  It changes rear and size
void enqueue(struct Queue* queue, int item)
{
    if (isFull(queue))
        return;
    queue->rear = (queue->rear + 1)%queue->capacity;
    queue->array[queue->rear] = item;
    queue->size = queue->size + 1;
    printf("%d enqueued to queue\n", item);
}
 
// Function to remove an item from queue.  It changes front and size
int dequeue(struct Queue* queue)
{
    if (isEmpty(queue))
        return INT_MIN;
    int item = queue->array[queue->front];
    queue->front = (queue->front + 1)%queue->capacity;
    queue->size = queue->size - 1;
    return item;
}
 
// Function to get front of queue
int front(struct Queue* queue)
{
    if (isEmpty(queue))
        return INT_MIN;
    return queue->array[queue->front];
}
 
// Function to get rear of queue
int rear(struct Queue* queue)
{
    if (isEmpty(queue))
        return INT_MIN;
    return queue->array[queue->rear];
}
/********************************************************************************************************/

/*********************************************Priority Round Robin***************************************/
typedef struct PriorQueue
{
	int pid;
	int priority;
}
PQueue;

void initialize(PQueue arr[], int n)
{
   int j;
   for (j = 0; j < n; j++)
   {
      arr[j].pid = 0 ; 
      arr[j].priority = 0;
   }
}

void sort(PQueue arr[], int n)
{
   int i, j;
   for (i = 0; i < n-1; i++)      
       for (j = 0; j < n-i-1; j++)
           if (arr[j].priority > arr[j+1].priority)
              {
			int temp1, temp2;
			temp1 = arr[j].pid ; temp2 = arr[j].priority;
			arr[j].pid = arr[j+1].pid ; arr[j].priority = arr[j+1].priority;
			arr[j+1].pid = temp1; arr[j+1].priority = temp2;
	      }
}

void shift(PQueue arr[], int n)
{
   int j;
   for (j = 0; j < n-1; j++)
   {
      arr[j].pid = arr[j+1].pid ; 
      arr[j].priority = arr[j+1].priority;
   }
   arr[j].pid = 0;
   arr[j].priority = 0;    
}

int empty(PQueue arr[])
{
	if(arr[0].pid == 0 && arr[0].priority == 0)
		return 1;
	else 
		return 0;
}
/********************************************************************************************************/

void terminate_regular()
{
	printf("P-%d : %d is Terminated\n", position(id, current_process), current_process);
	time_expire = 1;
	N--;
}

void terminate_priority()
{
	printf("P-%d : %d with priority : %d is Terminated\n",  position(id, current_process), current_process, current_priority);
	time_expire = 1;
	N--;
}

void IO_request_regular()
{
	printf("P-%d : %d requests I/O\n", position(id, current_process), current_process);
	time_expire = 1;
}

void IO_request_priority()
{
	printf("P-%d : %d with priority : %d requests I/O\n", position(id, current_process), current_process, current_priority);
	time_expire = 1;
}

int position(int id[], int pid)
{
	int i, pos;
	for(i = 0; i < N; i++)
	{
		if(id[i] == pid)
			return i;
	}
}
 
int main(int argc, char * argv[])
{

	if(argc != 3)
	{
		printf("./sched <Type = RR or PR> <Quantum>\n");
		exit(1);	
	}

	int msqid, i, quantum = 1000;
	int k;	
	printf("Main function - msqid = %d\n", msqid);
	char* c = argv[1];
	quantum = atoi(argv[2]);
	message_buf sbuf, rbuf;

	for(i= 0; i < N; i++)
		id[i] = 0;
	
	if(strcmp(c,"RR") == 0)
	{
		k = 0;
		struct Queue* RR = createRR(1000);
		msqid = createMssgQueue();

		while(N>0)
		{
			//sleep(1);
			while(msgrcv(msqid, &rbuf, sizeof(message_buf) - sizeof(long) , CREATE, IPC_NOWAIT) > 0)
			{
				printf("New process P-%d created with PID : %d\n", k, rbuf.pid);
				id[k] = rbuf.pid;
				k++;
				enqueue(RR, rbuf.pid);		
			}
			
			while(msgrcv(msqid, &rbuf, sizeof(message_buf) - sizeof(long) , IO_END, IPC_NOWAIT) > 0)
			{
				printf("P-%d : %d Completes I/O\n", position(id, rbuf.pid), rbuf.pid);
				enqueue(RR, rbuf.pid);		
			}
			
			if(isEmpty(RR) == 1)
				{continue;}

			current_process = front(RR);
			printf("P-%d : %d is Running\n", position(id, current_process), current_process);
			kill(current_process, SIGUSR2);
			for(i = 0; i < quantum; i++)
			{							
				signal(SIGUSR1, IO_request_regular);
				signal(SIGUSR2, terminate_regular);
				if(time_expire == 1)
				{
					dequeue(RR);
					break;
				}
			}
			if(time_expire == 0)
			{
				printf("P-%d : %d is Suspended\n", position(id, current_process), current_process);
				kill(current_process, SIGUSR1);
				dequeue(RR);
				enqueue(RR, current_process);
			}
			else
				time_expire = 0;
			
		}
	}
	else if(strcmp(c,"PR") == 0)
	{
		k = 0;
		int maxsize = 1000, capacity = 0;
		PQueue PR[maxsize];
		initialize(PR, maxsize);
		msqid = createMssgQueue();

		while(N>0)
		{
			//sleep(1);
			while(msgrcv(msqid, &rbuf, sizeof(message_buf) - sizeof(long) , CREATE, IPC_NOWAIT) > 0)
			{
				printf("New process P-%d with priority : %d created with PID : %d\n", k,  rbuf.priority, rbuf.pid);
				id[k] = rbuf.pid;
				k++;
				PR[capacity].pid = rbuf.pid;
				PR[capacity].priority = rbuf.priority;
				sort(PR, capacity+1);
				capacity++;
			}
			
			while(msgrcv(msqid, &rbuf, sizeof(message_buf) - sizeof(long) , IO_END, IPC_NOWAIT) > 0)
			{
				printf("P-%d : %d with priority : %d Completes I/O\n", position(id, rbuf.pid), rbuf.pid, rbuf.priority);
				PR[capacity].pid = rbuf.pid;
				PR[capacity].priority = rbuf.priority;
				sort(PR, capacity+1);
				capacity++;
			}
			
			if(empty(PR) == 1)
				{continue;}

			current_process = PR[0].pid;
			current_priority = PR[0].priority;
			printf("P-%d : %d with priority : %d is Running\n", position(id, current_process), current_process, current_priority);
			kill(current_process, SIGUSR2);
			for(i = 0; i < quantum; i++)
			{							
				signal(SIGUSR1, IO_request_priority);
				signal(SIGUSR2, terminate_priority);
				if(time_expire == 1)
				{
					shift(PR, capacity);
					capacity--;
					break;
				}
			}
			if(time_expire == 0)
			{
				printf("P-%d : %d with priority : %d is Suspended\n", position(id, current_process), current_process, current_priority);
				kill(current_process, SIGUSR1);
				shift(PR, capacity);
				capacity--;
				PR[capacity].pid = current_process;
				PR[capacity].priority = current_priority;	
				sort(PR, capacity+1);
				capacity++;			
			}
			else
				time_expire = 0;
			
		}
		
	}
	else
	{
		printf("Invalid Choice!\n");
		exit(1);
	}	 
	return 0;
}

