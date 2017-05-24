/***************************************************************
Operating Systems Lab Assignment 2 : Problem 2

****************************************************************/

// ADVANCED SHELL


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <time.h>
#include <termios.h>
#include <ctype.h>

#define disk_blk_size 512
#define file_blk_size 1024
#define MAX_SIZE 1024
extern char **environ ;

static struct termios old, new;

/* Initialize new terminal i/o settings */
void initTermios(int echo) 
{
  tcgetattr(0, &old); /* grab old terminal i/o settings */
  new = old; /* make new settings same as old settings */
  new.c_lflag &= ~ICANON; /* disable buffered i/o */
  new.c_lflag &= echo ? ECHO : ~ECHO; /* set echo mode */
  tcsetattr(0, TCSANOW, &new); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void) 
{
  tcsetattr(0, TCSANOW, &old);
}

/* Read 1 character - echo defines echo mode */
char getch_(int echo) 
{
  char ch;
  initTermios(echo);
  ch = getchar();
  resetTermios();
  return ch;
}

/* Read 1 character without echo */
char getch(void) 
{
  return getch_(0);
}

/* Read 1 character with echo */
char getche(void) 
{
  return getch_(1);
}

int startsWith(char *pre,char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}



char next_command[MAX_SIZE]="";

char history_file_path[MAX_SIZE];	// stores the full pathname of the command history file

void loop_SHELL();
void run(char**);

void searchHandler(int sig)
{
	
	FILE* _inp_=fopen(history_file_path,"r");
	char search_result[MAX_SIZE];

		if(_inp_ == NULL)
		{
			printf("Cannot open file ! Terminating...\n");
			return;
		}

		char sent[MAX_SIZE];

		int tot_count=0;

		while(fgets(sent, MAX_SIZE, _inp_)!=NULL)
			tot_count++;

	char prompt[MAX_SIZE]="";
	char ch='a';

	int found=1;

	while(ch!='\n')
	{
		
		clrscreen();

		if(strcmp(prompt,"")==0)
			strcpy(search_result,"");

		if(found)
		printf("\n (Reverse-Search) - [\"%s\"] > %s",prompt,search_result);
		else
			printf("\n (Reverse-Search) - [\"%s\"] (FAILED !)> %s\n",prompt,search_result);
		
			ch=getch();

		if(ch=='\n')
			break;

		if (ch == 127)
		{
			if(strcmp(prompt,"")!=0)
			prompt[strlen(prompt)-1]='\0';
		}
		else if (ch>=1 && ch<=255)
			{
				char temp[MAX_SIZE];
				sprintf(temp,"%c",ch);
				strcat(prompt,temp);
			}

		// !!! ADD MORE KEY FUNCTIONS, ASSOCIATING THEM WITH THEIR CORRESPONDING ASCII VALUES
	
		int mark=0,counter=0;
		char* ptr;

		rewind(_inp_);
		found=0;

		while(fgets(sent,MAX_SIZE,_inp_)!=NULL)
		{	
			counter++;
			int i=0;
			
			while(!isalpha(sent[i]) && sent[i]!='\0') i++;

				ptr = sent+i;
				if(startsWith(prompt,ptr))
				{
					found=1;
					mark=counter;
					//printf("PRE : %s\n",prompt );
					//printf("SENTENCE %s\n", ptr);
				}
		}

	if(found){
		rewind(_inp_);

		for(counter=0;counter<mark;counter++)
			fgets(sent,MAX_SIZE,_inp_);

		int i=0;
			
		while(!isalpha(sent[i]) && sent[i]!='\0') i++;

		ptr=sent+i;

		strcpy(search_result,ptr);
	}
	else
		strcpy(search_result,"");

	}

	strcpy(next_command,search_result);

	fclose(_inp_);
		
	//loop_SHELL();
}

int clrscreen()
{
	//system("clear");
	printf("\033[2J\033[1;1H") ;
	return 1;
}

