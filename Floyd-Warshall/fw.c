#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

// 1 <= N <= 100

/* Global NxN Matrix Graph representing the adjacency matrix of the input graph */
int Graph[100][100]; 

/* Global NxN Matrix dist maintaining the ALL-pair shortest path distances between any 2 nodes */
int dist[100][100];

// Graph[0][0] and dist[0][0] are to be ignored

void initMatrices()
{
	int n,m;

	for(n=0;n<101;n++)
		for(m=0;m<101;m++)
		{
			Graph[n][m]=-1;

			if(n==m) dist[n][m]=0;
			else dist[n][m] = -1; // represents Infinity
		}
}


/*  Global Mutex Locks for Readers Writers Problem */
pthread_mutex_t mutex_lock; // for accessing the variable readCount
pthread_cond_t condition_var; // writer part can access on ONLY 1 condition : readCount = 0
int readCount=0;

int arg_k,arg_N;

void* thread_function(void* arg)
{
	int j;
	long i = (long) arg;
	int k = arg_k;
	int N = arg_N;

	printf("Thread Started : i = %ld, k = %d \n",i,k);

	for(j=1;j<=N;j++)
	{
		// acquire the mutex lock for accessing the readCount variable
		pthread_mutex_lock(&mutex_lock);
		printf("Mutex lock acquired by Thread  i = %ld, k = %d j = %d      ",i,k,j);
		readCount++;
		// If this is the first reader, then the writer gets automatically blocked, when it waits for the condition variable @ condition ( readCount == 0 )			
		pthread_mutex_unlock(&mutex_lock);
		printf("Mutex lock released by Thread  i = %ld, k = %d j = %d \n",i,k,j);

		// Reading :
			int boolean;
			if(dist[i][k] == -1 || dist[k][j] == -1) boolean = 0;
			else if(dist[i][j]==-1) boolean = 1;
			else boolean = (dist[i][k] + dist[k][j] < dist[i][j]);

		/*  End reading part **/
				pthread_mutex_lock(&mutex_lock);
				printf("Mutex lock acquired by Thread  i = %ld, k = %d j = %d      ",i,k,j);
				readCount--;
				if(readCount==0)
					pthread_cond_signal(&condition_var); // to wake up any blocked writer parts
				pthread_mutex_unlock(&mutex_lock);
				printf("Mutex lock released by Thread  i = %ld, k = %d j = %d \n",i,k,j);
		/**	END of reading part**/

		if (boolean)
		{
			// Writing part begins :
			pthread_mutex_lock(&mutex_lock);
			printf("Mutex lock acquired for writing by Thread  i = %ld, k = %d j = %d      ",i,k,j);

			while(readCount!=0)
			{
				printf(" Thread i = %ld, k = %d j = %d  -> waiting for writing !\n",i,k,j);
				pthread_cond_wait(&condition_var,&mutex_lock);
			}
			printf("Thread  i = %ld, k = %d j = %d writing ...\n",i,k,j);
			dist[i][j] = dist[i][k] + dist[k][j];

			pthread_mutex_unlock(&mutex_lock);
			printf("Mutex lock released after writing by Thread  i = %ld, k = %d j = %d \n",i,k,j);
			// Writing part ENDS...
		}

	}


	pthread_exit(NULL);
}

void error(const char* str)
{
	printf("%s\n",str);
	exit(0);
}

void main()
{
	initMatrices();

	int N,M,i,j,k;

	printf("Enter Graph : \n\n");
	printf("Enter N,M (1<=N<=100) : ");
	scanf("%d %d",&N,&M);

	if(N<=0 || M<=0)
		error("Please enter positive parameters !\n");
	else if(N>100)
		error("N should not be more than 100 !\n");

	printf("Enter %d edges as (u,v,w) pairs (1<= u,v <=%d) : \n",M,N);

	for(i=0;i<M;i++)
	{
		int u,v,w;

		scanf("%d %d %d",&u,&v,&w);

		if(u<1 || u>N)
			error("Ui NOT within valid range : [1,N] !\n");
		else if(v<1 || v>N)
			error("Vi NOT within valid range : [1,N] !\n");
		else if(w<=0)
			error("Edge weight must be positive !\n");
		else
		{
			int p;
			char errStr[100];

			for(p=0;Graph[u][p]!=-1;p++)
				if(Graph[u][p]==v){
					sprintf(errStr,"Repeated edge pair (%d,%d) entered !\n",u,v);
					error(errStr);
				}
			Graph[u][p]=v;

			for(p=0;Graph[v][p]!=-1;p++)
				if(Graph[v][p]==u){
					sprintf(errStr,"Repeated edge pair (%d,%d) entered !\n",v,u);
					error(errStr);
				}
			Graph[v][p]=u;

			dist[u][v] = dist[v][u] = w;
		}
		
	}


	printf("Following is the adjacency matrix : \n");
	for(i=1;i<=N;i++)
	{
		printf("\nU = %d : ",i);
		for(j=0;Graph[i][j]!=-1;j++)
			printf("%d, ",Graph[i][j]);
	}

	printf("\n\nFollowing is the INITIAL dist matrix (-1 --> Infinity) : \n");
	for(i=1;i<=N;i++)
	{
		for(j=1;j<=N;j++)
			printf("%3d ",dist[i][j]);
		printf("\n");
	}

	pthread_t i_threads[N];
	pthread_attr_t attr;

	pthread_mutex_init(&mutex_lock,NULL);
	pthread_cond_init(&condition_var,NULL);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	for(k=1;k<=N;k++)
	{
		arg_N=N;
		arg_k=k;

		long i;
		for(i=0;i<N;i++)
			pthread_create(&i_threads[i],&attr,thread_function,(void*)i+1);
		
		// Wait for all the N threads to complete before starting the next k-iteration
		for(i=0;i<N;i++)
			pthread_join(i_threads[i],NULL);
				
	}

	pthread_attr_destroy(&attr);
	pthread_mutex_destroy(&mutex_lock);
	pthread_cond_destroy(&condition_var);

	printf("\n\nFollowing is the FINAL dist matrix (-1 --> Infinity) : \n");
	for(i=1;i<=N;i++)
	{
		for(j=1;j<=N;j++)
			printf("%3d ",dist[i][j]);
		printf("\n");
	}

	pthread_exit(NULL);

}
