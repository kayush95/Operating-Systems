#include <stdio.h>
#include <string.h>

void main()
{
	char inp[1024],*word;
	int count=0;

	fgets(inp,1024,stdin);

	word=strtok(inp," \t");

	while(word!=NULL)
	{
		count++;
		word=strtok(NULL," \t");
	}

	printf("This is trial 2. There are %d words in the First sentence taken from stdin\n",count );

	fgets(inp,1024,stdin);

	int N;
	scanf("%d",&N);

	printf("The square of the number received from stdin is \n%d\n",N*N);
}