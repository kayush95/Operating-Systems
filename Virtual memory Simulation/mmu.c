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

struct PageTable* ptr1;
struct FreeFrameList* ptr2;


int getmin(int pro_id)
{
    int k = ptr1->no_of_pages_required[pro_id];
    int min  = 100000000;
    int min_page = -1;
    int i = 0;
    for(i = 0; i < k;++i)
    {
    	if( (min > (ptr1->pages[pro_id][i]).last_update) && (ptr1->pages[pro_id][i]).valid == 1)
    	{
    		min = (ptr1->pages[pro_id][i]).last_update;
    		min_page = i;
    	}
    }
    return min_page;
}

int local_page_handler(int pro_id,int pageno)
{
	int pp = getmin(pro_id);
	if(pp == -1)
	{
		return 0; //failure
	}
	else
	{
		++(ptr1->last_updates[pro_id]);
		int frameno = (ptr1->pages[pro_id][pp]).frame_number;
		(ptr1->pages[pro_id][pp]).frame_number = -1;
		(ptr1->pages[pro_id][pp]).valid = 0;
		(ptr1->pages[pro_id][pp]).last_update = 0;

		(ptr1->pages[pro_id][pageno]).frame_number  = frameno;
		(ptr1->pages[pro_id][pageno]).valid = 1;
		(ptr1->pages[pro_id][pageno]).last_update = ptr1->last_updates[pro_id];
		return 1; //success
	}
}

