#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>
#include <bits/stdc++.h>

using namespace std;

#define MAX_SIZE 5024
#define MSSG_Q_KEY 200
#define COUPLE 10
#define UNCOUPLE 20
#define BROADCAST 30

struct mssg
{
	long mtype;
	char mtext[MAX_SIZE];
};

class Client
{
private:
	static long ID_counter;

	pid_t PID;
	long ID;

public:
	Client(pid_t PID)
	{
		this->PID = PID;
		ID=++ID_counter;
	}

	long getID()
	{
		return ID;
	}

	pid_t getPID()
	{
		return PID;
	}

	void printDetails()
	{
		cout<<"------------------------"<<endl;
		cout<<"Client   ID : " << ID << endl;
		cout<<"Process  ID : " << PID << endl;
		cout<<"------------------------"<<endl;
	}
};

long Client::ID_counter=0;

void showAllCoupledClients(list<Client*>);
void removePID(list<Client*>&,pid_t);
long getIDfromPID(list<Client*>,pid_t);

int main()
{

	list<Client*> client_list= list<Client*>();

	int mssgQ_ID=msgget((key_t)MSSG_Q_KEY,IPC_CREAT|0666);

	while(1)
	{
		struct mssg buffer;
		struct msqid_ds qstat;

		cout<<"Server waiting for message..." << endl;

		msgrcv(mssgQ_ID,&buffer,sizeof(buffer),0,0);

		if(buffer.mtype==COUPLE)
		{
			msgctl(mssgQ_ID,IPC_STAT,&qstat);
			//couple the requesting terminal
			Client* newClient=new Client(qstat.msg_lspid);
			client_list.push_back(newClient);

			cout << "\nThe terminal process with PID " << qstat.msg_lspid << " has been assigned ID = " << newClient->getID() << endl;		

			// send confirmation message to the newly coupled client terminal
			sprintf(buffer.mtext,"Coupled successfully to the terminal mirroring server... \nID Assigned : %ld\n\n",newClient->getID());
			buffer.mtype=newClient->getPID();
			msgsnd(mssgQ_ID,&buffer,sizeof(buffer),0);

			showAllCoupledClients(client_list);
		}
		else if(buffer.mtype==UNCOUPLE)
		{
			//uncouple the requesting terminal
			msgctl(mssgQ_ID,IPC_STAT,&qstat);

			removePID(client_list, qstat.msg_lspid);

			cout << "\nThe terminal process with PID " << qstat.msg_lspid << " has been uncoupled from the Terminal Mirroring Server ... \n"<< endl;
			
			// send confirmation message to the now decoupled client terminal
			sprintf(buffer.mtext,"Uncoupled successfully from the terminal mirroring server... \nID Assigned : \n\n");
			buffer.mtype= qstat.msg_lspid;
			msgsnd(mssgQ_ID,&buffer,sizeof(buffer),0);

			showAllCoupledClients(client_list);
		}
		else if(buffer.mtype==BROADCAST)
		{
			//mirror the terminal's broadcast to other terminals
			msgctl(mssgQ_ID,IPC_STAT,&qstat);

			long senderID=getIDfromPID(client_list, qstat.msg_lspid);

			char sent[MAX_SIZE];
			sprintf(sent,"MIRROR from Terminal ID : %ld\n\n",senderID);
			strcat(sent, buffer.mtext);
			strcpy(buffer.mtext,sent);

			for(list<Client*>::iterator it=client_list.begin() ; it!=client_list.end() ; it++ )
			{
				if((*it)->getID()!=senderID) 
					{
						buffer.mtype=(*it)->getPID();
						msgsnd(mssgQ_ID,&buffer,sizeof(buffer),0);
					}
			}
			
		}
	}

	return 0;
}

void showAllCoupledClients(list<Client*> clist)
{
	cout<<"\n\nUpdated list of all coupled clients : \n";
	for(list<Client*>::iterator it=clist.begin() ; it!=clist.end() ; it++ )
	{
		(*it)->printDetails();
		cout<<endl;
	}
}

void removePID(list<Client*> &clist,pid_t delPID)
{
	for(list<Client*>::iterator it=clist.begin() ; it!=clist.end() ; it++ )
	{
		cout<< " it -> " << (*it)->getPID() << " delPID : " << delPID << endl;
		if((*it)->getPID() ==delPID)
			it=clist.erase(it);
	}
}

long getIDfromPID(list<Client*> clist, pid_t PID)
{
	for(list<Client*>::iterator it=clist.begin() ; it!=clist.end() ; it++ )
	{
		if((*it)->getPID() ==PID)
			return (*it)->getID();
	}
}