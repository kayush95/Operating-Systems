/********************************************************************
Operating Systems Lab Assignment 6 : ATM - Client Transaction System
*********************************************************************/

// Master Program

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define LOCATOR_FILENAME "ATM_locator.txt"
#define SEMAPHORE_KEY 1234
#define MASTER_MSSG_Q_KEY 4321
#define MAX_SIZE 1024
#define MAX_CLIENTS 100
#define MAX_TRANSACTIONS 100
#define GLOBAL_SHARED_MEMORY_KEY 100 // Master process maintains a global shared memory space which keeps the information of the each client, updation timestamp and the current balance.

#define WITHDRAWAL 101
#define DEPOSIT 102

struct balance_record
{
	int client_accnt_no;	// nothing but the PID of the client process
	int client_balance;
	time_t last_update_timestamp;

};
struct balance_record* globl_shm_ptr;
struct balance_record* globl_shm_end_record;

struct transaction_record
{
	int client_accnt_no; // nothing but the PID of the client process
	int type;	// either WITHDRAWAL or DEPOSIT
	int amount; // $$$
	time_t timestamp;
};

struct mssg
{
	long mtype;
	char mtext[MAX_SIZE];
};

int semID,mssgQ_ID,globl_shmID;


void initializeAllSubSemaphoreValues(int n)
{
	int i;
	for(i=0;i<n;i++)
		if(semctl(semID,i,SETVAL,1)==-1)
			perror("Error in semctl SETVAL \n");
}

void createLocatorFile()
{
	FILE *fp = fopen(LOCATOR_FILENAME, "w+");
	fclose(fp);
}

void appendToLocatorFile(int a,int b,int c,int d)
{
	// Format : <ATM_ID> <mssgQueue_key> <sub_semaphore_index> <shared_memory_key>

	FILE *fp = fopen(LOCATOR_FILENAME, "a");
	fprintf(fp,"%d %d %d %d \n",a,b,c,d);
	fclose(fp);
}

void wrapup()
{
	if(semctl(semID,0,IPC_RMID,0)==-1)
		perror("Error in semctl IPC_RMID !\n");
	if(msgctl(mssgQ_ID,IPC_RMID,NULL)==-1)
		perror("Error in msgctl IPC_RMID !\n");

	if(shmctl(globl_shmID,IPC_RMID,NULL)==-1)
		perror("Error in shmctl IPC_RMID !\n");

	system("pkill atm");
	exit(0);
}

void checkClientAccount(int pid)
{
	struct balance_record* curr;

	for(curr=globl_shm_ptr;curr!=globl_shm_end_record; curr++)
	{
		if(curr->client_accnt_no==pid)
		{
			printf("Found Client Account ! AccountNo. : %d , Balance : %d, Last updated : %s\n\n\n\n",curr->client_accnt_no,curr->client_balance,asctime(localtime(&curr->last_update_timestamp)));
			return;
		}
	}

	printf("@Master : Client Account NOT FOUND ! Creating new Account...\n\n\n\n");

	struct balance_record new_accnt;
	new_accnt.client_accnt_no = pid;
	new_accnt.client_balance=0;
	new_accnt.last_update_timestamp=time(0);

	memcpy(globl_shm_end_record,&new_accnt,sizeof(struct balance_record));
	globl_shm_end_record++;
			
}

void updateAccount(int pid_accnt_no,int add_amount)
{
	struct balance_record* curr;

	for(curr=globl_shm_ptr;curr!=globl_shm_end_record; curr++)
	{
		if(curr->client_accnt_no==pid_accnt_no)
		{
			printf("		Adding %d to AccountNo. : %d \n",add_amount, curr->client_accnt_no);
			curr->client_balance += add_amount;
			curr->last_update_timestamp = time(0);
			return;
		}
	}

	printf("Error : Account PID %d Not found @ Master ! \n\n",pid_accnt_no);
	wrapup();
}

void copyGlobaltoLocalImage(struct balance_record* ptr)
{
	struct balance_record* curr;

	for(curr=globl_shm_ptr;curr!=globl_shm_end_record; curr++)
	{
		memcpy(ptr,curr,sizeof(struct balance_record));
		ptr++;
	}

}

