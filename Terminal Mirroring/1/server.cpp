#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <pthread.h>

#define MSGSZ 1000
#define KEY 1234
#define COUPLE 1
#define UNCOUPLE 2
#define CLIENT2SERVER 3

using namespace std;

/* 
 * Declare the message structure.
 */

vector<int> id_list;

typedef struct mssgbuff 
{
    long mtype;
    char cmd[20];
    char mtext[MSGSZ];
    int terminalID;
}
message_buf;

int createQueue()
{
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;

    /*
     * Get the message queue id for the
     * "name" 1234, which was created by
     * the server.
     */

    key = KEY;
    cout << "mssgget : Calling msgget("<<key<<","<<msgflg<<")"<<endl;

    if ((msqid = msgget(key, msgflg )) < 0) 
    {
        perror("msgget");
        exit(1);
    }
    else 
    (void) fprintf(stderr,"msgget: msgget succeeded: msqid = %d\n", msqid);

    return msqid;
}

void* couple(void* qid)
{

	int *q = (int*)qid;
    	int msqid = *q;


	while(1)
	{

		message_buf sbuf;
		message_buf rbuf;
		message_buf rbuf_couple;
		message_buf rbuf_uncouple;
		size_t sbuf_length;
		size_t rbuf_length;

		if(msgrcv(msqid, &rbuf_couple, sizeof(message_buf) - sizeof(long) , COUPLE, 0) < 0)
		{
			cout << "No coupling message received!\n";
			continue;
		}
		else
		{
			if(strcmp(rbuf_couple.mtext,"couple") == 0)
			{
				id_list.push_back(rbuf_couple.terminalID);
				sbuf.mtype = 5000 + rbuf_couple.terminalID + COUPLE;
				strcpy(sbuf.mtext,"");
				sbuf.terminalID = rbuf_couple.terminalID;
				sbuf_length = sizeof(message_buf) - sizeof(long);
				if(msgsnd(msqid, &sbuf, sbuf_length, 0) < 0)
					cout << "msgsnd error\n";
				else
					cout << "msgsnd Couplin from server succesful!\n";
			}
		
			cout << "Updated List :" << endl;
			for(int i = 0; i < id_list.size(); i++)
			{
				cout<<"ID["<<i<<"] = "<<id_list[i]<<endl;
			}	
		}
	}
}

void* uncouple(void* qid)
{
	int *q = (int*)qid;
    	int msqid = *q;

 
	while(1)
	{

		message_buf sbuf;
		message_buf rbuf;
		message_buf rbuf_couple;
		message_buf rbuf_uncouple;
		size_t sbuf_length;
		size_t rbuf_length;

		if(msgrcv(msqid, &rbuf_uncouple, sizeof(message_buf) - sizeof(long), UNCOUPLE, 0) < 0)
		{	
			cout << "Uncouple msgrcv error!\n";
			continue;
		}
		else
		{
			if(strcmp(rbuf_uncouple.mtext,"uncouple") == 0)
			{

				for(int i = 0; i < id_list.size(); i++)
				{
					if(id_list[i] == rbuf_uncouple.terminalID)
						cout << "Uncoupled ID : "<< rbuf_uncouple.terminalID << endl;
					if(id_list[i] != rbuf_uncouple.terminalID)
					{
						sbuf.mtype = 1000 + id_list[i];
						strcpy(sbuf.cmd,"uncouple");
						strcpy(sbuf.mtext, rbuf.mtext);
						sbuf.terminalID = rbuf_uncouple.terminalID;
						sbuf_length = sizeof(message_buf) - sizeof(long);
						if(msgsnd(msqid, &sbuf, sbuf_length, 0) < 0)
							cout << "Uncouple msgBroadcast error!\n";				
					}
				}
		
				int id_to_remove = rbuf_uncouple.terminalID;
				cout << "After removal Uncoupled ID : "<< rbuf_uncouple.terminalID << endl;
				id_list.erase(remove(id_list.begin(), id_list.end(), id_to_remove), id_list.end());
				cout << "Updated List :" << endl;
				if(id_list.size() == 0)
					cout << "No couples\n";
				for(int i = 0; i < id_list.size(); i++)
				{
					cout<<"ID["<<i<<"] = "<<id_list[i]<<endl;
				}	

			}
		}
	}

}