int get_free_frame()
{
	int m = ptr2->no_of_frames;
	int i;
	for(i =0; i<m; ++i)
	{
		if(ptr2->frames[i] == 1)
		{
			ptr2->frames[i] = 0;
			return i;
		}
	}
	return -1;
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



int allocate(int pro_id,int pageno)
{
	int k = get_free_frame();
	if(k == -1)
	{
		return 0; //failure
	}
	else
	{
		++(ptr1->last_updates[pro_id]);
		(ptr1->pages[pro_id][pageno]).frame_number = k;
		(ptr1->pages[pro_id][pageno]).valid = 1;
		(ptr1->pages[pro_id][pageno]).last_update = ptr1->last_updates[pro_id];
		return 1; //success
	}
}



void takeaway_allocated(int pro_id)
{
	int k = ptr1->no_of_pages_required[pro_id];
	int i;
	for(i = 0;i<k;++i)
	{
		if( (ptr1->pages[pro_id][i]).valid  == 1)
		{
			int frameno = (ptr1->pages[pro_id][i]).frame_number;
			ptr2->frames[frameno] = 1;
			(ptr1->pages[pro_id][i]).valid = 0;
			(ptr1->pages[pro_id][i]).frame_number = -1;
			(ptr1->pages[pro_id][i]).last_update = 0;
		}
	}
}


int globaltime = 1;
int no_of_page_faults[MAX_PROCESSES];
int invalid_references[MAX_PROCESSES];

int main(int argc,char* argv[])
{
	struct message sbuf,rbuf;
	int SM1 = atoi(argv[1]);
	int SM2 = atoi(argv[2]);

	int MSQ2 = atoi(argv[3]);
	int MSQ3 = atoi(argv[4]);
    FILE *ptr_file3;
    ptr_file3 = fopen("MMU_dump.txt", "w");

    FILE *ptr_file4;
    ptr_file4 = fopen("result_pagefaults.txt", "w");

    FILE *ptr_file5;
    ptr_file5 = fopen("result_invalidrefernces.txt", "w");

    FILE *ptr_file6;
    ptr_file6 = fopen("result_globalaccess.txt", "w");

    FILE *ptr_file7;
    ptr_file7 = fopen("result_count_of_pagefaults.txt", "w");

    FILE *ptr_file8;
    ptr_file8 = fopen("result_count_of_invalid_references.txt", "w");


	int mainpgid = atoi(argv[5]);
	int SEM2 = atoi(argv[6]);
    setpgid(getpid(),mainpgid);
    int sm1 = getshm(SM1);
    int sm2 = getshm(SM2);

    int msq2 = getmsg(MSQ2);
    int msq3 = getmsg(MSQ3);
    int sem2 = getsem(SEM2,1);
    ptr1 = (struct PageTable*)shmat(sm1,NULL,0);
 	ptr2 = (struct FreeFrameList*)shmat(sm2,NULL,0);
 	int yy,zz;
 	for(yy = 0;yy<MAX_PROCESSES;++yy)
 	{
 		no_of_page_faults[yy] = 0;
 		invalid_references[yy] = 0;
 	}

    int total_no_of_processes = ptr1->no_of_processes;
    int terminate_count = 0;
 	print_shared_mem_status();

    int mmmm = 0;
    while(1)
    {
    	    if(terminate_count == total_no_of_processes)
    	    {
    	    	printf("breaked\n");
    	    	break;
    	    }
    	    ++mmmm;
    	    //printf("waiting near rcv %d\n",mmmm);
    	    if(msgrcv(msq3,&rbuf,MSGSIZE,80000,0) > 0) //process and MMU
			{
				char* pp1 = (char*)malloc(50*sizeof(char));
		        strcpy(pp1,rbuf.mtext);
		        printf("MMU received %s\n",pp1);

		        char *was1 = strtok_r(pp1,"/",&pp1); //sends process_id/pageno
		        char* was2 = strtok_r(pp1, "/",&pp1);
                
		        int process_id = atoi(was1);
		        int pageno = atoi(was2);

		        printf("(%d,%d,%d)\n",globaltime,process_id,pageno);
		        fprintf(ptr_file3,"(%d,%d,%d)\n",globaltime,process_id,pageno);
		        fprintf(ptr_file6,"(%d,%d,%d)\n",globaltime,process_id,pageno);
		        ++globaltime;

		        if(pageno == -9)
		        {
		        	++terminate_count;
		        	printf("COUNT : %d\n",terminate_count);
		        	printf("process terminating %d\n",process_id);
		        	fprintf(ptr_file3,"process terminating %d\n",process_id);
		        	char str[50];
		        	strcpy(str,i_to_a(2));
		        	strcat(str,"/");
		        	strcat(str,"3");

		        	sbuf.mtype = 80000;
		        	strcpy(sbuf.mtext,str);

			        if(msgsnd (msq2, &sbuf, strlen(sbuf.mtext)+1 , 0) < 0) 
			        {
			            perror( "Error in process information to sched: ");
			            exit (1) ;
			        }
			        takeaway_allocated(process_id);
			        printf("Took away allocated resources to process id : %d\n",process_id);
			        fprintf(ptr_file3,"Took away allocated resources to process id : %d\n",process_id);
			        up(sem2,0); 
			        printf("notified scheduler terminating case\n");
			        fprintf(ptr_file3,"notified scheduler terminating case\n");
		        }
		        
                
		        else if(pageno > (ptr1->no_of_pages_required[process_id]) - 1)
		        {
		        	printf("process accessing %d , id: %d out of range \n",pageno,process_id);
		        	fprintf(ptr_file5,"(%d,%d)\n",process_id,pageno);
		        	invalid_references[process_id] = 1;
		        	fprintf(ptr_file3,"process accessing %d , id: %d out of range \n",pageno,process_id);
		        	sbuf.mtype = process_id + 1;
		        	strcpy(sbuf.mtext , i_to_a(-2)); //sending terminate signal to process
		        	if(msgsnd (msq3, &sbuf, strlen(sbuf.mtext)+1 , 0) < 0) 
			        {
			            perror( "Error in sending frame information: ");
			            exit (1) ;
			        }

			        sbuf.mtype = 80000;
			        char str[50];
			        strcpy(str,"2");
			        strcat(str,"/");
			        strcat(str,"3");
			        
			        strcpy(sbuf.mtext,str);

			        /*if(msgsnd (msq2, &sbuf, strlen(sbuf.mtext)+1 , 0) < 0) 
			        {
			            perror( "Error in process information to sched: ");
			            exit (1) ;
			        }*/
			        up(sem2,0);
			        printf("notified scheduler out of range case \n");
			        fprintf(ptr_file3,"notified scheduler out of range case \n");
		        }
		        else
		        {
		        	if((ptr1 -> pages[process_id][pageno]).valid == 1)
		        	{
		        		sbuf.mtype = process_id + 1;
		        		int frameno =  (ptr1->pages[process_id][pageno]).frame_number;
		        		(++ptr1->last_updates[process_id]);
		        		(ptr1->pages[process_id][pageno]).last_update = ptr1->last_updates[process_id];
		        		strcpy(sbuf.mtext , i_to_a(frameno));
		        		if(msgsnd (msq3, &sbuf, strlen(sbuf.mtext)+1 , 0) < 0) 
				        {
				            perror( "Error in sending frame information: ");
				            exit (1) ;
				        }
				        printf("process accessed %d , id: %d and frame returned\n",pageno,process_id);
				        fprintf(ptr_file3,"process accessed %d , id: %d and frame returned\n",pageno,process_id);
		        	}
		        	else //pagefault case
		        	{
		        		int m = allocate(process_id,pageno);
		        		int t = 2;
		        		if(m == 0) //no free frames
		        		{
		        			t = local_page_handler(process_id,pageno);
		        			if(t  == 1)
		        			{
		        				printf("pagefault of pageno %d, id :%d resolved by LRU\n",pageno,process_id);
		        				fprintf(ptr_file3,"pagefault of pageno %d, id :%d resolved by LRU\n",pageno,process_id);
		        			}
		        			else 
		        			{
		        				printf("pagefault of pageno %d,id : %d not resolved by LRU\n",pageno,process_id);
		        				fprintf(ptr_file3,"pagefault of pageno %d,id : %d not resolved by LRU\n",pageno,process_id);
		        			}
		        		}
		        		else
		        		{
		        			printf("pagefault of pageno %d, id : %d resolved by allocation\n",pageno,process_id);
		        			fprintf(ptr_file3,"pagefault of pageno %d, id : %d resolved by allocation\n",pageno,process_id);
		        		}
		        		sbuf.mtype = process_id + 1;
		        		strcpy(sbuf.mtext,i_to_a(-1));

		        		if(msgsnd (msq3, &sbuf, strlen(sbuf.mtext)+1 , 0) < 0) 
				        {
				            perror( "Error in sending frame information: ");
				            exit (1) ;
				        }

				        char str[30];
				        strcpy(str,"1/");
				        strcat(str,i_to_a(process_id));

				        sbuf.mtype = 80000;
				        strcpy(sbuf.mtext,str);

				        if(msgsnd (msq2, &sbuf, strlen(sbuf.mtext)+1 , 0) < 0) 
				        {
				            perror( "Error in process information to sched: ");
				            exit (1) ;
				        }
				        

				        up(sem2,0);
				        printf("notified scheduler about case of pagefault, i.e case 1\n");
				        fprintf(ptr_file4, "(%d,%d)\n",process_id,pageno);
				        ++no_of_page_faults[process_id];
				        fprintf(ptr_file3,"notified scheduler about case of pagefault, i.e case 1\n");
		        	}
		        }
		    }

    }
    for(yy = 0;yy<total_no_of_processes;++yy)
    {
    	fprintf(ptr_file7,"Process : %d, Page faults: %d\n",yy,no_of_page_faults[yy]);
    }
    for(yy = 0;yy<total_no_of_processes;++yy)
    {
    	fprintf(ptr_file8,"Process : %d, invalid_references: %d\n",yy,invalid_references[yy]);
    }
    fclose(ptr_file3);
    fclose(ptr_file4);
    fclose(ptr_file5);
    fclose(ptr_file6);
    fclose(ptr_file7);
    fclose(ptr_file8);
}