#include <stdio.h>
#include <stdlib.h>

void main(int argc,char** argv)
{
	if(argc!=2)
		{
			printf("Sorry ! Please provide one int argument to the program\n");
			exit(1);
		}

	printf("The program trial 1 prints this sentence onto stdout\n");

	printf("This program also outputs the no. %d to stdout\n", atoi(argv[1]));

	printf("%d\n", atoi(argv[1]));
}