void* message(void *qid)
{
	
	int *q = (int*)qid;
   	int msqid = *q;
        message_buf sbuf;
        message_buf rbuf;
        message_buf rbuf_couple;
        message_buf rbuf_uncouple;
        size_t sbuf_length;
        size_t rbuf_length;

	while(1)
	{
		if(msgrcv(msqid, &rbuf, sizeof(message_buf) - sizeof(long), CLIENT2SERVER, 0) < 0)
		{  	
			cout << "Did not enter\n";
			continue;
		}
		else
		{
			for(int i = 0; i < id_list.size(); i++)
			{
				if(id_list[i] != rbuf.terminalID)
				{
					sbuf.mtype = 1000 + id_list[i];
					strcpy(sbuf.cmd,rbuf.cmd);
					strcpy(sbuf.mtext, rbuf.mtext);
					sbuf.terminalID = rbuf.terminalID;
					sbuf_length = sizeof(message_buf) - sizeof(long);
					msgsnd(msqid, &sbuf, sbuf_length, 0);				
				}
			}		
		}
	}

}


main()
{
    int msqid;
    msqid = createQueue();
    cout << "Main function - msqid =" << msqid<<endl;


    pthread_t threads[3];
    int rc;

    long i;
    i = 0;
    rc = pthread_create(&threads[i], NULL, couple, (void*)&msqid);
    if (rc)
    {
   	printf("Error:unable to create thread, %d\n",rc);
   	exit(-1);
    }

    i = 1;
    rc = pthread_create(&threads[i], NULL, uncouple, (void*)&msqid);
    if (rc)
    {
   	printf("Error:unable to create thread, %d\n",rc);
   	exit(-1);
    }

    i = 2;
    rc = pthread_create(&threads[i], NULL, message, (void*)&msqid);
    if (rc)
    {
   	printf("Error:unable to create thread, %d\n",rc);
   	exit(-1);
    }

    pthread_exit(NULL);     
   
    exit(0);
}





//inside main

    /*message_buf sbuf;
    message_buf rbuf;
    message_buf rbuf_couple;
    message_buf rbuf_uncouple;
    size_t sbuf_length;
    size_t rbuf_length;

    while(1)
    {
	if(msgrcv(msqid, &rbuf_couple, sizeof(message_buf) - sizeof(long) , COUPLE, 0) < 0)
		continue;
        else
	{
		if(strcmp(rbuf_couple.mtext,"couple") == 0)
		{
			id_list.push_back(rbuf_couple.terminalID);
			sbuf.mtype = 1000 + rbuf_couple.terminalID;
			strcpy(sbuf.mtext,"");
			sbuf.terminalID = rbuf_couple.terminalID;
			sbuf_length = sizeof(message_buf) - sizeof(long);
			msgsnd(msqid, &sbuf, sbuf_length, 0);
		}
		
		cout << "Updated List :" << endl;
		for(int i = 0; i < id_list.size(); i++)
		{
			cout<<"ID["<<i<<"] = "<<id_list[i]<<endl;
		}	
	}
	
	if(msgrcv(msqid, &rbuf_uncouple, sizeof(message_buf) - sizeof(long), UNCOUPLE, 0) < 0)
		continue;
        else
	{
		if(strcmp(rbuf_uncouple.mtext,"uncouple") == 0)
		{

			for(int i = 0; i < id_list.size(); i++)
		        {
				if(id_list[i] != rbuf_uncouple.terminalID)
				{
					sbuf.mtype = 1000 + id_list[i];
					strcpy(sbuf.mtext, rbuf.mtext);
					sbuf.terminalID = id_list[i];
					sbuf_length = sizeof(message_buf) - sizeof(long);
					msgsnd(msqid, &sbuf, sbuf_length, 0);				
				}
		        }
		
			int id_to_remove = rbuf_uncouple.terminalID;
			id_list.erase(remove(id_list.begin(), id_list.end(), id_to_remove), id_list.end());
			cout << "Updated List :" << endl;
			if(id_list.size() == 0)
				cout << "No couples\n";
			for(int i = 0; i < id_list.size(); i++)
			{
				cout<<"ID["<<i<<"] = "<<id_list[i]<<endl;
			}	

		}
	}

	if(msgrcv(msqid, &rbuf, sizeof(message_buf) - sizeof(long), CLIENT2SERVER, 0) < 0)
		continue;
	else
	{
		for(int i = 0; i < id_list.size(); i++)
		{
			if(id_list[i] != rbuf.terminalID)
			{
				sbuf.mtype = 1000 + id_list[i];
				strcpy(sbuf.mtext, rbuf.mtext);
				sbuf.terminalID = id_list[i];
				sbuf_length = sizeof(message_buf) - sizeof(long);
				msgsnd(msqid, &sbuf, sbuf_length, 0);				
			}
		}		
	}
    }*/
     

