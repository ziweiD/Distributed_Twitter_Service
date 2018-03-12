/* file name: project.cpp
   purpose: implementaion file of project.h
*/
#include "project.h"
#include <stdio.h>
#include <algorithm>
#include <iostream>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <cassert>


//==============================================================================
// TweetClass
TweetClass::TweetClass() {
	user = "default";
	message = "default";
	t = time(0);
}

TweetClass::TweetClass(const std::string &name,
	                     const std::string &msg,
											 const time_t t) {
	this->user = name;
	this->message = msg;
	this->t = t;
}

//==============================================================================
// EventRecord
EventRecord::EventRecord(const std::string& op, int node, int count) {
	this->operation = op;
	this->node = node;
	this->count = count;
}

EventRecord& EventRecord::operator=(const EventRecord& old) {
	this->operation = old.getOperation();
	this->node = old.getnode();
	this->count = old.getcount();
	return *this;
}

// modify the operation string into two part, the first is the operation name
// such as: block, unblock and tweet. The second is a pair. For the block and
// unblock events, the first string of the pair is the user name and the second
// string of the pair is the follower name. For the tweet event, only the first
// string of the pair is useful and it's the information of the tweet. The second
// string is just "NULL".
std::pair<std::string, std::pair<std::string, std::string> > EventRecord::getOp() const {
	int i = 0;
	while (operation[i] != '(') {
		i++;
	}
	std::string op(operation, 0, i);
	int j = i;
	if (op == "block" || op == "unblock") {
		while (operation[j] != ',') {
			j++;
		}
		std::string user(operation, i + 1, j - i - 1);
		std::string follower(operation, j+1, operation.size() - j - 2);
		return std::make_pair(op, std::make_pair(user, follower));
	}
  else if (op == "tweet") {
		while (operation[j] != ')') {
			j++;
		}
    // if this is tweet, get the tweet information
		std::string info(operation, i + 1, operation.size() - 7);
		std::string other("NULL");
		return std::make_pair(op, std::make_pair(info, other));
	}
  return std::make_pair("NULL", std::make_pair("NULL", "NULL"));
}

//==============================================================================
// Message
Message::Message(const std::list<EventRecord> &np,
				         const std::vector<std::vector<int> > &TimeMatrix) {
	std::copy(np.begin(), np.end(),
          std::back_insert_iterator<std::list<EventRecord> >(NP));
	this->TimeMatrix = TimeMatrix;
}

//==============================================================================
// Client
// read four files: "log.txt", "dictionary.txt", "tweet.txt" and "timeMatrix.txt"
// for recovery.
void Client::readFile() {
  std::string s;
  char ch;
  // read log.txt and if there is anything in the file, copy to PartialLog
  std::ifstream inLog("log.txt");
  if (!inLog) {
    std::cerr << "ERROR: can't open log.txt." << '\n';
    exit(1);
  }
  while(inLog >> ch) {
    if (inLog.eof()) {
      break;
    }
    int node, count;
    inLog >> s;
    inLog >> s;
    inLog >> node;
    inLog >> s;
    inLog >> s;
    inLog >> count;
    getline(inLog, s);
    std::string op(s, 2, s.size() - 2);
    EventRecord eR(op, node, count);
    PartialLog.push_back(eR);
  }
  // read dictionary.txt and if there is anything in the file, copy to Dictionary
  std::ifstream inDic("dictionary.txt");
  if (!outDic) {
    std::cerr << "ERROR: can't open dictionary.txt." << '\n';
    exit(1);
  }
  while(inDic >> ch) {
    if (inDic.eof()) {
      break;
    }
    inDic >> s;
    std::string user(s, 0, s.size() - 1);
    inDic >> s;
    std::string follower(s, 0, s.size() - 1);
    Dictionary.insert(std::make_pair(user, follower));
  }
  // read tweets.txt and if there is anything in the file, copy to tweets
  std::ifstream inTw("tweet.txt");
  if (!inTw) {
    std::cerr << "ERROR: can't open tweet.txt." << '\n';
    exit(1);
  }
  while (inTw >> ch) {
    if (inTw.eof()) {
      break;
    }
    std::string user;
    inTw >> user;
    std::string tm;
    inTw >> tm;
    time_t t = std::stol(tm);
    std::string msg;
    getline(inTw, msg);
    TweetClass tw(user, msg.substr(1, msg.size() - 2), t);
    tweets.push_back(tw);
  }
	// read timeMatrix.txt and if there is anything in the file, copy to TimeMatrix
  std::ifstream inTm("timeMatrix.txt");
  if (!inTm) {
    std::cerr << "ERROR: can't open timeMatrix.txt." << '\n';
    exit(1);
  }
	inTm >> s;
	if (!inTm.eof()) {
		int size = std::stoi(s);
		while (inTm >> ch) {
			if (inTm.eof()) {
				break;
			}
			int i = size;
			std::vector<int> tmp;
			while (i > 0) {
				inTm >> s;
				tmp.push_back(std::stoi(s));
				i--;
			}
			TimeMatrix.push_back(tmp);
		}
	}
  inLog.close();
  inDic.close();
  inTw.close();
	inTm.close();
}

