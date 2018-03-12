#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include <sys/select.h>

#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>

#include <map>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>

#include "site.h"

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 5
#define PORT 6666

std::map<int,std::string> config_data;
std::map<int,int> userid_socket;
std::map<int,int> online_check;
int count_N = 0;
int maxN = 0;
int oldMax = 0;
int selfid;
Site site;
struct sockaddr_in server;
socklen_t len;
int sock;
void ClientThread(int userid);
void ReadInput()
{

    while(1) 
    {
        char type[1024];
        char buf[1024];
        char result[1024];
        scanf("%s",type);
        fgets(buf,sizeof(buf),stdin);
        buf[strlen(buf)-1] = '\0';
        strcpy(result,buf+1);
        //different operation

        std::string message(result);

        if(std::string(type) == "tweet")
        {
            site.tweet(message);
        }
        else if(std::string(type) == "block")
        {
        	int id = std::stoi(message);
            site.block(id);
        }
        else if(std::string(type) == "unblock")
        {
        	int id = std::stoi(message);
            site.unblock(id);
        }
        else if(std::string(type) == "view"){
            site.view();
        }
        else if(std::string(type) == "viewdict"){
            site.view_dict();
        }
        else {
            printf("Invalid Command...Try again!\n");
        }

    }
}


void RecvMsg(char* m_msg)
{	
	
	char* msg = m_msg;
	int type = *(int*)msg;
	int N, userid;
	long n;
	std::string value;
	msg += sizeof(int);
	N = *(int*)msg;
	msg += sizeof(int);
	if(type == 0)
	{
		n = *(long*)msg;
		msg += sizeof(long);
		userid = *(int*)msg;
		msg += sizeof(int);
		site.recv_prepare(N, n, userid);
	}
	else if(type == 4)
	{
		value = std::string(msg);
		msg = msg + value.size() + 1;
		userid = *(int*)msg;
		msg += sizeof(int);
		site.recv_commit(N, value, userid);
	}
	else if(type == 5)
	{
		int recv_N = N;
		count_N++;
		if(recv_N > maxN)
		{
			maxN = recv_N;
		}

		int count = 0;
		for(std::map<int,int>::iterator it = online_check.begin(); it != online_check.end(); it++)
		{
			if(it->second == 1)
			{
				count++;
			}
		}
		if(count_N == count)
		{	
			while(oldMax != site.getN())
			{
			}
			site.recovery(maxN);
			oldMax = maxN;
		}
	}
	else
	{
		n = *(long*)msg;
		msg += sizeof(long);
		value = std::string(msg);
		msg = msg + value.size() + 1;
		userid = *(int*)msg;
		msg += sizeof(int);

		if(type == 1)
		{
			site.accept(N, n, value, userid);
		}
		else if(type == 2)
		{
			site.recv_accept(N, n, value, userid);
		}
		else
		{
			site.commit(N, n, value, userid);
		}
	}
	free(m_msg);
}


void ServerReceive()
{
	fd_set readfds;
	struct sockaddr_in client;
	socklen_t fromlen = sizeof(client);
	while(1)
	{
		FD_ZERO(&readfds);
		FD_SET(sock, &readfds);

		for(std::map<int,int>::iterator it = userid_socket.begin(); it != userid_socket.end(); it++)
		{
			FD_SET(it->second, &readfds);
		}

		int ready = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
		if ( ready == 0 )
		{
	      printf( "No activity\n" );
	      continue;
	    }

		if(FD_ISSET(sock, &readfds))
		{
			int newsock = accept(sock, (struct sockaddr *)&client, (socklen_t *)&fromlen);
			//inet_ntoa(*((struct in_addr*)&client.sin_addr.s_addr))
			for(std::map<int,std::string>::iterator it_find = config_data.begin(); it_find != config_data.end(); it_find++)
			{
				if(it_find->second == std::string(inet_ntoa((struct in_addr)client.sin_addr)))
				{
					userid_socket[it_find->first] = newsock;
					site.setSocketMap(userid_socket);
					printf("userid %d is connected\n",it_find->first);
					if(online_check[it_find->first] == 0)
					{
						online_check[it_find->first] = 1;
						std::thread t_client(ClientThread,it_find->first);
						t_client.detach();
					}
				}
			}
		}


		for(std::map<int,int>::iterator it = userid_socket.begin(); it != userid_socket.end(); it++)
		{
			int fd = it->second;
			int n;
			if(FD_ISSET(fd, &readfds))
			{
				char buffer[1024];
				n = recv(fd, buffer, sizeof(buffer), 0);

				if(n < 0)
				{
					perror("recv() failed");
				}
				else if( n == 0)
				{
					close(fd);
					for(std::map<int,int>::iterator it_erase = userid_socket.begin(); it_erase != userid_socket.end(); it_erase++)
					{
						if(it_erase->second == fd)
						{
							printf("userid %d is disconnected\n",it_erase->first);
							online_check[it_erase->first] = 0;
							userid_socket.erase(it_erase);
							site.setSocketMap(userid_socket);
							break;
						}
					}
					break;
				}
				else
				{
					buffer[n] = '\0';
					//printf("Received message from %s: %s\n",inet_ntoa((struct in_addr)client.sin_addr), buffer);
					//RecvMsg(buffer);
					
					char* m_recvbuf = (char*)malloc(1024*sizeof(char));
	      			memcpy(m_recvbuf,buffer,1024);
					std::thread recv_thread(RecvMsg,m_recvbuf);
					recv_thread.detach();
				}
				memset(buffer,0, sizeof(buffer));
			}
		}
	}
}

