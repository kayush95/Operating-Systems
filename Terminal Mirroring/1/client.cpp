#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <vector>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <string>
#include <cstdlib>
#include <pthread.h>

#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <time.h>
#include <termios.h>
#include <ctype.h>

#define MSGSZ 1000
#define KEY 1234
#define COUPLE 1
#define UNCOUPLE 2
#define CLIENT2SERVER 3

using namespace std;

int mode = 0;
// 0 - uncoupled
// 1 - coupled

/*Declare the message structure.*/
typedef struct mssgbuff 
{
    long mtype;
    char cmd[20];
    char mtext[MSGSZ];
    int terminalID;
} 
message_buf;

int changeDirectory(char* s)
{
     char cwd[100];
     {
	if(s==NULL)
		chdir("/home");
	else
	{
		if(getcwd(cwd, 100) != NULL && strcmp(cwd,"/home")==0 && strcmp(s,"..")==0);
		else 
			return chdir(s);
	}
     }

     return 1;
}

int makeDirectory(char* s)
{
	return mkdir(s,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

int removeDirectory(char* s)
{
	return rmdir(s);
}


int connectQueue()
{
    int msqid;
    key_t key;

    key = KEY;
    

    if ((msqid = msgget(key, 0666 )) < 0) 
    {
        perror("msgget");
        exit(1);
    }
    else cout << "Successfully connected to msqid = " << msqid << endl;
    return msqid;
}

void couple(int msqid)
{
    message_buf  sbuf;
    message_buf  rbuf;

    int pid = getpid();
    
    sbuf.mtype = COUPLE;
    strcpy(sbuf.mtext, "couple");
    sbuf.terminalID = pid;
    cout << "PID before coupling : " << pid << endl;
    size_t sbuf_length = sizeof(message_buf) - sizeof(long);
    if(msgsnd(msqid, &sbuf, sbuf_length, 0) < 0)
    {
        perror("msgsnd error");
        exit(1);
    }	
    else
    	cout << "Succesful coupling message sent!\n";
    cout << "Entering coupling msgrcv region!\n";
    if(msgrcv(msqid, &rbuf, sizeof(message_buf) - sizeof(long) , 5000+pid + COUPLE, 0) < 0)
    {
        perror("msgrcv error");
        exit(1);	
    }
    else 
	cout << "Succesful Coupling!\n";
    cout << "Leaving coupling msgrcv region!\n";

    cout << "ID : " << rbuf.terminalID << endl;
    cout << "PID : " << pid << endl;
}

void uncouple(int msqid)
{
    message_buf  sbuf;

    int pid = getpid();
    cout << "PID = " << pid << endl;

    sbuf.mtype = UNCOUPLE;
    strcpy(sbuf.mtext, "uncouple");
    sbuf.terminalID = pid;
    size_t sbuf_length = sizeof(message_buf) - sizeof(long);
    if(msgsnd(msqid, &sbuf, sbuf_length, 0) < 0)
    {
        perror("msgsnd error");
        exit(1);
    }
    else
	cout << "Uncouple message sent succesfully!\n";	

}


void* input_and_send(void* qid)
{
    int *q = (int*)qid;
    int msqid = *q;
    cout << "input_out_send - msqid = " << msqid << endl;
    char command[MSGSZ];
    char cwd[100];
    int pid_main;
   
    do
    {	    
  	    
	    if(getcwd(cwd, 100) != NULL)
		cout << cwd << ">";
	    else
	    {
		printf("Command Prompt error ! Terminating...\n ");
		exit(1);
	    }
	    
	    
	    memset(command, 0, MSGSZ);
	    //fgets(command,MSGSZ,stdin);

	    if(fgets(command, MSGSZ, stdin)==NULL)
		continue;
			   
     	    if(strcmp(command,"\n") == 0)
		continue;

            char cmmd[MSGSZ];
	    strcpy(cmmd,command);
	    char **args = (char**)malloc(1000*sizeof(char*));
	    char** p =args;
	    *p=strtok(cmmd, " \t\n");

	     while(*p!=NULL)
	     {
		p++;
		*p=strtok(NULL, " \t\n");
	     }


	    if(strcmp(command,"couple\n") == 0)
	    {
		couple(msqid);
   		mode = 1;
	    }
	    else if(strcmp(command,"uncouple\n") == 0)
	    {
		uncouple(msqid);
		mode = 0;
            }
	    else if(strcmp(command,"clear\n") == 0)
	    {
		system("clear");
		message_buf sbuf;
		sbuf.mtype = CLIENT2SERVER;
		strcpy(sbuf.cmd, command);
		strcpy(sbuf.mtext, "");
		int x = getpid();
		sbuf.terminalID = x;
		
		if(mode == 1)
		{	
			cout << "mode = "<< 1 <<endl;
			if(msgsnd(msqid, &sbuf, sizeof(message_buf) - sizeof(long), 0) < 0)
				perror("msgsnd error\n");
			else
				cout << "Succcess!\n";
		}				
	    }
	    else if(strcmp(args[0],"mkdir")==0)
	    {

		message_buf sbuf;

		strcpy(sbuf.mtext, "");

		if(args[1]==NULL)
		{
			printf("Invalid Command ! Please enter : mkdir <dir_name>\n");
			strcpy(sbuf.mtext, "Invalid Command ! Please enter : mkdir <dir_name>\n");
		}
		else if(makeDirectory(args[1])==-1)
		{
			printf("ERROR in creating directory !\n");
			strcpy(sbuf.mtext, "ERROR in creating directory !\n");
		}

		sbuf.mtype = CLIENT2SERVER;
		strcpy(sbuf.cmd, command);
		
		int x = getpid();
		sbuf.terminalID = x;
		
		if(mode == 1)
		{	
			cout << "mode = "<< 1 <<endl;
			if(msgsnd(msqid, &sbuf, sizeof(message_buf) - sizeof(long), 0) < 0)
				perror("msgsnd error\n");
			else
				cout << "Succcess!\n";
		}	

	    }
	    else if(strcmp(args[0],"rmdir")==0)
	    {
		message_buf sbuf;

		strcpy(sbuf.mtext, "");

		if(args[1]==NULL)
		{
			printf("Invalid Command ! Please enter : rmdir <dir_name>\n");
			strcpy(sbuf.mtext, "Invalid Command ! Please enter : rmdir <dir_name>\n");
		}
		else if(removeDirectory(args[1])==-1)
		{
			printf("ERROR in removing directory !\n");
			strcpy(sbuf.mtext, "ERROR in removing directory !\n");
		}

		
		sbuf.mtype = CLIENT2SERVER;
		strcpy(sbuf.cmd, command);
		
		int x = getpid();
		sbuf.terminalID = x;
		
		if(mode == 1)
		{	
			cout << "mode = "<< 1 <<endl;
			if(msgsnd(msqid, &sbuf, sizeof(message_buf) - sizeof(long), 0) < 0)
				perror("msgsnd error\n");
			else
				cout << "Succcess!\n";
		}	
	    }
    	    else if((strcmp(args[0],"cd")) == 0)
	    {
		/*char cmmd[MSGSZ];
		strcpy(cmmd,command);
	 	char **args = (char**)malloc(1000*sizeof(char*));
		char** p =args;
		*p=strtok(command, " \t\n");

		while(*p!=NULL)
		{
			p++;
			*p=strtok(NULL, " \t\n");
		}

		if(changeDirectory(args[1])==-1)
		        printf("ERROR : No such directory exists!\n");*/

		
		message_buf sbuf;
		sbuf.mtype = CLIENT2SERVER;
		strcpy(sbuf.cmd, command);
		strcpy(sbuf.mtext, "");
		int x = getpid();
		sbuf.terminalID = x;
		
		if(mode == 1)
		{	
			cout << "mode = "<< 1 <<endl;
			if(msgsnd(msqid, &sbuf, sizeof(message_buf) - sizeof(long), 0) < 0)
				perror("msgsnd error\n");
			else
				cout << "Succcess!\n";
		}				
		
	    }
	    else if(strcmp(command,"exit\n") == 0)
	    {
		uncouple(msqid);
	    }
	    /*else if(strcmp(command,"env\n") == 0)
	    {
		int fd[2];
		pipe(fd);

		pid_main = fork();
		
		if(pid_main == 0)
		{
			close(fd[0]);
			close(1);
			dup(fd[1]);
			close(fd[1]);
			system(command);
			exit(0);
		}
		else
		{

			char buffer[1000];
			int number_of_bytes;
			strcpy(buffer,"");
			number_of_bytes = read(fd[0], buffer, 1000);
			cout << buffer;

			message_buf sbuf;
			sbuf.mtype = CLIENT2SERVER;
			strcpy(sbuf.cmd, command);
			strcpy(sbuf.mtext, buffer);
			int x = getpid();
			sbuf.terminalID = x;
			strcpy(buffer,"");

			cout << x <<endl;

			if(mode == 1)
			{	
				cout << "mode = "<< 1 <<endl;
				if(msgsnd(msqid, &sbuf, sizeof(message_buf) - sizeof(long), 0) < 0)
					perror("msgsnd error\n");
				else
					cout << "Succcess!\n";
			}		
		}
	    }*/
	    else
	    {		
		int fd[2];
		pipe(fd);

		pid_main = fork();
		
		if(pid_main == 0)
		{
			close(fd[0]);
			close(1);
			dup(fd[1]);
			close(fd[1]);
/////////////////////////////////////////////////////////////////
   			int ret;		
			ret=execvp(args[0],args);
			printf("Invalid command !\n");					
/////////////////////////////////////////////////////////////////
			//system(command);
			exit(0);
		}
		else
		{
/////////////////////////////////////////////////////////////////
			int count=0;
			while(args[count]!=NULL)
				count++;

			if(strcmp(args[count-1],"&")==0) ;
			else wait(NULL);
/////////////////////////////////////////////////////////////////
			char buffer[1000];
			int number_of_bytes;
			strcpy(buffer,"");
			number_of_bytes = read(fd[0], buffer, 1000);
			close(fd[0]);
			close(fd[1]);
			cout << buffer;

			message_buf sbuf;
			sbuf.mtype = CLIENT2SERVER;
			strcpy(sbuf.cmd, command);
			strcpy(sbuf.mtext, buffer);
			int x = getpid();
			sbuf.terminalID = x;
			strcpy(buffer,"");

			cout << x <<endl;

			if(mode == 1)
			{	
				cout << "mode = "<< 1 <<endl;
				if(msgsnd(msqid, &sbuf, sizeof(message_buf) - sizeof(long), 0) < 0)
					perror("msgsnd error\n");
				else
					cout << "Succcess!\n";
			}		
		}
	    }	    
    }
    while(strcmp(command,"exit\n") != 0);
    exit(0);
}


void* receive(void* qid)
{
    int *q = (int*)qid;
    int msqid = *q;
    char cwd[100];
    while(1)
    {
	if(mode == 1)
	{
		
		int pid = getpid();
		message_buf rbuf;
		if(msgrcv(msqid, &rbuf, sizeof(message_buf) - sizeof(long) , 1000 + pid , 0) > 0)
		{
			cout << "\nTerminal " << rbuf.terminalID << " : " << rbuf. cmd << endl;
			cout << rbuf.mtext << endl;
			
			/*cout << "Entering cwd!\n";
			if(getcwd(cwd, 100) != NULL)
			{	
				cout << "Entered cwd!\n";			
				cout << cwd << ">";
				//cout << "Entered cwd!\n";
			}
		        else
		        {
				printf("Command Prompt error ! Terminating...\n ");
				exit(1);
		        }*/
		}
		else
			cout << "Unsuccesful receive\n";
	}
    }
}



main()
{
    int msqid; 
    msqid = connectQueue();

    pthread_t threads[2];
    int rc;

    long i;
    i = 0;
    rc = pthread_create(&threads[i], NULL, input_and_send, (void*)&msqid);
    i = 1;

    if (rc)
    {
   	printf("Error:unable to create thread, %d\n",rc);
   	exit(-1);
    }

    rc = pthread_create(&threads[i], NULL, receive, (void*)&msqid);

    if (rc)
    {
   	printf("Error:unable to create thread, %d\n",rc);
   	exit(-1);
    }

    pthread_exit(NULL);       
}