// Erase the entry of "dictionary.txt"
void Client::dicErase(const std::string &user, const std::string &follower) {
  std::ifstream inDic("dictionary.txt");
  std::string s;
  std::ofstream outfile("tmp.txt",std::ios::out|std::ios::trunc);
  while(inDic >> s) {
    if (inDic.eof()) {
      break;
    }
    std::string u(s, 1, s.size() - 2);
    inDic >> s;
    std::string f(s, 0, s.size() - 1);
		/* check whether the name of user and follower is the same of what
		   we want to erase. If it is not the same pair, write to a new file. */
    if (u != user || f != follower) {
      outfile << "[" << u << ", " << f << "]\n";
    }
  }
  outfile.close();
  inDic.close();
	// delete the old dictionary.txt
	std::string filename("dictionary.txt");
  if (remove(filename.c_str()) != 0) {
    perror("Error in remove file");
  }
	// rename the new file to the dictionary.txt
	char oldname[] = "tmp.txt";
	char newname[] = "dictionary.txt";
	int result = rename(oldname, newname);
	if ( result != 0 ) {
		perror( "Error renaming file" );
	}
}

// write the entry to the dictionary.txt file in the following format:
// [user, follower]
void Client::dicWrite(const std::string &user, const std::string &follower) {
  outDic.open("dictionary.txt", std::ios::app);
  if (!outDic) {
    std::cerr << "ERROR: can't open log.txt." << '\n';
    exit(1);
  }
  outDic << "[" << user << ", " << follower << "]\n";
  outDic.close();
}

// Erase the given EventRecord of "log.txt"
void Client::logErase(int i, int c, const std::string& op) {
	std::ifstream inLog("log.txt");
  std::string s;
  std::ofstream outfile("tmp2.txt",std::ios::out|std::ios::trunc);
  while(inLog >> s) {
    if (inLog.eof()) {
      break;
    }
		assert(s == "[i");
		int node,count;
		inLog >> s;
		assert(s == "=");
		inLog >> s;
		node = std::stoi(s);
		inLog >> s;
		assert(s == "=");
		getline(inLog, s);
		count = std::stoi(s);
		int j = 0;
		while (s[j] != '[') {
			j++;
		}
    std::string operation(s, j+1, s.size() - j - 2);
		// check whether this EventRecord is the one that we want to erase, if not
		// write it to the new file.
    if (node != i || count != c || operation != op) {
      outfile << "[i = " << node << "][C = " << count << "][" << operation << "]\n";
    }
  }
  outfile.close();
  inLog.close();
	// delete the old "log.txt" file
	std::string filename("log.txt");
  if (remove(filename.c_str()) != 0) {
    perror("Error in remove file");
  }
	// rename the new file to "log.txt"
	char oldname[] = "tmp2.txt";
	char newname[] = "log.txt";
	int result = rename(oldname, newname);
	if ( result != 0 ) {
		perror( "Error renaming file" );
	}
}

// write the EventRecord to the log.txt file in the following format:
// [i = 0][c = 0][operation]
void Client::logWrite(int i, int c, const std::string& op) {
  outLog.open("log.txt", std::ios::app);
  if (!outLog) {
    std::cerr << "ERROR: can't open log.txt." << '\n';
    exit(1);
  }
  outLog << "[i = " << i << "][C = " << c << "][" << op << "]\n";
  outLog.close();
}

// write the tweet to the tweet.txt file in the following format:
// [user time message]
void Client::twWrite(const TweetClass& tw) {
  outTw.open("tweet.txt", std::ios::app);
  if (!outTw) {
    std::cerr << "ERROR: can't open tweet.txt." << '\n';
    exit(1);
  }
  outTw << "[" << tw.getUser() << " " << tw.getTime() << " " <<
        tw.getMsg() << "]"<< std::endl;
  outTw.close();
}

