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
#define SM1 10
#define SM2 11
#define MSQ1 13
#define MSQ2 14
#define MSQ3 15
#define SEM1 16
#define SEM2 17
#define FILE_SEM 19
#define TERMINATE_SEM 20
#define LOCAL_SIZE 4096
#define MSGSIZE 256

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



struct PageTable* ptr1;
struct FreeFrameList* ptr2;


void initialize(int no_of_processes,int *no_of_pages_required,int no_of_free_frames,int sm1,int sm2)
{
	int i,j;
	
	ptr1 = (struct PageTable*)shmat(sm1,NULL,0);
	ptr2 = (struct FreeFrameList*)shmat(sm2,NULL,0);

    ptr2->no_of_frames = no_of_free_frames;
    for(i = 0;i<no_of_free_frames;++i)
    {
    	ptr2->frames[i] = 1;  //means available
    }

    ptr1->no_of_processes = no_of_processes;

	for(i = 0;i<no_of_processes;++i)
	{
		for(j = 0;j < MAX_SIZE;++j)
		{
			(ptr1->pages[i][j]).frame_number = -1; //means on disk
			(ptr1->pages[i][j]).valid = 0; //means invalid
            (ptr1->pages[i][j]).last_update = 0;
		}
		ptr1->no_of_pages_required[i] = no_of_pages_required[i];
        ptr1->last_updates[i] = 0;
	}
}



int sm1,sm2;

void handler(int a)
{
    printf("Entered\n");
	shmctl(sm1, IPC_RMID, NULL);
	shmctl(sm2, IPC_RMID, NULL);

	char str[30];
	strcpy(str,"ipcrm -Q ");
	strcat(str,i_to_a(MSQ1));
	system(str);

	strcpy(str,"ipcrm -Q ");
	strcat(str,i_to_a(MSQ2));
	system(str);

	strcpy(str,"ipcrm -Q ");
	strcat(str,i_to_a(MSQ3));
	system(str);

    strcpy(str,"ipcrm -S ");
    strcat(str,i_to_a(SEM1));
    system(str);

    strcpy(str,"ipcrm -S ");
    strcat(str,i_to_a(SEM2));
    system(str);

    strcpy(str,"ipcrm -S ");
    strcat(str,i_to_a(FILE_SEM));
    system(str);

    printf("sending\n");

	kill(0,SIGINT);
	printf("sent kill everything\n");
	exit(0);
}