void performGlobalConsistencyCheck()
{
	printf("\n\n\n@MASTER : Performing Global Consistency Check ...\n\n");


	FILE *fp = fopen(LOCATOR_FILENAME, "r+");
	int a,b,c,d;
	while(fscanf(fp,"%d %d %d %d",&a,&b,&c,&d)==4)
	{
		int other_key = d;

		int other_shmID = shmget((key_t)other_key,MAX_TRANSACTIONS*sizeof(struct transaction_record) + MAX_CLIENTS*sizeof(struct balance_record),0666);
		if(other_shmID==-1) perror("Error in shmget !\n");

		struct transaction_record* other_transac_ptr = shmat(other_shmID,NULL,0);
		struct transaction_record* curr_t;

		printf("Transactions @ ATM%d : \n", a);
		for(curr_t=other_transac_ptr;curr_t->client_accnt_no != -1 && curr_t!=NULL; curr_t++)
		{
					switch(curr_t->type)
					{
						case WITHDRAWAL: 
						{
							printf("	Accnt No. : %d WITHDRAWAL of %d on %s",curr_t->client_accnt_no, curr_t->amount,asctime(localtime(&curr_t->timestamp)));
							updateAccount(curr_t->client_accnt_no,(-1 * curr_t->amount));
							break;
						}
						case DEPOSIT: 
						{
							printf("	Accnt No. : %d DEPOSIT of %d on %s",curr_t->client_accnt_no, curr_t->amount,asctime(localtime(&curr_t->timestamp)));
							updateAccount(curr_t->client_accnt_no,curr_t->amount);
							break;
						}
						default:continue;
					}

			// RE-INITIALIZE the transaction record
			curr_t->client_accnt_no = -1;
				
		}
		int detach = shmdt(other_transac_ptr);
	}
	struct balance_record* local_shm_ptr;

	// COPY THE UPDATED BALANCE TABLE FROM GLOBAL SPACE TO LOCAL IMAGES @ ATMs

	rewind(fp);

	while(fscanf(fp,"%d %d %d %d",&a,&b,&c,&d)==4)
	{
		int other_key = d;

		int other_shmID = shmget((key_t)other_key,MAX_TRANSACTIONS*sizeof(struct transaction_record) + MAX_CLIENTS*sizeof(struct balance_record),0666);
		if(other_shmID==-1) perror("Error in shmget !\n");

		struct transaction_record* other_transac_ptr = shmat(other_shmID,NULL,0);
		local_shm_ptr =(struct balance_record*) (other_transac_ptr + MAX_TRANSACTIONS);

		copyGlobaltoLocalImage(local_shm_ptr);

		int detach = shmdt(other_transac_ptr);

	}

	fclose(fp);

}


void main(int argc,char* argv[])
{
	signal(SIGKILL,wrapup);
	signal(SIGINT,wrapup);
	signal(SIGTERM,wrapup);
	signal(SIGQUIT,wrapup);

	int num_ATMs,i;

	if(argc!=2)
	{
		printf("Please enter : %s <number_of_ATMs>\n", argv[0]);
		exit(0);
	}
	else
		sscanf(argv[1],"%d",&num_ATMs);

	createLocatorFile();

	semID = semget((key_t)SEMAPHORE_KEY,num_ATMs,IPC_CREAT|0666);
	if(semID==-1) perror("Error in semget !\n");

	mssgQ_ID=msgget((key_t)MASTER_MSSG_Q_KEY,IPC_CREAT|0666);
	if(mssgQ_ID==-1) perror("Error in msgget !\n");

	globl_shmID = shmget((key_t)GLOBAL_SHARED_MEMORY_KEY,MAX_CLIENTS*sizeof(struct balance_record),IPC_CREAT|0666);
	if(globl_shmID==-1) perror("Error in shmget !\n");

	globl_shm_ptr = shmat(globl_shmID,NULL,0);
	globl_shm_end_record = globl_shm_ptr; // initially NO client record

	initializeAllSubSemaphoreValues(num_ATMs);

	for(i=0;i<num_ATMs;i++)
	{
		int index,KEY; // the same KEY will be used for both the ATM - Client Message Queue as well as the shared memory of the ATM process

		int pid = fork();
		if(pid == 0)
			{			
				// ATM (forked child) process
				index = getpid() - getppid();
				KEY = 10*index;

				char buf[10];
				sprintf(buf,"%d",KEY);

				//int retv = execlp("atm.c","./atm",buf,NULL);
				int retv = execlp("./atm","./atm",buf,NULL);	
				if(retv == -1) {perror("Error in execlp ! \n"); exit(1);}
			}
		else
		{
			// master(parent) process
			index = pid - getpid();
			KEY = 10*index;

			appendToLocatorFile(index,KEY,index-1,KEY);
			
		}

		//sleep(1);	// sleep for 1 second before generating another process
	}

	while(1)
	{
		int pid;
		int atm_pid;

		struct mssg buffer;
		struct msqid_ds qstat;

		msgrcv(mssgQ_ID,&buffer,sizeof(buffer),getpid(),0);
		msgctl(mssgQ_ID,IPC_STAT,&qstat);

		printf("Master Process received message %s from ATM%d\n\n",buffer.mtext,qstat.msg_lspid-getpid());

		if(sscanf(buffer.mtext,"CHCK_ACCOUNT %d",&pid)==1)
			checkClientAccount(pid);
		else if(strcmp(buffer.mtext,"GLOBAL_CONSISTENCY_CHECK_REQUEST")==0)
		{
			performGlobalConsistencyCheck();

			buffer.mtype=qstat.msg_lspid;
			strcpy(buffer.mtext,"GLOBAL CONSISTENCY CHECK COMPLETION CONFIRMATION ");
			msgsnd(mssgQ_ID,&buffer,sizeof(buffer),0);

		}
		else printf("Invalid Message !\n\n");

	}

	wrapup();
}

/*
struct mssg buffer;
struct msqid_ds qstat;
msgrcv(mssgQ_ID,&buffer,sizeof(buffer),0,0);
msgctl(mssgQ_ID,IPC_STAT,&qstat);
//qstat.msg_lspid
msgsnd(mssgQ_ID,&buffer,sizeof(buffer),0);

*/