// write the matrix clock to the "timeMatrix.txt" file in the following format:
// size
// [0 0]
// [0 0]
void Client::tmWrite(const std::vector<std::vector<int> > &tm) {
	outTm.open("timeMatrix.txt", std::ios::trunc);
	if (!outTm) {
		std::cerr << "ERROR: can't open timeMatrix.txt." << '\n';
		exit(1);
	}
	outTm << TimeMatrix.size() << std::endl;
	for (int i = 0; i < TimeMatrix.size(); i++) {
		outTm << "[";
		for (int j = 0; j < TimeMatrix[i].size(); j++) {
			outTm << TimeMatrix[i][j];
			if (j != TimeMatrix[i].size() - 1) {
				outTm << " ";
			}
		}
		outTm << "]\n";
	}
	outTm.close();
}

// Constructor. Open the four files, if the files are not existed, create them.
Client::Client()
{
  // if there is no file, create the file
  outLog.open("log.txt", std::ios::app);
  if (!outLog) {
    std::cerr << "ERROR: can't open log.txt." << '\n';
    exit(1);
  }
  outDic.open("dictionary.txt", std::ios::app);
  if (!outDic) {
    std::cerr << "ERROR: can't open dictionary.txt." << '\n';
    exit(1);
  }
  outTw.open("tweet.txt", std::ios::app);
  if (!outTw) {
    std::cerr << "ERROR: can't open tweet.txt." << '\n';
    exit(1);
  }
	outTm.open("timeMatrix.txt", std::ios::app);
  if (!outTw) {
    std::cerr << "ERROR: can't open timeMatrix.txt." << '\n';
    exit(1);
  }
  outLog.close();
  outDic.close();
  outTw.close();
	outTm.close();
	// recovery from the old file. If the files are empty, do nothing.
  readFile();
}

// initialize the Client. Set the user name, node number and the map of other
// node number and its corresponding user name. Set the local time counter to be
// 0 and if the TimeMatrix has no element, set all elements to be 0;
void Client::setClient(const std::string& name, int n,
                       const std::map<int, std::string> &map) {
  this->name = name;
  node = n;
  nameMap = map;

	// initialize
	if (TimeMatrix.size() == 0) {
    count = 0;
		for (int j = 0; j < nameMap.size(); j++) {
	    std::vector<int> v;
	    for (int k = 0; k < nameMap.size(); k++) {
	      v.push_back(0);
	    }
	    TimeMatrix.push_back(v);
	  }
	} else {
    count = TimeMatrix[node][node];
  }
}

// Insert the given pair to Dictionary
void Client::block(std::string follower)
{
  std::pair<std::set<std::pair<std::string,std::string> >::iterator, bool> inst;
	inst = Dictionary.insert(std::make_pair(name,follower));
	// if insert is success, tick the counter, update the TimeMatrix and put into log
  if (inst.second) {
    dicWrite(name, follower); // add this entry to dictionary.txt
    std::string tmp = "block(" + name + "," + follower + ")";
    count++;
    TimeMatrix[node][node]++;
    tmWrite(TimeMatrix);  // backup the TimeMatrix to the file
    EventRecord eR(tmp, node, count);
    PartialLog.push_back(eR);
    logWrite(node, count, tmp);  // add this eR to the log.txt file
  }
}

// delete the given pair from Dictionary
void Client::unblock(std::string follower)
{
  std::set<std::pair<std::string,std::string> >::iterator iter;
  iter = Dictionary.find(std::make_pair(name,follower));
  if (iter != Dictionary.end()) { // if find an element in the Dictionary
    Dictionary.erase(iter);
    dicErase(name, follower); // erase this entry to dictionary.txt
    std::string tmp = "unblock(" + name + "," + follower + ")";
    count++;
    TimeMatrix[node][node]++;
    tmWrite(TimeMatrix);   // backup the TimeMatrix to the file
    EventRecord eR(tmp, node, count);
    PartialLog.push_back(eR);
    // add this eR to the log.txt file
    logWrite(node, count, tmp);
  }
}

// create a new tweet and save its EventRecord to log
void Client::tweet(const std::string &message)
{
	// create a tweet include user, message and physical time
	time_t t = time(0); // get the physical time
	TweetClass tw(name, message, t);
  tweets.push_back(tw);
  twWrite(tw);

  // put tweet event into PartialLog
  long n = t;
  std::string str = "tweet(" + name + " " + std::to_string(n) + " " + message + ")";
	count++;
	TimeMatrix[node][node]++;
	tmWrite(TimeMatrix);
	EventRecord eR(str, node, count);
	PartialLog.push_back(eR);
  logWrite(node, count, str);
}

