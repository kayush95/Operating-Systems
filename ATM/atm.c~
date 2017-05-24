/********************************************************************
Operating Systems Lab Assignment 6 : ATM - Client Transaction System

Name : Kumar Ayush
Roll No. 13CS10058

Name : Varun Rawal
Roll No. 13CS10059

*********************************************************************/

// ATM Program

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
struct balance_record* local_shm_ptr;
struct balance_record* local_shm_end_record;


struct transaction_record
{
	int client_accnt_no; // nothing but the PID of the client process
	int type;	// either WITHDRAWAL or DEPOSIT
	int amount; // $$$
	time_t timestamp;
};
struct transaction_record* transac_ptr;
struct transaction_record* transac_end_record;


struct mssg
{
	long mtype;
	char mtext[MAX_SIZE];
};

int client_mssgQ_ID,master_mssgQ_ID,local_shmID;

void wrapup()
{
	if(msgctl(client_mssgQ_ID,IPC_RMID,NULL)==-1)
		perror("Error in msgctl IPC_RMID !\n");

	if(shmctl(local_shmID,IPC_RMID,NULL)==-1)
		perror("Error in shmctl IPC_RMID !\n");

	exit(0);
}

int performLocalConsistencyCheck(int pid_acc_no)
{
	printf("\n\n\n@ATM%d : Performing Local Consistency Check ...\n\n",getpid()-getppid());
	int base_balance = 0,transaction_profit=0;
	struct balance_record* curr;

	for(curr=local_shm_ptr;curr->client_accnt_no!=-1; curr++)
		if(curr->client_accnt_no==pid_acc_no)
			base_balance+=curr->client_balance;
	
	FILE *fp = fopen(LOCATOR_FILENAME, "r+");
	int a,b,c,d;
	while(fscanf(fp,"%d %d %d %d",&a,&b,&c,&d)==4)
	{
		//printf("Checking ATM index : %d %d %d %d\n",a,b,c,d);
		int other_key = d;

		int other_shmID = shmget((key_t)other_key,MAX_TRANSACTIONS*sizeof(struct transaction_record),0666);
		if(other_shmID==-1) perror("Error in shmget !\n");

		struct transaction_record* other_transac_ptr = shmat(other_shmID,NULL,0);
		//printf("THIS --> %p + %d = %p \n\n",other_transac_ptr,MAX_TRANSACTIONS*(int)sizeof(struct transaction_record),other_transac_ptr+MAX_TRANSACTIONS);
		struct transaction_record* curr_t;
		//printf("End @ %p \n\n",other_transac_ptr+MAX_TRANSACTIONS*sizeof(struct transaction_record));
		//sleep(5);
		printf("Transactions of %d @ ATM%d : \n",pid_acc_no, a);
		for(curr_t=other_transac_ptr;curr_t->client_accnt_no != -1 && curr_t!=NULL; curr_t++)
		{
			//printf("COMPARE %p / %p : %d ,	",curr_t, other_transac_ptr+ sizeof(struct transaction_record),  curr_t <= other_transac_ptr+ sizeof(struct transaction_record) );	

			if(curr_t->client_accnt_no==pid_acc_no)
				{
					switch(curr_t->type)
					{
						case WITHDRAWAL: 
						{
							printf("	WITHDRAWAL of %d on %s\n",curr_t->amount,asctime(localtime(&curr_t->timestamp)));
							transaction_profit-=curr_t->amount;
							break;
						}
						case DEPOSIT: 
						{
							printf("	DEPOSIT of %d on %s\n",curr_t->amount,asctime(localtime(&curr_t->timestamp)));
							transaction_profit+=curr_t->amount;
							break;
						}
						default:continue;
					}
					
				}
		}
		int detach = shmdt(other_transac_ptr);
		//printf("A3\n");

	}
	fclose(fp);
	int total_Balance = base_balance + transaction_profit; // transaction profit may be negative (loss)
	return total_Balance;
}

void performGlobalConsistencyCheck()
{
	struct mssg buffer;

	// send message to master process
	buffer.mtype=getppid();
	sprintf(buffer.mtext,"GLOBAL_CONSISTENCY_CHECK_REQUEST");
	msgsnd(master_mssgQ_ID,&buffer,sizeof(buffer),0);

	// receive conformation from master
	msgrcv(master_mssgQ_ID,&buffer,sizeof(buffer),getpid(),0);
	printf("ATM%d Received conformation from Master process : %s\n\n",getpid()-getppid(),buffer.mtext);

}

