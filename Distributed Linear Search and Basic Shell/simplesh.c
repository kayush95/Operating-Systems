/***************************************************************
Operating Systems Lab Assignment 1 : Problem 2

Name : Kumar Ayush
Roll No. 13CS10058

Name : Varun Rawal
Roll No. 13CS10059

****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <string.h>
#include <sys/stat.h>


void main()
{
	while(1)
	{
		char cwd[500];

		if(getcwd(cwd, 500) != NULL)
		printf("%s > ",cwd);
		
		else{
		printf("prompt error !\n ");
		exit(0);
		}

		char buf[1000];

		if(fgets(buf, 500, stdin)==NULL)
			continue;

		if(strlen(buf)<=1)
			continue;

		FILE* _out_=fopen("commands.txt","a+");

				if(_out_ == NULL)
				{
					printf("Cannot open file ! Terminating...\n");
					exit(1);
				}

				char sent[1000];
				int t=0;

				while(fgets(sent, sizeof(sent), _out_)!=NULL)
				{
					char *temp=strtok(sent," \t\n");
					t=atoi(temp);
				}

				fprintf(_out_, "%d %s",t+1,buf );
				fclose(_out_);

		char **args = (char**)malloc(500*sizeof(char*));

		char** p =args;

		*p=strtok(buf, " \t\n");

		while(*p!=NULL)
		{
			p++;
			*p=strtok(NULL, " \t\n");
		}

		if(args==NULL)
			continue;

		if(strcmp(args[0],"clear")==0)
			system("clear");
		else if(strcmp(args[0],"env")==0)
			system("env");
		else if(strcmp(args[0],"exit")==0)
			exit(0);
		else if(strcmp(args[0],"pwd")==0)
			{
				char cwd[500];

				if(getcwd(cwd, 500) != NULL)
				printf("%s \n",cwd);
			
				else{
				printf("prompt error !\n ");
				exit(0);
				}
			}
		else if(strcmp(args[0],"cd")==0)
			{
				if(args[1]==NULL)
					chdir("/home");
				else
				{
					char cwd[500];

				if(getcwd(cwd, 500) != NULL && strcmp(cwd,"/home ")==0)
				{}
				else
					chdir(args[1]);
				}
			}
		
			else if(strcmp(args[0],"mkdir")==0)
			{
				if(args[1]==NULL)
					printf("Invalid Command !\n");
				else
					mkdir(args[1],S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			}
			else if(strcmp(args[0],"rmdir")==0)
			{
				if(args[1]==NULL)
					printf("Invalid Command !\n");
				else
					rmdir(args[1]);
			}
			else if(strcmp(args[0],"ls")==0)
			{
				if(args[1]==NULL)
					system("ls");
				else if(strcmp(args[1],"-l")==0)
					system("ls -l");
			}
			else if(strcmp(args[0],"history")==0)
			{

				if(args[1]==NULL)
				{


				FILE* _inp_=fopen("commands.txt","r");

				if(_inp_ == NULL)
				{
					printf("Cannot open file ! Terminating...\n");
					exit(1);
				}

				char sent[1000];
				
				while(fgets(sent, sizeof(sent), _inp_)!=NULL)
				{
					printf("%s",sent );
				}

				fclose(_inp_);
			}

			else
			{
				int hist_arg=atoi(args[1]);

				if(hist_arg==0)
					printf("Invalid Command !\n");
				else
				{

					FILE* _inp_=fopen("commands.txt","r");

				if(_inp_ == NULL)
				{
					printf("Cannot open file ! Terminating...\n");
					exit(1);
				}

				char sent[1000];
				int count=0,tot_count=0;
				
				while(fgets(sent, sizeof(sent), _inp_)!=NULL)
				{
					//printf("%s",sent );
					tot_count++;
				}
				rewind(_inp_);
				while(fgets(sent, sizeof(sent), _inp_)!=NULL)
				{
					
					count++;
					if(count>tot_count - hist_arg)
						printf("%s",sent );
				}

				fclose(_inp_);

				}
			}


			}
		else  

		{

    		int pid = fork();
    		int ret;

			if(pid == 0){
				ret=execvp(args[0],args);
			}

			int count=0;
			while(args[count]!=NULL)
				count++;


			if(strcmp(args[count-1],"&")==0) {
				//printf("Backgrnd prcss\n");
				
			}
			else
			{
				wait(NULL);	
				//printf("NON-Backgrnd prcss\n");
			}

			if(ret==-1)
				printf("Invalid command !\n");

			
		}
	
	}
}