void terminate_fun()
{
    shmctl(sm1, IPC_RMID, NULL);
    shmctl(sm2, IPC_RMID, NULL);

    char str[30];
    strcpy(str,"ipcrm -Q ");
    strcat(str,i_to_a(MSQ1));
    system(str);

    strcpy(str,"ipcrm -Q ");
    strcat(str,i_to_a(MSQ2));
    system(str);

    strcpy(str,"ipcrm -Q ");
    strcat(str,i_to_a(MSQ3));
    system(str);

    strcpy(str,"ipcrm -S ");
    strcat(str,i_to_a(SEM1));
    system(str);

    strcpy(str,"ipcrm -S ");
    strcat(str,i_to_a(SEM2));
    system(str);

    strcpy(str,"ipcrm -S ");
    strcat(str,i_to_a(FILE_SEM));
    system(str);

    strcpy(str,"ipcrm -S ");
    strcat(str,i_to_a(TERMINATE_SEM));
    system(str);

}

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
	srand(time(NULL));
	signal(SIGINT,handler);
    system("cc -o mmu.out mmu.c");
    system("cc -o sched.out sched.c");
    system("cc -o process.out process.c");
	int no_of_processes;
	int max_no_of_pages_per_process;
	int i;
	int no_of_pages_required[MAX_PROCESSES];
	int no_of_free_frames;
	FILE *ptr_file;
	int msq1,msq2,msq3;
    int semid1,semid2,semid3,semid4;
    ptr_file =fopen("input.txt", "r");
    fscanf(ptr_file,"%d",&no_of_processes);
    fscanf(ptr_file,"%d",&max_no_of_pages_per_process);
    struct message sbuf,rbuf;

    pid_t mainpid = getpid();
    pid_t mainpgid = getpgid(mainpid);

    for(i = 0;i<no_of_processes;++i)
    {
    	no_of_pages_required[i] = (rand() % max_no_of_pages_per_process) + 1;
    }

    fscanf(ptr_file,"%d",&no_of_free_frames);
    if((sm1 = shmget(SM1, LOCAL_SIZE, IPC_CREAT|0666)) < 0)
    {
        perror("Error in creatign shared memory 1:");
        exit(1);
    }
    if((sm2 = shmget(SM2, LOCAL_SIZE, IPC_CREAT|0666)) < 0)
    {
        perror("Error in creatign shared memory 2:");
        exit(1);
    }
    
    if (( msq1 = msgget ( MSQ1 , 0666|IPC_CREAT ) ) < 0 ) 
    {
        perror ("Error int creating message queue 1:"); 
        exit (1) ;
    }
    if (( msq2 = msgget ( MSQ2 , 0666|IPC_CREAT ) ) < 0 ) 
    {
        perror ("Error int creating message queue 1:"); 
        exit (1) ;
    }
    if (( msq3 = msgget ( MSQ3 , 0666|IPC_CREAT ) ) < 0 ) 
    {
        perror ("Error int creating message queue 1:"); 
        exit (1) ;
    }
    
    if(( semid1 =semget(SEM1, no_of_processes,0666|IPC_CREAT)) < 0)
    {
        perror("Error in creating semaphores :");
        exit(1);
    }
    ushort val[no_of_processes];
    for(i = 0;i<no_of_processes;++i)
    {
        val[i] = 0;
    }
    semctl(semid1, 0, SETALL, val);

    if(( semid2 =semget(SEM2,1,0666|IPC_CREAT)) < 0)
    {
        perror("Error in creating semaphores :");
        exit(1);
    }
    ushort val1[1];
    for(i = 0;i<1;++i)
    {
        val1[i] = 0;
    }
    semctl(semid2, 0, SETALL, val1);

    if(( semid3 =semget(FILE_SEM,1,0666|IPC_CREAT)) < 0)
    {
        perror("Error in creating semaphores :");
        exit(1);
    }
    ushort val2[1];
    for(i = 0;i<1;++i)
    {
        val2[i] = 1;
    }
    semctl(semid3, 0, SETALL, val2);

    if(( semid4 =semget(TERMINATE_SEM,1,0666|IPC_CREAT)) < 0)
    {
        perror("Error in creating semaphores :");
        exit(1);
    }
    ushort val3[1];
    for(i = 0;i<1;++i)
    {
        val3[i] = 0;
    }
    semctl(semid4, 0, SETALL, val3);



    initialize(no_of_processes,no_of_pages_required,no_of_free_frames,sm1,sm2);

    
    

    //calling scheduler

    char str[200];
    strcpy(str,"./sched.out ");
    strcat(str,i_to_a(SM1));
    strcat(str," ");
    strcat(str,i_to_a(SM2));
    strcat(str," ");
    strcat(str,i_to_a(MSQ1));
    strcat(str," ");
    strcat(str,i_to_a(MSQ2));
    strcat(str," ");
    strcat(str,i_to_a(mainpgid));
    strcat(str," ");
    strcat(str,i_to_a(SEM1));
    strcat(str," ");
    strcat(str,i_to_a(no_of_processes));
    strcat(str," ");
    strcat(str,i_to_a(SEM2));
    strcat(str,"&");

    system(str);
    
    printf("generated scheduler\n");
    //xterm of mmu

    char cwd[1024];
    if(getcwd(cwd, sizeof(cwd)) != NULL)printf("Current working dir: %s\n", cwd);

    strcpy(str,"xterm -hold -e ");
    strcat(str,cwd);
    strcat(str,"/mmu.out ");
    strcat(str,i_to_a(SM1));
    strcat(str," ");
    strcat(str,i_to_a(SM2));
    strcat(str," ");
    strcat(str,i_to_a(MSQ2));
    strcat(str," ");
    strcat(str,i_to_a(MSQ3));
    strcat(str," ");
    strcat(str,i_to_a(mainpgid));
    strcat(str," ");
    strcat(str,i_to_a(SEM2));
    strcat(str,"&");

    system(str);

    printf("generated mmu\n");
    //calling processes one after other

    for(i = 0;i<no_of_processes;++i)
    {
    	sleep(0.25);
        sbuf.mtype = 80000;  //means that process is there here
        strcpy(sbuf.mtext,i_to_a(i));
    	
        strcpy(str,"./process.out ");
    	strcat(str,i_to_a(MSQ1));
    	strcat(str," ");
    	strcat(str,i_to_a(MSQ3));
    	strcat(str," ");
        strcat(str,i_to_a(no_of_pages_required[i]));
        strcat(str," ");
    	strcat(str,i_to_a(mainpgid));
        strcat(str," ");
        strcat(str,i_to_a(i));
        strcat(str," ");
        strcat(str,i_to_a(SEM1));
        strcat(str," ");
        strcat(str,i_to_a(no_of_processes));
        strcat(str,"&");
        system(str);

        if(msgsnd (msq1, &sbuf, strlen(sbuf.mtext)+1 , 0) < 0) 
        {
            perror( "Error in sending Process data : ");
            exit (1) ;
        }
    }
    printf("generated all processes\n");
    
    print_shared_mem_status();

    down(semid4,0);
    terminate_fun();
    printf("-----------Master got Terminated!!------------\n");


}