void main(int argc,char* argv[])
{
	signal(SIGKILL,wrapup);
	signal(SIGINT,wrapup);
	signal(SIGTERM,wrapup);
	signal(SIGQUIT,wrapup);

	int KEY;  // the same KEY will be used for both the ATM - Client Message Queue as well as the shared memory of the ATM process

	if(argc!=2)
	{
		printf("Please enter : %s <ATM_key>\n", argv[0]);
		exit(0);
	}
	else sscanf(argv[1],"%d",&KEY);

	printf("%d %d ATM process started : my_ID : %d , my_KEY : %d\n",getpid(),getppid(), getpid()-getppid(),KEY);

	master_mssgQ_ID=msgget((key_t)MASTER_MSSG_Q_KEY,IPC_CREAT|0666);
	if(master_mssgQ_ID==-1) perror("Error in msgget !\n");

	client_mssgQ_ID=msgget((key_t)KEY,IPC_CREAT|0666);
	if(client_mssgQ_ID==-1) perror("Error in msgget !\n");

	local_shmID = shmget((key_t)KEY,MAX_TRANSACTIONS*sizeof(struct transaction_record) + MAX_CLIENTS*sizeof(struct balance_record),IPC_CREAT|0666);
	if(local_shmID==-1) perror("Error in shmget !\n");

	transac_ptr = shmat(local_shmID,NULL,0);
	transac_end_record = transac_ptr; // initially NO transaction record

	// INITIALIZATION of transaction records
	struct transaction_record* curr_t;
	for(curr_t=transac_ptr ; curr_t!= transac_ptr + MAX_TRANSACTIONS ; curr_t++)
		curr_t->client_accnt_no = -1;

	local_shm_ptr =(struct balance_record*) (transac_ptr + MAX_TRANSACTIONS);

	// INITIALIZATION of balance records
	struct balance_record* curr;
	for(curr= local_shm_ptr ; curr!= local_shm_ptr + MAX_CLIENTS ; curr++)
		curr->client_accnt_no = -1;


	while(1)
	{
		int amount;

		struct mssg buffer;
		struct msqid_ds qstat;

		msgrcv(client_mssgQ_ID,&buffer,sizeof(buffer),ATM_RECEIVE_CODE,0);
		msgctl(client_mssgQ_ID,IPC_STAT,&qstat);

		printf("ATM%d received message %s from Client PID : %d\n\n",getpid()-getppid(),buffer.mtext,qstat.msg_lspid);

		/////////////////////////////////////////// JOINING PROTOCOL /////////////////////////////
		if(strcmp(buffer.mtext,"ENTER")==0)
		{
			buffer.mtype=getppid();
			sprintf(buffer.mtext,"CHCK_ACCOUNT %d",qstat.msg_lspid);
			msgsnd(master_mssgQ_ID,&buffer,sizeof(buffer),0);

			buffer.mtype=qstat.msg_lspid;
			sprintf(buffer.mtext,"Welcome Client %d",qstat.msg_lspid);
			msgsnd(client_mssgQ_ID,&buffer,sizeof(buffer),0);

			printf("Client PID %d logged in \n\n",qstat.msg_lspid);
		}
		//////////////////////////////////////////////////////////////////////////////////////////



		/////////////////////////////////////////// DEPOSIT /////////////////////////////////////
		else if(sscanf(buffer.mtext,"DEPOSIT %d",&amount)==1)
		{
			struct transaction_record new_transac;
			new_transac.client_accnt_no=qstat.msg_lspid;
			new_transac.type=DEPOSIT;
			new_transac.amount=amount;
			new_transac.timestamp=time(0);

			transac_end_record = transac_ptr;
			while(transac_end_record->client_accnt_no!=-1)transac_end_record++;

			memcpy(transac_end_record,&new_transac,sizeof(struct transaction_record));
			transac_end_record++;

			buffer.mtype=qstat.msg_lspid;
			sprintf(buffer.mtext,"$%d deposited into account of Client PID : %d",amount,qstat.msg_lspid);
			msgsnd(client_mssgQ_ID,&buffer,sizeof(buffer),0);

			printf("Transaction Successful \n\n");
		}
		//////////////////////////////////////////////////////////////////////////////////////////


		/////////////////////////////////////////// WITHDRAWAL /////////////////////////////////////
		else if(sscanf(buffer.mtext,"WITHDRAW %d",&amount)==1)
		{
			int total_Balance = performLocalConsistencyCheck(qstat.msg_lspid);
			if(total_Balance<amount)
			{
				buffer.mtype=qstat.msg_lspid;
				sprintf(buffer.mtext,"Sorry ! Insufficient funds to withdraw $%d ! Your current Balance : $%d",amount,total_Balance);
				msgsnd(client_mssgQ_ID,&buffer,sizeof(buffer),0);	
				continue;
			}

			struct transaction_record new_transac;
			new_transac.client_accnt_no=qstat.msg_lspid;
			new_transac.type=WITHDRAWAL;
			new_transac.amount=amount;
			new_transac.timestamp=time(0);

			transac_end_record = transac_ptr;
			while(transac_end_record->client_accnt_no!=-1)transac_end_record++;

			memcpy(transac_end_record,&new_transac,sizeof(struct transaction_record));
			transac_end_record++;

			buffer.mtype=qstat.msg_lspid;
			sprintf(buffer.mtext,"$%d withdrawed from account of Client PID : %d",amount,qstat.msg_lspid);
			msgsnd(client_mssgQ_ID,&buffer,sizeof(buffer),0);

			printf("Transaction Successful \n\n");
		}
		//////////////////////////////////////////////////////////////////////////////////////////


		/////////////////////////////////////////// WITHDRAWAL /////////////////////////////////////
		else if(strcmp(buffer.mtext,"VIEW BALANCE")==0)
		{
			performGlobalConsistencyCheck();

			struct balance_record* curr;
			int total_Balance=-1;
			time_t updated;

			for(curr=local_shm_ptr;curr->client_accnt_no!=-1; curr++)
				if(curr->client_accnt_no== qstat.msg_lspid)
					{total_Balance = curr->client_balance; updated = curr->last_update_timestamp;}

			buffer.mtype=qstat.msg_lspid;
			sprintf(buffer.mtext,"Current Account Balance of Client PID %d :\n $%d \n Last Updated : %s",qstat.msg_lspid,total_Balance,asctime(localtime(&updated)));
			msgsnd(client_mssgQ_ID,&buffer,sizeof(buffer),0);

			printf("Balance VIEW Successful \n\n");
		}
		//////////////////////////////////////////////////////////////////////////////////////////


		else 
		{
			printf("Cannot process message !\n\n");

			buffer.mtype=qstat.msg_lspid;
			sprintf(buffer.mtext,"Transaction Ignored !");
			msgsnd(client_mssgQ_ID,&buffer,sizeof(buffer),0);
		}
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