void ClientThread(int userid)
{
	int sd = socket( AF_INET, SOCK_STREAM, 0 );

	if ( sd < 0 )
	{
	perror( "socket() failed" );
	exit( EXIT_FAILURE );
	}

	std::string ip = config_data[userid];

	struct hostent * hp = gethostbyname(ip.c_str());


	if ( hp == NULL )
	{
	fprintf( stderr, "ERROR: gethostbyname() failed\n" );
	return;
	}

	struct sockaddr_in server;
	server.sin_family = AF_INET;
	memcpy( (void *)&server.sin_addr, (void *)hp->h_addr, hp->h_length );
	server.sin_port = htons( PORT );

	//printf( "server address is %s\n", inet_ntoa( server.sin_addr ) );


	printf( "connecting to server.....\n" );
	if ( connect( sd, (struct sockaddr *)&server, sizeof( server ) ) < 0 )
	{
		printf("server is off\n");
		return;
	}
	online_check[userid] = 1;
	printf("server connected!");

	char sendline[1024];
	int type = 5;
	memcpy(sendline,&type,sizeof(int));
	int my_N = site.getN();
	memcpy(sendline+sizeof(int),&my_N,sizeof(int));
	send(sd,sendline,sizeof(sendline),0);

	fd_set rfds;
	while(1)
	{
	FD_ZERO(&rfds);
	FD_SET(sd, &rfds);

	int ready = select(FD_SETSIZE, &rfds, NULL, NULL, NULL);

	if(ready == -1)
	{
	  perror("select() failed\n");
	  break;
	}
	else if(ready == 0)
	{
	  continue;
	}
	else
	{
	  if(FD_ISSET(sd, &rfds))
	  {
	    char recvbuf[1024];
	    int n = recv(sd, recvbuf, sizeof(recvbuf), 0);
	    if(n == 0)
	    {
	      break;
	    }
	    else if(n == 1)
	    {
	      perror("recv() failed\n");
	    }
	    else
	    {
	      recvbuf[n] = '\0';
	      //printf("Received message %s\n",recvbuf);
	      //RecvMsg(recvbuf);
	      
	      char* m_recvbuf = (char*)malloc(1024*sizeof(char));
	      memcpy(m_recvbuf,recvbuf,1024);
	      std::thread recv_thread(RecvMsg,m_recvbuf);
	      recv_thread.detach();
	      
	    }
	    memset(recvbuf, 0, sizeof(recvbuf));
	  }
	}

	}
}

int main(int argc, char* argv[])
{
	selfid = atoi(argv[1]);
	int number = atoi(argv[2]);
	site.setMajority(number);
	std::ifstream config("config.txt");
	site.setID(selfid);
	oldMax = site.getN();

	for(int i = 0; i < number; i++)
	{
		online_check.insert(std::make_pair(i,0));
	}

	int userid;
	std::string ip;
	while(config >> userid >> ip)
	{
		config_data[userid] = ip;
	}

	for(std::map<int,std::string>::iterator it = config_data.begin(); it != config_data.end(); it++)
	{
		std::cout << it->first << ":" << it->second << std::endl;
	}


	sock = socket(AF_INET, SOCK_STREAM, 0);

	if(sock < 0)
	{
		perror("socket() failed");
		exit(EXIT_FAILURE);
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(PORT);

	len = sizeof(server);

	int on = 1;
	if((setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0)
	{
		perror("setsockopt() failed");
		exit(EXIT_FAILURE);
	}

	if(bind(sock, (struct sockaddr *)&server, len) < 0)
	{
		perror("bind() failed");
		exit(EXIT_FAILURE);
	}

	listen(sock, MAX_CLIENTS);

	std::thread t1(ReadInput);
	t1.detach();
	std::thread t2(ServerReceive);
	t2.detach();

	for(std::map<int,std::string>::iterator it = config_data.begin(); it != config_data.end(); it++)
	{
		std::thread t_client(ClientThread,it->first);
		t_client.detach();
	}



	while(1)
	{
	}


	return EXIT_SUCCESS;
}