// nodej: receive end.
// if nodej is not blocked by me, I will create a NP and package it with the
// TimeMatrix into a Message object and convert it to char* and send it.
void Client::sendMsg(int nodej, int sockfd) {
  std::string follower = nameMap[nodej];
  std::set<std::pair<std::string,std::string> >::iterator iter;
  // check if the follower is blocked by me
  iter = Dictionary.find(std::make_pair(name, follower));
  if (iter == Dictionary.end()) { // not blocked, send message
    // create a NP
  	std::list<EventRecord> NP;
  	std::list<EventRecord>::iterator it;
  	for (it = PartialLog.begin(); it != PartialLog.end(); ++it) {
  		if (!hasRec(TimeMatrix, *it, nodej)) {
  			NP.push_back(*it);
  		}
  	}

  	// create the whole sending message and transfer the msg to a string
  	Message msg(NP, TimeMatrix);
  	char sendline[1024];
  	msg2string(sendline, msg, nodej);

		// send the msg
    if( send(sockfd, sendline, sizeof(sendline), 0) < 0)
  	{
  	printf("send messageg error: %s(errno: %d)\n", strerror(errno), errno);
  	exit(0);
  	}
  }
}

// receive the message and update PartialLog, Dictionary and tweets
void Client::receive(char * recvline)
{
	// convert the receive string to a Message
	int sendNode;
	Message msg = string2msg(recvline, sendNode);

	// create NE
	std::list<EventRecord> NE;
	std::list<EventRecord> NP = msg.getNP();
	std::list<EventRecord>::iterator fR;
	for (fR = NP.begin(); fR != NP.end(); ++fR) {
		if (!hasRec(TimeMatrix, *fR, node)) {
			NE.push_back(*fR);
		}
	}

	// update TimeMatrix
	std::vector<std::vector<int> > recvT = msg.getTM();
	for (int k = 0; k < nameMap.size(); k++) {
		TimeMatrix[node][k] = std::max(TimeMatrix[node][k], recvT[sendNode][k]);
	}
	for (int k = 0; k < nameMap.size(); k++) {
		for (int l = 0; l < nameMap.size(); l++) {
			TimeMatrix[k][l] = std::max(TimeMatrix[k][l], recvT[k][l]);
		}
	}
	tmWrite(TimeMatrix);

	// update Dictonary
	std::list<EventRecord>::iterator dR;
	for (dR = NE.begin(); dR != NE.end(); ++dR) {
		// push_back all eR in NE into PL (NE U PL)
		PartialLog.push_back(*dR);
		logWrite(dR->getnode(), dR->getcount(), dR->getOperation());

    std::pair<std::string, std::string> pa = dR->getOp().second;
    std::set<std::pair<std::string,std::string> >::iterator v;
		// the event is block
		if (dR->getOp().first == "block") {
      v = Dictionary.find(pa);
			// if didn't find the entry in Dictionary, insert this entry
			if (v == Dictionary.end()) {
				Dictionary.insert(pa);
        dicWrite(pa.first, pa.second);
			}
		}
		// the event is unblock
		else if (dR->getOp().first == "unblock") {
      v = Dictionary.find(pa);
			// if find the entry in Dictionary, delete this entry
			if (v != Dictionary.end()) {
				Dictionary.erase(pa);
        dicErase(pa.first, pa.second);
			}
		}
		// if the event is tweet, add tweet to the local vector
		else if (dR->getOp().first == "tweet") {
			std::string info = dR->getOp().second.first;
			TweetClass tw = string2tw(info);
			twWrite(tw);
			tweets.push_back(tw);
		}
	}

	// update PartialLog
	std::list<EventRecord>::iterator eR = PartialLog.begin();
  while (eR != PartialLog.end()) {
    bool find = false;
    for (int k = 0; k  < nameMap.size(); k++) {
      if (!hasRec(TimeMatrix, *eR, k)) {
        find = true;
        break;
      }
    }
		// if all sites knows about this event and the event is block or unblock
		// delete this event from the PartialLog
    if (!find && eR -> getOp().first != "tweet") {
      logErase(eR->getnode(), eR->getcount(), eR->getOperation());
      eR = PartialLog.erase(eR);
    } else {
      eR++;
    }
  }
}

// view the tweets which creator didn't blocked this site
void Client::view() {
	// sort the tweets by time to get the right timeline
	sort(tweets.begin(), tweets.end(), sortByTime);
	std::vector<TweetClass>::iterator iter;
	// for each tweet in the log(vector of tweets), find whether this sites
	// can view.
	for (iter = tweets.begin(); iter != tweets.end(); ++iter) {
		std::set<std::pair<std::string,std::string> >::iterator v;
    v = Dictionary.find(std::make_pair(iter->getUser(), name));
		if (v == Dictionary.end()) { // if I didn't be blocked by the tweet creator
			printTweet(*iter);
		}
	}
}

