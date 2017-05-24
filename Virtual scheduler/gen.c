#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <grp.h>



int main()
{	
	system("xterm -e ./process 10000 10 0.3 1 &");
	sleep(1);
	system("xterm -e ./process 10000 10 0.3 1 &");
	sleep(1);
	system("xterm -e ./process 4000 5 0.7 3 &");
	sleep(1);
	system("xterm -e ./process 4000 5 0.7 3 &");
	
	return 0;	
}
