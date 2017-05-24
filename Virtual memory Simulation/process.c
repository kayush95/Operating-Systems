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

struct message
{
    long mtype ;
    char mtext[MSGSIZE];
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


int main(int argc,char* argv[])
{
	srand(getpid());
    struct message sbuf,rbuf;

	int MSQ1 = atoi(argv[1]);
	int MSQ3 = atoi(argv[2]);
	int no_of_pages = atoi(argv[3]);
    int i;
	int mainpgid = atoi(argv[4]);
	int id = atoi(argv[5]);
	int SEM1 = atoi(argv[6]);
	int no_of_processes = atoi(argv[7]);
	setpgid(getpid(),mainpgid);

    int msq1 = getmsg(MSQ1);
    int msq3 = getmsg(MSQ3);
    int sem1 = getsem(SEM1,no_of_processes);
    int sem3 = getsem(FILE_SEM,1);

    down(sem1,id);
    
    FILE *ptr_file5;
    ptr_file5 = fopen("process_and_sched_dump.txt", "a");
	

	int length = (rand() % ((8*no_of_pages) + 1) ) + (2*no_of_pages);

    printf("I am %d , no of required pages:%d, length of reference %d\n",id,no_of_pages,length);
    down(sem3,0);
    fprintf(ptr_file5,"I am %d , no of required pages:%d, length of reference %d\n",id,no_of_pages,length);
    up(sem3,0);
    
    char str[50];
    int prev_page = -1;
    for(i = 0;i<length;++i)
    {
        int pageno;
        if(prev_page != -1)
        {
            pageno = prev_page;
            printf("I am accessing again page %d, id: %d because of previous page fault\n",pageno,id);
            down(sem3,0);
            fprintf(ptr_file5,"I am accessing again page %d, id: %d because of previous page fault\n",pageno,id);
            up(sem3,0);
        }
    	else 
        {
            int naw = (rand()%5);
            printf("NAW : %d\n",naw);
            if(naw <= 1 )pageno = (rand() % no_of_pages);
            else pageno = no_of_pages + rand()%20;

            printf("I am accessing page %d , id : %d\n",pageno,id);
            down(sem3,0);
            fprintf(ptr_file5,"I am accessing page %d , id : %d\n",pageno,id);
            up(sem3,0);
        }
        sbuf.mtype = 80000;
        strcpy(str,i_to_a(id));
        strcat(str,"/");
        strcat(str,i_to_a(pageno));

        strcpy(sbuf.mtext,str);
        printf("sending %s\n",sbuf.mtext);

        if(msgsnd (msq3, &sbuf, strlen(sbuf.mtext)+1 , 0) < 0) 
	    {
		    perror( "Error in sending frame information12: ");
		    exit (1) ;
	    }

	    if(msgrcv(msq3,&rbuf,MSGSIZE,id + 1,0) > 0) //process and MMU
	    {
	    	int status = atoi(rbuf.mtext);
	    	if(status == -1) //pagefault
	    	{
	    		printf("Page fault occured %d\n",id);
                down(sem3,0);
                fprintf(ptr_file5,"Page fault occured %d\n",id);
                up(sem3,0);
	    		down(sem1,id);
                prev_page = pageno;
                i = i - 1;
	    	}
	    	else if(status == -2)
	    	{
	    		strcpy(str,i_to_a(id));
		        strcat(str,"/");
		        strcat(str,i_to_a(-9));
                sbuf.mtype = 80000;
                strcpy(sbuf.mtext,str);

		        if(msgsnd (msq3, &sbuf, strlen(sbuf.mtext)+1 , 0) < 0) 
			    {
				    perror( "Error in sending frame information: ");
				    exit (1) ;
			    }
                printf("I got terminated because of invalid access %d\n",id);
                down(sem3,0);
                fprintf(ptr_file5,"I got terminated because of invalid access %d\n",id);
                up(sem3,0);
                fclose(ptr_file5);
			    exit(0);
	    	}
            else
            {
                printf("Succesful page access %d\n",id);
                down(sem3,0);
                fprintf(ptr_file5,"Succesful Page access %d\n",id);
                up(sem3,0);
                prev_page = -1;
            } 

	    }


    }
    sbuf.mtype = 80000;
    
    strcpy(str,i_to_a(id));
    strcat(str,"/");
    strcat(str,i_to_a(-9));
    strcpy(sbuf.mtext,str);

    if(msgsnd (msq3, &sbuf, strlen(sbuf.mtext)+1 , 0) < 0) 
    {
        perror( "Error in sending frame information: ");
        exit (1) ;
    }
    printf("I got terminated %d\n",id);
    down(sem3,0);
    fprintf(ptr_file5,"I got terminated %d\n",id);
    up(sem3,0);
    fclose(ptr_file5);
}