#include <stdio.h>

void main()
{
	char inp[1024];

	fgets(inp,1024,stdin);

	printf("This is trial 3 . The 1st sentence I received from stdin is as follows :\n");
	fputs(inp,stdout);

	fgets(inp,1024,stdin);

	printf("This is trial 3 . The 2nd sentence I received from stdin is as follows :\n");
	fputs(inp,stdout);

}