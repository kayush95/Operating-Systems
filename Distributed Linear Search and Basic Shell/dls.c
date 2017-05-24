/***************************************************************
Operating Systems Lab Assignment 1 : Problem 1

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

int* arr;	// global array of integers (received as input from the file)

int dls(int*,int,int,int);

int main(int argc,char* argv[])
{
	if(argc!=3)
	{
		printf("Sorry ! Invalid argument count ! Please provide the input file name and the search integer as arguments...\n");
		exit(1);
	}

	FILE* _inp_=fopen(argv[1],"r");

	if(_inp_ == NULL)
	{
		printf("Cannot open file ! Terminating...\n");
		exit(1);
	}

	int i,num,count=0;

	while(fscanf(_inp_,"%d",&num) !=  EOF)
		count++;

	arr=(int*)malloc(count*sizeof(int));
	rewind(_inp_);

	for(i=0;i<count;i++)
		fscanf(_inp_,"%d",&arr[i]);

	int search = dls(arr,atoi(argv[2]),0,count-1);

	if(search==-1)
		printf("not found !\n");
	else if(arr[search]==atoi(argv[2]))
		printf("found @ index : %d\n", search);
	
	fclose(_inp_);
		
}

int dls(int* A,int N,int beg,int end)	// returns -1 if NOT FOUND
{
	if(end-beg+1 <=5)
	{
		int k;
		for(k=beg;k<=end;k++)
			if(A[k]==N)
			{
				//printf("I found @ position : %d\n", k);
				return k;
			}
		
	}
	else
	{
		int mid=(beg+end)/2;

		int fork_val1=fork();
		
		if(fork_val1<0)
		{
			printf("Sorry ! Out of memory resources ! Cannot continue the search. Terminating ....\n");
			exit(1);
		}

		if(fork_val1==0)
			exit(dls(A,N,beg,mid));



		int fork_val2=fork();

		if(fork_val2<0)
		{
			printf("Sorry ! Out of memory resources ! Cannot continue the search. Terminating ....\n");
			exit(1);
		}

		if(fork_val2==0)
			exit(dls(A,N,mid+1,end));
		
		
		

		int status;

		int reply_cid=wait(&status);
		int indx=WEXITSTATUS(status);

		if(indx==255 || indx<beg || indx>end)
			{}
		else if(A[indx]==N)
		{
			//printf("Found @ index %d by my Child_ID : %d\n", indx,reply_cid);
			kill(fork_val2,SIGKILL); 	// kill the second child
			return indx;
		}

		reply_cid=wait(&status);
		indx=WEXITSTATUS(status);

		if(indx==255 || indx<beg || indx>end)
			return -1;

		if(A[indx]==N)
		{
			//printf("Found @ index %d by my Child_ID : %d\n", indx,reply_cid);
			return indx;
		}

		return -1;	// if NOT FOUND 
	}
}