int showEnvironmentVariables()
{
	//system("env");
        int i = 0 ;
        char *s = *environ;
        for (; s; i++) 
        {
            printf("%s\n", s);
            s = *(environ+i);
        }         
	return 1;
}

int showWorkingDirectory()
{
	char cwd[MAX_SIZE];

	if(getcwd(cwd, MAX_SIZE) != NULL)
		printf("%s\n",cwd);
		
	else
	{
		printf("Command Prompt error ! \n ");
		return -1;
	}

	return 1;
}

int updateHistory(char* s)
{
	FILE* _out_=fopen(history_file_path,"a+");

	if(_out_ == NULL)
	{
		printf("Cannot update history ! \n");
		return -1;
	}

	char sent[MAX_SIZE];
	int t=0;

	while(fgets(sent, MAX_SIZE, _out_)!=NULL)
	{
		char *temp=strtok(sent," \t\n");
		t=atoi(temp);
	}

	fprintf(_out_, "%d %s",t+1,s );
	fclose(_out_);

	return 1;
}

int changeDirectory(char* s)
{
	char cwd[MAX_SIZE];
	{
		if(s==NULL)
			chdir("/home");
		else
			{
				if(getcwd(cwd, MAX_SIZE) != NULL && strcmp(cwd,"/home")==0 && strcmp(s,"..")==0);
				else return chdir(s);
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

int listContents(char* s)
{	

    DIR* directory;
    struct dirent* dir_entry;
    struct stat dir_stat;
    unsigned long int total_blocks_size;

	
	if(s==NULL)
	{
		//system("ls");
		directory = opendir(".");
		if(directory == NULL)
		{
		    perror("");
		    return;
		}
		// hidden files start with '.' . We don't need to print them
		while((dir_entry = readdir(directory)) != NULL)
		{
		    if(dir_entry->d_name[0] != '.')
		        printf("%s\t",dir_entry->d_name);
		    
		}   
		printf("\n");

	}
	else if(strcmp(s,"-l")==0)
	{
		//system("ls -l");
		directory = opendir(".");
		if(directory == NULL)
		{
		    perror("");
		    return;
		}
		//calculate the total block size of all the files which are not hidden
		total_blocks_size = 0;
		while((dir_entry = readdir(directory)) != NULL)
		{
		    if(dir_entry->d_name[0] != '.')
		    {
		        if(lstat(dir_entry->d_name,&dir_stat))
		            perror("");
		        
		        else
		            total_blocks_size += dir_stat.st_blocks;
		    }
		}
		closedir(directory);

		//open current directory for printing files with their statistics
		directory = opendir(".");
		if(directory == NULL)
		{
		    perror("");
		    return;
		}
		//total block size have to be multiplied by the fraction (block_sizeof_disk/block_sizeof_file) .
		total_blocks_size = (total_blocks_size * disk_blk_size)/file_blk_size;
		printf("total %lu\n",total_blocks_size);
		//print the statistics for files which are not hidden
		while((dir_entry = readdir(directory)) != NULL)
		{
		    if(dir_entry->d_name[0] != '.')
		    {
		        //lstat used instead of stat as it gives symbolic links also in the statistics.
		        if(lstat(dir_entry->d_name,&dir_stat))
		            perror("");
		        
		        else 
		        {    
		            char permission[11] ;
		            //print the permissions of the file

		            permission[0] = (S_ISDIR(dir_stat.st_mode))  ? 'd' : '-' ;
		            permission[1] = (dir_stat.st_mode & S_IRUSR) ? 'r' : '-' ;
		            permission[2] = (dir_stat.st_mode & S_IWUSR) ? 'w' : '-' ;
		            permission[3] = (dir_stat.st_mode & S_IXUSR) ? 'x' : '-' ;
		            permission[4] = (dir_stat.st_mode & S_IRGRP) ? 'r' : '-' ; 
		            permission[5] = (dir_stat.st_mode & S_IWGRP) ? 'w' : '-' ;
		            permission[6] = (dir_stat.st_mode & S_IXGRP) ? 'x' : '-' ;
		            permission[7] = (dir_stat.st_mode & S_IROTH) ? 'r' : '-' ; 
		            permission[8] = (dir_stat.st_mode & S_IWOTH) ? 'w' : '-' ;
		            permission[9] = (dir_stat.st_mode & S_IXOTH) ? 'x' : '-' ;
		            
		            int per;
		            for(per = 0 ; per < 10 ; per++)
		                printf("%c",permission[per]) ;

		            //print number of links to this file, the user name (acc. to user id of the file),
		            //the group name (acc. to group id of the file).
		            printf(" %d %s\t%s\t",(int)dir_stat.st_nlink,getpwuid(dir_stat.st_uid)->pw_name,getgrgid(dir_stat.st_gid)->gr_name);
		            //print the size of the file.
		            printf("%5lu\t",dir_stat.st_size);   
		            time_t t = dir_stat.st_mtime;
		            struct tm lt;
		            localtime_r(&t, &lt);
		            char timbuf[80];
		            strftime(timbuf, sizeof(timbuf), "%b %d %H:%M", &lt);       
		            //print the last modified time of the file.
		            printf("%s\t%s\n",timbuf,dir_entry->d_name);
		        }
		    }
		}       		
	}
	else
		printf("Sorry ! This shell supports only the following \n ls\nls -l\n");
	closedir(directory);

	return 1;
}

int showHistory(char* s)
{

	if(s==NULL)
	{

		FILE* _inp_=fopen(history_file_path,"r");

		if(_inp_ == NULL)
		{
			printf("Cannot open file ! Terminating...\n");
			return -1;
		}

		char sent[MAX_SIZE];
				
		while(fgets(sent, MAX_SIZE, _inp_)!=NULL)
			printf("%s",sent);
		
		fclose(_inp_);
	}

	else
	{
		int hist_arg=atoi(s);

		if(hist_arg==0)
		{
			printf("Please enter an integer argument as : history <int_arg>\n");
			return -1;
		}
		else
			{

				FILE* _inp_=fopen(history_file_path,"r");

				if(_inp_ == NULL)
				{
					printf("Cannot open file ! Terminating...\n");
					return -1;
				}

				char sent[MAX_SIZE];
				int count=0,tot_count=0;
				
				while(fgets(sent, MAX_SIZE, _inp_)!=NULL)tot_count++;

				rewind(_inp_);
				while(fgets(sent, MAX_SIZE, _inp_)!=NULL)
				{
					count++;
					if(count>tot_count - hist_arg)
						printf("%s",sent);
				}

				fclose(_inp_);

				}
			}
	return 1;
}

void executeCommand(char** s)
{

    int pid = fork();
    int ret;

	if(pid == 0) 
		{
			ret=execvp(s[0],s);
			printf("Invalid command !\n");
			exit(0);
		}
	

	int count=0;
	while(s[count]!=NULL)
		count++;

	if(strcmp(s[count-1],"&")==0) ;
	else wait(NULL);

}

/*	FOR BINARY PIPE 
void executePipedCommand(char* command_pair)
{
	char* first_command,*second_command;

	first_command=strtok(command_pair,"|");
	second_command=strtok(NULL,"|");

	//printf("1st : %s\n",first_command );
	//printf("2nd : %s\n",second_command);

	if((first_command==NULL)||(second_command==NULL))
		printf("Impossible Error !\n");

	char **args_1 = (char**)malloc(MAX_SIZE*sizeof(char*));
	char **args_2 = (char**)malloc(MAX_SIZE*sizeof(char*));

	char** p_1 =args_1;
	char** p_2 =args_2;

	*p_1=strtok(first_command, " \t\n");

	while(*p_1!=NULL)
	{
		p_1++;
		*p_1=strtok(NULL, " \t\n");
	}

	*p_2=strtok(second_command, " \t\n");

	while(*p_2!=NULL)
	{
		p_2++;
		*p_2=strtok(NULL, " \t\n");
	}

	int fd[2];
	pipe(fd);
	

	int pid_1=fork();

	if(pid_1==0)
	{
		// first child -> writer of pipe
		close(fd[0]);//close the read end
		close(1);	// close stdout
		dup(fd[1]);
		close(fd[1]);
		run(args_1);
		exit(0);
	}

	int pid_2=fork();

	if(pid_2==0)
	{
		// second child -> reader of pipe
		close(fd[1]);//close the writer end
		close(0);	// close stdin
		dup(fd[0]);
		close(fd[0]);
		run(args_2);
		exit(0);
	}

	waitpid(pid_1,NULL,0);
	waitpid(pid_2,NULL,0);

}
*/


/*SHELL SCRIPT TO CHECK MULTI-PIPE COMMAND

gcc -o a trial1.c
gcc -o b trial2.c
gcc -o c trial3.c
./a 7 |./b|./c
./a 34 |./b|./c
./a <num> |./b|./c

***********************/


// SIMILAR TO THE ABOVE (NOW COMMENTED OUT) BINARY PIPED COMMAND PAIR, THIS CODE STANDS FOR N-ARY PIPED COMMAND GROUP, IN GENERAL
void executePipedCommand(char* command_group,int commCount)
{
	//printf("commCount = %d\n", commCount);

	char* command[commCount];
	//command=(char**)malloc((commCount+1)*sizeof(char*));	// make space for commCount new commands

	int y=0;

	command[y]=strtok(command_group,"|");

	while(command[y]!=NULL)
	{
		//printf("%d th command : %s\n", y, command[y]);
		y++;
		command[y]=strtok(NULL,"|");
	}

	char*** comm_args=(char***)malloc(commCount*sizeof(char**));	// array of (lists of arguments, one for each command)

	for(y=0;y<commCount;y++)
		comm_args[y] = (char**)malloc(MAX_SIZE*sizeof(char*));

	for(y=0;y<commCount;y++)
	{
		//printf("%d th command : %s\n", y, command[y]);

		char** p=comm_args[y];

		*p=strtok(command[y], " \t\n");

		while(*p!=NULL)
		{
			p++;
			*p=strtok(NULL, " \t\n");
		}

	}

	int** fd=(int**)malloc(commCount*sizeof(int*));

	for(y=0;y<commCount-1;y++)			// pipeCount = commCount-1, where pipeCount is the no. of inter-process pipes
		{
			fd[y]=(int*)malloc(2*sizeof(int));
			pipe(fd[y]);
		}

	//fd[y] is the yth pipe
	// Representation : Process_0 <pipe_0> Process_1 <pipe_1> ........ <pipe_N> Process_N+1
	//					.........   |      .........    |      ........    |      .......
	// N = pipeCount
	// N+1 = pipeCount+1 = command_Count



	int* pid=(int*)malloc(commCount*sizeof(int));

	for(y=0;y<commCount;y++)
	{
		pid[y]=fork();
	

	if(pid[y]==0)
	{
		// for yth child process in general
		if(y==0)
		{
			// 0th process

			close(fd[y][0]);//close the read end
			close(1);	// close stdout file
			dup(fd[y][1]);
			close(fd[y][1]);
		
		}
		else if(y!=commCount-1)
		{

			// process[y] is the writer for pipe[y]

			close(fd[y][0]);//close the read end
			close(1);	// close stdout file
			dup(fd[y][1]);
			close(fd[y][1]);



			// process[y] is the reader for pipe[y-1]

			close(fd[y-1][1]);//close the writer end
			close(0);	// close stdin file
			dup(fd[y-1][0]);
			close(fd[y-1][0]);

		}
		else if (y == commCount-1)
		{
			close(fd[y-1][1]);//close the writer end
			close(0);	// close stdin file
			dup(fd[y-1][0]);
			close(fd[y-1][0]);
		}
		else
			printf("Impossible value of y : %d !!!\n",y);

		run(comm_args[y]);
		exit(0);

	}

}


	for(y=0;y<commCount;y++)
		wait(NULL);

}

/*SHELL SCRIPT TO CHECK MULTI-PIPE COMMAND

gcc -o a trial1.c
gcc -o b trial2.c
gcc -o c trial3.c
./a 7 |./b|./c
./a 34 |./b|./c
./a <num> |./b|./c
./a <num> |./b|./b | ./b | ./b ....

***********************/


void executeRedirectedCommand(char* red_command,char direct)	// direct represents the direction character : < (in) or > (out)
{
	char* command= (direct=='<' ? strtok(red_command,"<") : strtok(red_command,">"));
	char* file_name_temp= (direct=='<' ? strtok(NULL,"<") : strtok(NULL,">"));

	char* file_name=strtok(file_name_temp," \t\n");

	int pid=fork();

	if(pid==0){

	if(direct=='>')
	{
		int fd=open(file_name, O_RDWR | O_TRUNC | O_CREAT , S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH);
	
		if(fd==-1)
		{
			printf("Redirection failed !!!\n");
			return;
		}
		
		close(1);	// close stdout file
		dup(fd);
		close(fd);
	}
	else
	{
		int fd=open(file_name,O_RDONLY, S_IRUSR | S_IRGRP | S_IROTH);
	
		if(fd==-1)
		{
			printf("Redirection failed !!!\n");
			return;
		}
		
		close(0);	// close stdin file
		dup(fd);
		close(fd);
	}

	char **args = (char**)malloc(MAX_SIZE*sizeof(char*));

		char** p =args;

		*p=strtok(command, " \t\n");

		while(*p!=NULL)
		{
			p++;
			*p=strtok(NULL, " \t\n");
		}

		run(args);
		exit(0);
	}

	wait(NULL);

}

/*SHELL SCRIPT TO CHECK BIDIRECTIONAL REDIRECTION COMMAND

./a <num> > input.txt
./b < input.txt > output.txt
./b > output.txt < input.txt 

***********************/


void executeBidirectedCommand(char* red_command,int mode)	// direct represents the direction character : < (in) or > (out)
{

	char* command = strtok(red_command,"<>");

	char* file_name_temp_1 = strtok(NULL,"<>");
	char* file_name_temp_2 = strtok(NULL,"<>");

	char* file_name_1=strtok(file_name_temp_1," \t\n");
	char* file_name_2=strtok(file_name_temp_2," \t\n");


	int pid=fork();

	if(pid==0){

	if(mode==1)	// i.e. < occurs before >
	{
		int fd=open(file_name_2,O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH);
	
		if(fd==-1)
		{
			printf("Write Redirection failed !!!\n");
			return;
		}
		
		close(1);	// close stdout file
		dup(fd);
		close(fd);


		fd=open(file_name_1,O_RDONLY, S_IRUSR | S_IRGRP | S_IROTH);
	
		if(fd==-1)
		{
			printf("Read Redirection failed !!!\n");
			return;
		}
		
		close(0);	// close stdin file
		dup(fd);
		close(fd);
	}
	else if(mode==2)	// i.e. > occurs before <
	{
		int fd=open(file_name_2,O_RDONLY, S_IRUSR | S_IRGRP | S_IROTH);
	
		if(fd==-1)
		{
			printf("Read Redirection failed !!!\n");
			return;
		}
		
		close(0);	// close stdin file
		dup(fd);
		close(fd);


		fd=open(file_name_1,O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH);
	
		if(fd==-1)
		{
			printf("Write Redirection failed !!!\n");
			return;
		}
		
		close(1);	// close stdout file
		dup(fd);
		close(fd);
	}
	else
		printf("Impossible ERROR !\n");

	char **args = (char**)malloc(MAX_SIZE*sizeof(char*));

		char** p =args;

		*p=strtok(command, " \t\n");

		while(*p!=NULL)
		{
			p++;
			*p=strtok(NULL, " \t\n");
		}

		run(args);
		exit(0);
	}

	wait(NULL);

}

/*SHELL SCRIPT TO CHECK BIDIRECTIONAL REDIRECTION COMMAND

./a <num> > input.txt
./b < input.txt > output.txt
./b > output.txt < input.txt 

***********************/

void run(char** args)
{
	if(args==NULL)
			return;

		if(strcmp(args[0],"clear")==0 && args[1]==NULL)
			clrscreen();

		else if(strcmp(args[0],"ls")==0)
			listContents(args[1]);

		else if(strcmp(args[0],"env")==0 && args[1]==NULL)
			showEnvironmentVariables();

		else if(strcmp(args[0],"exit")==0 && args[1]==NULL)
			exit(0);

		else if(strcmp(args[0],"pwd")==0 && args[1]==NULL)
		{
			if (showWorkingDirectory() == -1)
				printf("Error showing current directory !\n");
		}

		else if(strcmp(args[0],"cd")==0)
		{
			if(changeDirectory(args[1])==-1)
				printf("ERROR : No such directory exists!\n");
		}
		
		else if(strcmp(args[0],"mkdir")==0)
		{
			if(args[1]==NULL)
				printf("Invalid Command ! Please enter : \n mkdir <dir_name>\n");
			else if(makeDirectory(args[1])==-1)
				printf("ERROR in creating directory !\n");
		}

		else if(strcmp(args[0],"rmdir")==0)
		{
			if(args[1]==NULL)
				printf("Invalid Command ! Please enter : \n rmdir <dir_name>\n");
			else if(removeDirectory(args[1])==-1)
				printf("ERROR in deleting directory !\n");
		}

		else if(strcmp(args[0],"history")==0)
		{
			if(showHistory(args[1])==-1)
				printf("ERROR ! Trouble showing history !\n");
		}
		
		else executeCommand(args);
}

void loop_SHELL()
{
	FILE* _io_=fopen("commands.txt","a+");

	signal(SIGQUIT,searchHandler);	// register the installed Signal Handler
	
	realpath("commands.txt", history_file_path);
	
   	while(1)
	{
		char cwd[MAX_SIZE];
		char buf[MAX_SIZE];

		FILE* _out_;

		if(getcwd(cwd, MAX_SIZE) != NULL)
			printf(" %s > ",cwd);	
		else
		{
			printf("Command Prompt error ! Terminating...\n ");
			exit(1);
		}

		if(strcmp(next_command,"")!=0)
		{
			strcpy(buf,next_command);
			strcpy(next_command,"");
		}
		else
		{
			if(fgets(buf, MAX_SIZE, stdin)==NULL)
				continue;
		}
	

		if(strlen(buf)<=1)
			continue;

		if(updateHistory(buf)==-1)
			printf("Trouble updating History !\n");

		int x=0,pipeCounter=0;

		for(x=0;buf[x]!='\0';x++)
			if(buf[x]=='|')
				pipeCounter++;

		if(pipeCounter>0)
		{
			executePipedCommand(buf,pipeCounter+1);
			continue;
		}

		x=0;
		char redirect_char;
		int redirect_Counter=0;
		for(x=0;buf[x]!='\0';x++)
			if(buf[x]=='<' || buf[x]=='>')
			{
				redirect_char=buf[x];
				redirect_Counter++;
			}

		if(redirect_Counter==1)
		{
			executeRedirectedCommand(buf,redirect_char);
			continue;
		}
		else if (redirect_Counter==2)
		{
			int mode;
		
			for(x=0;buf[x]!='\0';x++)
			{
				if(buf[x]=='<') 
				{
					mode=1;
					break;
				}
				else if(buf[x]=='>')
				{
					mode=2;
					break;
				}
			}

			executeBidirectedCommand(buf,mode);	// mode = 1 if < occurs 'b4' > and 2 if otherwise
			continue;
		}

		char **args = (char**)malloc(MAX_SIZE*sizeof(char*));

		char** p =args;

		*p=strtok(buf, " \t\n");

		while(*p!=NULL)
		{
			p++;
			*p=strtok(NULL, " \t\n");
		}

		run(args);		
	}
}


void main()
{
	loop_SHELL();
}
