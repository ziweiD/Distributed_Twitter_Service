#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <iostream>
#include <thread>
#include <list>
#include <map>
#include <fstream>
#include "project.h"

#define PORT 7000
#define BUFFER_SIZE 1024

//socket info
int s;
struct sockaddr_in servaddr;
socklen_t len;
int selfid;
std::string selfname;
//some map to build connection between socketfd and number and name of each client
Client c;
std::map<int,std::string> config_data;
std::map<int,std::string> number_name;
std::map<std::string,int> name_number;
std::map<int,int> order_conn;
std::map<int,int> Number_conn;




//thread waiting for any clients want to connect
void ClientThread(int server);
void getConn() {
    while(1) {
        int conn = accept(s, (struct sockaddr*)&servaddr, &len);
        int number1 = order_conn.size();
        order_conn.insert(std::make_pair(number1,conn));
    }
}

void getData() {
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    while(1) {
        //std::list<int>::iterator it;
        std::map<int,int>::iterator it;
        for(it = order_conn.begin(); it != order_conn.end(); ++it) {            
            fd_set rfds;    
            FD_ZERO(&rfds);
            int maxfd = 0;
            int retval = 0;
            FD_SET(it->second, &rfds);
            if(maxfd < it->second) {
                maxfd = it->second;
            }
            retval = select(maxfd+1, &rfds, NULL, NULL, &tv);
            if(retval == -1) {
                printf("select error\n");
            }
            else if(retval == 0) {
            }
            else {
                char buf[1024];
                memset(buf, 0 ,sizeof(buf));
                int len = recv(it->second, buf, sizeof(buf), 0);
                if(len == 0) {
                }
                else if(len == -1) {
                }
                else {
                    //once a client is connected, server needs to identify whether it's an "old" client
                    int dummy;
                    memcpy(&dummy,buf,sizeof(int));
                    if(dummy == -1) {   
                        int number;
                        memcpy(&number,buf+sizeof(int),sizeof(int));
                        std::map<int,int>::iterator it_find = Number_conn.find(number);
                        if(it_find != Number_conn.end()) {
                            it_find -> second = it->second;
                            printf("%s reconnected\n",number_name[number].c_str());
                            std::thread t(ClientThread,number);
                            t.detach();
                        }
                        else {
                            Number_conn.insert(std::make_pair(number,it->second));
                            printf("%s connected\n",number_name[number].c_str());
                        }
                    }
                }
            }
        }

    }
}

void ReadInput()
{

    while(1) {
        char type[1024];
        char buf[1024];
        scanf("%s",type);
        fgets(buf,sizeof(buf),stdin);
        buf[strlen(buf)-1] = '\0';
        std::string message(buf+1);
        //different operation
        if(std::string(type) == "tweet") {   
            c.tweet(message);
            for(int i = 0; i < Number_conn.size(); i++) {
                if(i==selfid) {
                    continue;
                }
                c.sendMsg(i,Number_conn[i]);
            }
        }
        else if(std::string(type) == "block"){
            c.block(message);
        }
        else if(std::string(type) == "unblock"){
            c.unblock(message);
        }
        else if(std::string(type) == "view"){
            c.view();
        }
        else {   
            printf("Invalid Command...Try again!\n");
        }


    }
}

void ClientThread(int server)
{   
    int sock_cli;
    fd_set rfds;
    struct timeval tv;
    int retval,maxfd;

    sock_cli = socket(AF_INET,SOCK_STREAM, 0);
    ///define sockaddr_in
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);  ///our port is set to 7000

    std::string ip = config_data[server];
    servaddr.sin_addr.s_addr = inet_addr(ip.c_str()); 

    //Keep trying to connect to the server until success
    while(connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    }
    char name[1024];
    int tmp = -1;
    memcpy(name,&tmp,sizeof(int));
    int node = selfid;
    memcpy(name+sizeof(int),&node,sizeof(int));
    send(sock_cli,name,sizeof(name),0);
    while(1) {
        //select stuff
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        maxfd = 0;
        FD_SET(sock_cli, &rfds);   
        if(maxfd < sock_cli)
            maxfd = sock_cli;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        retval = select(maxfd+1, &rfds, NULL, NULL, &tv);
        if(retval == -1) {
            printf("select() failed\n");
            break;
        }
        else if(retval == 0) {
            //no meesage from the server
            continue;
        }
        else{
            //receive message from the server
            if(FD_ISSET(sock_cli,&rfds)) {
                char recvbuf[BUFFER_SIZE];
                int len;
                len = recv(sock_cli, recvbuf, sizeof(recvbuf),0);
                if(len==0) {   
                    break;
                }
                if(len==-1) {
                    printf("error\n");

                }
                else {    
                    c.receive(recvbuf);
                }
                memset(recvbuf, 0, sizeof(recvbuf));
            }
        }
    }
}

int main(int argc, char* argv[]) {
    //read config
    selfid = atoi(argv[1]);
    std::ifstream config("./config.txt");
    int number;
    std::string ip;
    std::string name;
    while(config >> number >> ip >> name) {
        if(number == selfid) {
            selfname = name;
        }
        config_data.insert(std::make_pair(number,ip));
        name_number.insert(std::make_pair(name,number));
        number_name.insert(std::make_pair(number,name));
    }
    c.setClient(selfname,selfid,number_name);
    Number_conn.insert(std::make_pair(selfid,0));
    //new socket
    s = socket(AF_INET, SOCK_STREAM, 0);
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    int on = 1;  
    if((setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)))<0) {  
        perror("setsockopt() failed");  
        exit(EXIT_FAILURE);  
    }  
    if(bind(s, (struct sockaddr* ) &servaddr, sizeof(servaddr))==-1) {
        perror("bind() failed");
        exit(1);
    }
    if(listen(s, 20) == -1) {
        perror("listen() failed");
        exit(1);
    }
    len = sizeof(servaddr);

    //Thread to wait connection from other clients
    std::thread t(getConn);
    t.detach();
    //Thread to read the input from the command line as our UI
    std::thread t1(ReadInput);
    t1.detach();
    //Thread to receive the data from the clients
    std::thread t2(getData);
    t2.detach();

    //run different thread as a clinet to each server
    for(int i = 0; i < config_data.size();i++) {
        if(i == selfid) {
            continue;
        }
        std::thread tn(ClientThread,i);
        tn.detach();
    }
    
    


    while(1){

    }
    return 0;
}