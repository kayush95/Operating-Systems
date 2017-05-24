#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>


#define MAX_SIZE 20
#define MAX_PROCESSES 6
#define LOCAL_SIZE 4096
#define MSGSIZE 256
#define FILE_SEM 19
#define TERMINATE_SEM 20

struct message
{
    long mtype ;
    char mtext[MSGSIZE];
};

struct Page
{
	int frame_number;
	int valid;
	int last_update;
};

struct PageTable
{
	struct Page pages[MAX_PROCESSES][MAX_SIZE];
	int no_of_processes;
	int no_of_pages_required[MAX_PROCESSES];
	int last_updates[MAX_PROCESSES];
};

struct FreeFrameList
{
	int frames[MAX_SIZE];  //each contains 0 or 1 to denote valid or not
	int no_of_frames;
};

char* i_to_a(int x)   //converting int to char*
{
    char *s = (char*)malloc(20*sizeof(char));
    sprintf(s,"%d",x);
    return s;
}

int getmsg(int a)
{
    int r;
    if (( r = msgget ( a , 0666|IPC_CREAT ) ) < 0 ) 
    {
        perror ("Error int creating message queue:"); 
        exit (1) ;
    }
    return r;
}

int getsem(int a,int b)
{
    int r;
    if(( r =semget(a, b, 0666|IPC_CREAT)) < 0)
    {
        perror("Error in creating semaphores :");
        exit(1);
    }
    return r;
}

int getshm(int a)
{
    int r;
    if((r = shmget(a, LOCAL_SIZE, IPC_CREAT|0666)) < 0)
    {
        perror("Error in creatign shared memory :");
        exit(1);
    }
    return r;
}

void down(int semid,int a)
{
            struct sembuf sop;
            sop.sem_num= a;
            sop.sem_op= -1;
            sop.sem_flg =0;
            semop(semid, &sop, 1);
}

void up(int semid,int a)
{
            struct sembuf sop;
            sop.sem_num= a;
            sop.sem_op= 1;
            sop.sem_flg =0;
            semop(semid, &sop, 1);
}


struct PageTable* ptr1;
struct FreeFrameList* ptr2;

void print_shared_mem_status()
{
	printf("Page Table entries are-----\n");
    int i;
    for(i = 0;i< ptr1->no_of_processes;++i)
    {
        printf("Process -- %d,%d\n",i+1,ptr1->no_of_pages_required[i]);
        /*int no_entries = ptr1->no_of_pages_required[i];
        int j;
        for(j = 0;j<no_entries;++j)
        {
            printf("(%d,%d);",(ptr1->pages[i][j]).frame_number,(ptr1->pages[i][j]).valid);
        }*/
    }
    printf("\n");
    printf("Free Frames are-------\n");
    for(i = 0;i<ptr2->no_of_frames;++i)
    {
        printf("%d ",ptr2->frames[i]);
    }
    printf("\n");
}


int main(int arc,char* argv[])
{
	struct message sbuf,rbuf;
	int SM1 = atoi(argv[1]);
	int SM2 = atoi(argv[2]);

	int MSQ1 = atoi(argv[3]);
	int MSQ2 = atoi(argv[4]);

	int mainpgid = atoi(argv[5]);
	int SEM1 = atoi(argv[6]);
	int no_of_processes = atoi(argv[7]);
	int SEM2 = atoi(argv[8]);
    
    FILE *ptr_file4;
    ptr_file4 = fopen("process_and_sched_dump.txt", "a");


    setpgid(getpid(),mainpgid);

    int sm1 = getshm(SM1);
    int sm2 = getshm(SM2);

    int msq1 = getmsg(MSQ1);
    int msq2 = getmsg(MSQ2);
    int sem1 = getsem(SEM1,no_of_processes);
    int sem2 = getsem(SEM2,1);
    int sem3 = getsem(FILE_SEM,1);
    int sem4 = getsem(TERMINATE_SEM,1);

    ptr1 = (struct PageTable*)shmat(sm1,NULL,0);
	ptr2 = (struct FreeFrameList*)shmat(sm2,NULL,0);
    
    ushort retval[1];
    semctl(sem2, 0, GETALL, retval);

	//print_shared_mem_status();
    int terminate_count = 0;
    int total_no_of_processes = ptr1->no_of_processes;

    while(1)
    {
        if(terminate_count == total_no_of_processes)
        {
            printf("sched broke\n");
            break;
        }
    	if(msgrcv(msq1,&rbuf,MSGSIZE,80000,0) > 0)
	    {
	    	int process_id = atoi(rbuf.mtext);
	    	printf("SCHED: wakes up process id %d\n",process_id);
            down(sem3,0);
            fprintf(ptr_file4,"SCHED: wakes up process id %d\n",process_id);
            up(sem3,0);
	    	up(sem1,process_id); //wakes up this process
            
            printf("SCHED: I got blocked till MMU notifies\n");
            down(sem3,0);
            fprintf(ptr_file4,"SCHED: I got blocked till MMU notifies\n");
            up(sem3,0);
	    	down(sem2,0);  //blocks itself till MMU notifies
            printf("SCHED: I woke up\n");
            down(sem3,0);
            fprintf(ptr_file4,"SCHED: I woke up\n");
            up(sem3,0);
	        
	        if (msgrcv(msq2,&rbuf,MSGSIZE,80000,MSG_NOERROR|0) > 0)  //gets reply from MMU
	        {
	        	//received message format-- case/process_id
	        	char* pp1 = (char*)malloc(50*sizeof(char));
                strcpy(pp1,rbuf.mtext);

                char *was1 = strtok_r(pp1,"/",&pp1);
                char* was2 = strtok_r(pp1, "/",&pp1);

                if(strcmp(was1,"1") == 0) //means page fault case
                {
                	sbuf.mtype = 80000;
                	strcpy(sbuf.mtext,was2);
                	if(msgsnd (msq1, &sbuf, strlen(sbuf.mtext)+1 , 0) < 0) 
			        {
			            perror( "Error in sending Process data : ");
			            exit (1) ;
			        }
                    printf("SCHED: pushing process id : %s to last\n",was2);
                    down(sem3,0);
                    fprintf(ptr_file4,"SCHED: pushing process id : %s to last\n",was2);
                    up(sem3,0);
                }
                if(strcmp(was1,"2") == 0)
                {
                    printf("SCHED terminate count %d\n",terminate_count);
                    ++terminate_count;
                }
	        }

	    }

    }
    fclose(ptr_file4);
    up(sem4,0); //TERMINATE SIGNAL TO MASTER
}