// For sending the message, we convert the message to string
void Client::msg2string(char * buf, const Message& msg, int node_to_send) {
  //node from
  int my_node = this->node;
  memcpy(buf, &my_node, sizeof(int));
  buf += sizeof(int);
  //node_to_send
  int node_to = node_to_send;
  memcpy(buf, &node_to, sizeof(int));
  buf += sizeof(int);

	//NP
	std::list<EventRecord> NP = msg.getNP();
	int NPsize = NP.size();
	memcpy(buf,&NPsize,sizeof(int));
	buf += sizeof(int);
  std::list<EventRecord>::iterator iter;
  for (iter = NP.begin(); iter != NP.end(); ++iter) {
    std::string operation = iter->getOperation();
		int node_NP = iter->getnode();
		int count_NP = iter->getcount();
		strcpy(buf,operation.c_str());
		buf += operation.size();
		buf += 1;
		memcpy(buf,&node_NP,sizeof(int));
		buf += sizeof(int);
		memcpy(buf,&count_NP,sizeof(int));
		buf += sizeof(int);
  }

	//TimeMatrix
	std::vector<std::vector<int> > TM = msg.getTM();
	int TMsize = TM.size();
	memcpy(buf,&TMsize,sizeof(int));
	buf += sizeof(int);
	for(int i = 0; i < TMsize; i++) {
		for(int j = 0; j < TMsize; j++) {
			int value = TM[i][j];
			memcpy(buf,&value,sizeof(int));
			buf += sizeof(int);
		}
	}
}

// For receiving the message, we convert the receiving char * into Message
Message Client::string2msg(char* str, int &sendNode) {
	//from who
	int from_who = 0;
	memcpy(&from_who,str,sizeof(int));
	sendNode = from_who;
	str += sizeof(int);
	//to me
	str += sizeof(int);
	//Message

	//NP
	int NPsize = 0;
	memcpy(&NPsize,str,sizeof(int));
	str += sizeof(int);
	std::list<EventRecord> NP;
	for(int i = 0; i < NPsize; i++) {
		std::string operation(str);
		str = str + operation.size() + 1;
		int node = 0;
		memcpy(&node, str, sizeof(int));
		str += sizeof(int);
		int count = 0;
		memcpy(&count, str, sizeof(int));
		str += sizeof(int);
		EventRecord eR(operation, node, count);
		NP.push_back(eR);
	}

	//TimeMatrix
	int TMsize = 0;
	memcpy(&TMsize, str, sizeof(int));
	str += sizeof(int);
	std::vector<std::vector<int> > TM;
	for(int i = 0; i < TMsize; i++) {
		std::vector<int> TM_row;
		for(int j = 0; j < TMsize; j++) {
			int value = 0;
			memcpy(&value,str,sizeof(int));
			str += sizeof(int);
			TM_row.push_back(value);
		}
		TM.push_back(TM_row);
	}

	Message m(NP, TM);
	return m;
}

// convert the string stored information of a tweet into a TweetClass
TweetClass Client::string2tw(const std::string& str) {
  int i = 0;
  while(str[i] != ' ') {
    i++;
  }
  std::string user(str, 0, i);
  int j = i + 1;
  while(str[j] != ' ') {
    j++;
  }
  std::string tm(str, i + 1, j - i - 1);
  time_t t = std::stol(tm);
  std::string message(str, j + 1, str.size() - j - 1);

	TweetClass tw(user, message, t);
  return tw;
}

//==============================================================================
bool hasRec(std::vector<std::vector<int> > T, EventRecord eR, int k) {
  return T[k][eR.getnode()] >= eR.getcount();
}

// helper function: if a is bigger than b, return true. (b is more recent than a)
bool sortByTime(const TweetClass &a, const TweetClass &b) {
	time_t ta = a.getTime(), tb = b.getTime();
	return difftime(ta,tb) > 0.0 ? true : false;
}

// hepler function: print the tweet
void printTweet(const TweetClass &tw) {
  char tmp[64];
	time_t t = tw.getTime();
  strftime( tmp, sizeof(tmp), "%Y/%m/%d %X", localtime(&t));
  puts( tmp );
	printf("[%s][%s]\n", tw.getUser().c_str(), tw.getMsg().c_str());
}
