/********************************************************************
Operating Systems Lab Assignment 6 : ATM - Client Transaction System

Name : Kumar Ayush
Roll No. 13CS10058

Name : Varun Rawal
Roll No. 13CS10059

*********************************************************************/

// Client Program

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
#include <sys/shm.h>

#define ATM_RECEIVE_CODE 1
#define LOCATOR_FILENAME "ATM_locator.txt"
#define SEMAPHORE_KEY 1234
#define MAX_SIZE 1024

int semID;

struct mssg
{
	long mtype;
	char mtext[MAX_SIZE];
};

int getNoofATMs()
{
	FILE *fp = fopen(LOCATOR_FILENAME, "r+");
	int a,b,c,d,N=0;
	while(fscanf(fp,"%d %d %d %d",&a,&b,&c,&d)==4)N++;
	fclose(fp);
	return N;
}

void populateATMInfo(int* al,int* bl,int* cl)
{
	FILE *fp = fopen(LOCATOR_FILENAME, "r+");
	int a,b,c,d,i=0;
	while(fscanf(fp,"%d %d %d %d",&a,&b,&c,&d)==4)
	{
		al[i]=a;
		bl[i]=b;
		cl[i]=c;
		i++;
	}
	fclose(fp);
}

int try_enter_ATM(int sub_sema_index)
{
	struct sembuf sem_operator;

	sem_operator.sem_num = sub_sema_index;
	sem_operator.sem_op = -1;
	sem_operator.sem_flg = IPC_NOWAIT; // non-blocking wait, if already 0

	int ret = semop(semID,&sem_operator,1);
	return ret;
}


void leave_ATM(int sub_sema_index)
{
	struct sembuf sem_operator;

	sem_operator.sem_num = sub_sema_index;
	sem_operator.sem_op = +1;
	sem_operator.sem_flg = 0;

	semop(semID,&sem_operator,1);
}


void main()
{
	int num_ATMs = getNoofATMs();
	
	int ATM_indx[num_ATMs];
	int ATM_sem[num_ATMs];
	int ATM_mssgQkey[num_ATMs];

	populateATMInfo(ATM_indx,ATM_mssgQkey,ATM_sem);

	semID = semget((key_t)SEMAPHORE_KEY,num_ATMs,0666);

	int entered=0;
	char buf[100];

	int index;
	while(1)
	{
		if(entered)
		{
			int amount;

			printf("Please type one of the following commands : \n \"WITHDRAW <$$$>\" \n \"DEPOSIT <$$$>\" \n \"VIEW BALANCE\" \n \"LEAVE\" \n\n");
			scanf("%[^\n]s",buf);
			getchar();

			if(strcmp(buf,"LEAVE")==0)
			{
				leave_ATM(ATM_sem[index-1]);
				printf("\nExited ATM%d\n\n",index);
				entered=0;
				// send message to ATM
			}
			else if(strcmp(buf,"VIEW BALANCE")==0)
			{
				printf("Requesting ATM%d for balance check...\n\n",index);
				
				// send message to ATM
					int ATM_mssgQ_ID=msgget((key_t)ATM_mssgQkey[index-1],0666);
					if(ATM_mssgQ_ID==-1) perror("Error in msgget !\n");
					
					struct mssg buffer;

					buffer.mtype=ATM_RECEIVE_CODE;
					strcpy(buffer.mtext,buf);
					msgsnd(ATM_mssgQ_ID,&buffer,sizeof(buffer),0);

					msgrcv(ATM_mssgQ_ID,&buffer,sizeof(buffer),getpid(),0);
					printf("ATM%d : %s \n\n",index, buffer.mtext);
			}
			else if(sscanf(buf,"WITHDRAW %d",&amount)==1)
			{
				printf("Requesting ATM%d for withdrawal of $%d...\n\n",index,amount);

				// send message to ATM
					int ATM_mssgQ_ID=msgget((key_t)ATM_mssgQkey[index-1],0666);
					if(ATM_mssgQ_ID==-1) perror("Error in msgget !\n");
					
					struct mssg buffer;

					buffer.mtype=ATM_RECEIVE_CODE;
					strcpy(buffer.mtext,buf);
					msgsnd(ATM_mssgQ_ID,&buffer,sizeof(buffer),0);

					msgrcv(ATM_mssgQ_ID,&buffer,sizeof(buffer),getpid(),0);
					printf("ATM%d : %s \n\n",index, buffer.mtext);

			}
			else if(sscanf(buf,"DEPOSIT %d",&amount)==1)
			{
				printf("Requesting ATM%d for deposit of $%d...\n\n",index,amount);
				
				// send message to ATM
					int ATM_mssgQ_ID=msgget((key_t)ATM_mssgQkey[index-1],0666);
					if(ATM_mssgQ_ID==-1) perror("Error in msgget !\n");
					
					struct mssg buffer;

					buffer.mtype=ATM_RECEIVE_CODE;
					strcpy(buffer.mtext,buf);
					msgsnd(ATM_mssgQ_ID,&buffer,sizeof(buffer),0);

					msgrcv(ATM_mssgQ_ID,&buffer,sizeof(buffer),getpid(),0);
					printf("ATM%d : %s \n\n",index, buffer.mtext);
			}
			else printf("Plz enter command according to format !\n\n");	
		}
		else
		{
			index=-1;

			printf("Please type command : \"ENTER ATM<n>\" (n = 1 to %d) to enter an ATM \n\n",num_ATMs);
			scanf("%[^\n]s",buf);
			getchar();
			if(sscanf(buf,"ENTER ATM%d",&index)!=1 || index>num_ATMs)
				printf("Plz enter command according to format !\n\n");
			else
			{
				if(try_enter_ATM(ATM_sem[index-1])==-1)
					printf("Sorry ! ATM%d occupied \n\n",index);
				else
				{
					
					int ATM_mssgQ_ID=msgget((key_t)ATM_mssgQkey[index-1],0666);
					if(ATM_mssgQ_ID==-1) perror("Error in msgget !\n");
					
					struct mssg buffer;

					buffer.mtype=ATM_RECEIVE_CODE;
					strcpy(buffer.mtext,"ENTER");
					msgsnd(ATM_mssgQ_ID,&buffer,sizeof(buffer),0);

					system("clear");

					msgrcv(ATM_mssgQ_ID,&buffer,sizeof(buffer),getpid(),0);
					printf("ATM%d : %s \n\n",index, buffer.mtext);
					
					entered=1;
					printf("\nEntered ATM%d\n\n",index);
				}
			}
		}
	}

}