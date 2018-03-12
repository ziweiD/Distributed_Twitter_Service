#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <thread>
#include <cassert>
#include <string.h>
#include <string>
#include <cstdlib>
#include <algorithm>
#include "site.h"

//==============================================================================
// TweetClass
TweetClass::TweetClass() {
	sender = 0;
	message = "default";
	t = time(0);
}

TweetClass::TweetClass(int id, const std::string &msg, const time_t t) {
	this->sender = id;
	this->message = msg;
	this->t = t;
}

//==============================================================================
// Site
// helper function for sending messages
void Site::prepConvert(char * sendline, int N, long n, int userid) {
	// 0 refers to prepare
	int prep = 0;
  memcpy(sendline, &prep, sizeof(int));
  sendline += sizeof(int);

	// N
	memcpy(sendline, &N, sizeof(int));
	sendline += sizeof(int);

	// n
	memcpy(sendline, &n, sizeof(long));
	sendline += sizeof(long);

	// userid
	memcpy(sendline, &userid, sizeof(int));
	sendline += sizeof(int);
}

void Site::convert(char * sendline, int N, long n, const std::string& v, int userid, int flag) {
	// 1 refers to promise, 2 refers to accept, 3 refers to ack
  memcpy(sendline, &flag, sizeof(int));
  sendline += sizeof(int);

	// N
	memcpy(sendline, &N, sizeof(int));
	sendline += sizeof(int);

	// n
	memcpy(sendline, &n, sizeof(long));
	sendline += sizeof(long);

	// value
	strcpy(sendline,v.c_str());
	sendline += v.size();
	sendline += 1;

	// userid
	memcpy(sendline, &userid, sizeof(int));
	sendline += sizeof(int);
}

void Site::comConvert(char * sendline, int N, const std::string& v, int userid) {
	// 4 refers to prepare
	int commit = 4;
  memcpy(sendline, &commit, sizeof(int));
  sendline += sizeof(int);

	// N
	memcpy(sendline, &N, sizeof(int));
	sendline += sizeof(int);

	// value
	strcpy(sendline,v.c_str());
	sendline += v.size();
	sendline += 1;

	// userid
	memcpy(sendline, &userid, sizeof(int));
	sendline += sizeof(int);
}

// read file: "log.txt" for recovery.
void Site::readFile() {
  std::string value;
  // read log.txt and if there is anything in the file, copy to PartialLog
  std::ifstream inLog("log.txt");
  if (!inLog) {
    std::cerr << "ERROR: can't open log.txt." << '\n';
    exit(1);
  }
  while(getline(inLog, value)) {
    if (inLog.eof()) {
      break;
    }
		accVals.push_back(value);
		accNums.push_back(0);
		maxPrepares.push_back(0);
		myLog.push_back(value);
		CheckInfo(value);
  }
	inLog.close();
}

// check whether this site is leader
bool Site::isLeader() {
	if (findN() == 0) return false;
	int leader = myLog[findN() - 1][0] - '0';
	if (leader == UserID) return true;
	return false;
}

void Site::checkPromise(int logNum) {
	std::this_thread::sleep_for (std::chrono::seconds(5));
	if (promises[logNum].size() < majority) {
		printf("user %d didn't receive promises from majority, start over\n", UserID);
		promises[logNum].clear();
		sendPrepare(logNum);
	}
}

void Site::checkAck(int logNum) {
	std::this_thread::sleep_for (std::chrono::seconds(10));
	if (acks[logNum].size() < majority) {
		printf("user %d: proposal is not accepted.\n", UserID);
		acks[logNum].clear();
		accNums[logNum] = 0;
		accVals[logNum] = "";
	}
}

void Site::removeTweet(int userid) {
	std::vector<TweetClass> tmp;
	for(unsigned int i = 0; i < tweets.size(); i++) {
		if (tweets[i].getUser() != userid) {
			tmp.push_back(tweets[i]);
		}
	}
	tweets = tmp;
}

void Site::addTweet(int userid) {
	for (unsigned int i = 0; i < myLog.size(); i++) {
		if (myLog[i] != "") {
			int id = std::stoi(myLog[i].substr(0, 1));
			std::string value = myLog[i].substr(2, myLog[i].size() - 2);
			std::string check = value.substr(0,5);
			if (check == "tweet") {
				// this is the user who unblock me
				if (id == userid) {
					int i = 6;
					while (isdigit(value[i])) {
						i++;
					}
					time_t t = atol(value.substr(6, i - 6).c_str());
					std::string msg = value.substr(i + 1, value.size() - i - 1);
					TweetClass tw(userid, msg, t);
					tweets.push_back(tw);
				}
			}
		}
	}
}

void Site::sendPrepare(int logNum) {
	if (isLeader()) {
		N = logNum;
		leaderAccept(N);
	}
	else {
		prepare(logNum);
	}
}

int Site::findN() {
	unsigned int i;
	for (i = 0; i < myLog.size(); i++) {
		if (myLog[i].size() == 0) {
			break;
		}
	}
	return (int) i;
}


//==============================================================================
// Constructor
Site::Site()
{
	std::ifstream check_log("log.txt");
	if(!check_log)
	{
		std::cout << "create a log file" << std::endl;
		myFile.open("log.txt");
	}
	else
	{
		std::cout << "log file existed" << std::endl;
		myFile.open("log.txt",std::ios::app);
	}
	this->n = 0;
	this->value = "";
	maxPrepares.push_back(0);
	accNums.push_back(0);
}

//==============================================================================
// operation
void Site::setMajority(int number)
{
	this->majority = number/2+1;
}

void Site::setID(int id) {
	this->UserID = id;
	readFile();
	this->N = findN();
	accVals.push_back("");
	myLog.push_back("");
}

void Site::tweet(const std::string &message) {
	time_t t = time(0);
	this->value = std::to_string(UserID) + " tweet " + std::to_string((long) t) + " " + message;
	sendPrepare(findN());
}

void Site::block(int follower) {
	this->value = std::to_string(UserID) + " block " + std::to_string(follower);
	sendPrepare(findN());
}

void Site::unblock(int follower) {
	this->value = std::to_string(UserID) + " unblock " + std::to_string(follower);
	sendPrepare(findN());
}

void Site::view() {
	// sort the tweets by time to get the right timeline
	sort(tweets.begin(), tweets.end(), sortByTime);

	// for each tweet in the log(vector of tweets), find whether this sites
	// can view.
	for (unsigned int i = 0; i < tweets.size(); i++) {
		printTweet(tweets[i]);
	}
}

void Site::setSocketMap(const std::map<int, int>& map) {
	this->socketMap = map;
}

void Site::recovery(int number) {
	while (number > N) {
		int oldN = N;
		prepare(N);
		while (oldN == N) {
		}
	}
}

//==============================================================================
void Site::view_dict() const
{
	std::set<int>::iterator itr;
	if (blockInfo.size() == 0) {
		std::cout << "No one block this user.\n";
	} else {
		std::cout << "User " << UserID << " is blocked by: ";
		for (itr = blockInfo.begin(); itr != blockInfo.end(); ++itr) {
			std::cout << *itr;
		}
		std::cout << std::endl;
	}
}

//==============================================================================
// acceptor part
void Site::promise(int N, long accnum, const std::string& accval, int userid)
{
	char sendline[1024];
	convert(sendline, N, accnum, accval, UserID,1);
	send(socketMap[userid],sendline, sizeof(sendline),0);
}

void Site::recv_prepare(int N, long n, int userid)
{
	printf("receive prepare(%ld) from %d for number %d log entry\n",n,userid, N);
	while((unsigned int)N >= accNums.size())
	{
		accNums.push_back(0);
		accVals.push_back("");
		maxPrepares.push_back(0);
		myLog.push_back("");
	}
	if(n > maxPrepares[N])
	{
		maxPrepares[N] = n;
		promise(N, accNums[N], accVals[N], userid);
		printf("send promise(%ld,%s) to %d for number %d log entry\n",accNums[N], accVals[N].c_str(),userid, N);
	}
}

void Site::recv_accept(int N, long n, const std::string& value, int userid)
{
	printf("receive accept(%ld,%s) from %d for number %d log entry\n", n, value.c_str(), userid, N);
	while ((unsigned int)N >= accNums.size())
	{
		accNums.push_back(0);
		accVals.push_back("");
		maxPrepares.push_back(0);
		myLog.push_back("");
	}
	if(n >= maxPrepares[N])
	{
		accNums[N] = n;
		accVals[N] = value;
		maxPrepares[N] = n;
		ack(N, accNums[N], accVals[N], userid);
		printf("send ack(%ld,%s) to %d for number %d log entry\n", accNums[N], accVals[N].c_str(), userid, N);
	}
}

void Site::ack(int N, long accnum, const std::string& accval, int userid)
{
	char sendline[1024];
	convert(sendline, N, accnum, accval, UserID,3);
	send(socketMap[userid],sendline, sizeof(sendline),0);
}

void Site::CheckInfo(const std::string& info)
{
	int userid = std::stoi(info.substr(0, 1));
	std::string v = info.substr(2, info.size() - 2);
	std::string check = v.substr(0,5);
	if(check == "block")
	{
		std::string num_str = v.substr(6,1);
		int number = stoi(num_str);
		if (number == this->UserID) {
			blockInfo.insert(userid);
			removeTweet(userid);
		}
	}
	else if(check == "unblo")
	{
		std::string num_str = v.substr(8,1);
		int number = stoi(num_str);
		if (number == this->UserID) {
			assert(blockInfo.find(userid) != blockInfo.end());
			blockInfo.erase(userid);
			addTweet(userid);
		}
	}
	else if (check == "tweet") {
		// if this site isn't blocked by the user send this tweet, add tweet to time line
		if (blockInfo.find(userid) == blockInfo.end()) {
			int i = 6;
			while (isdigit(v[i])) {
				i++;
			}
			time_t t = atol(v.substr(6, i - 6).c_str());
			std::string msg = v.substr(i + 1, v.size() - i - 1);
			TweetClass tw(userid, msg, t);
			tweets.push_back(tw);
		}
	}
}

void Site::recv_commit(int N, const std::string& value, int userid)
{
	printf("receive commit(%s) from %d for number %d log entry\n", value.c_str(), userid, N);
	mutex.lock();
	if (N >= this->N) {
		myLog[N] = value;
		CheckInfo(value);
		write(value);
		this->N = findN();
	}
	mutex.unlock();
}

void Site::write(const std::string& value)
{
	this->myFile << value << std::endl;
	printf("log entry \"%s\" is created\n",value.c_str());
}

//==============================================================================
// proposer part
// send prepare(N, n) to all acceptors
void Site::prepare(int logNum) {
  // choose the next log entry
  //this->N = logNum;
  // choose new proposal number n
  this->n = time(0);

  // send prepare message
  char sendline[1024];
  prepConvert(sendline, logNum, n, UserID);
	std::map<int, int>::iterator iter;
	for (iter = socketMap.begin(); iter != socketMap.end(); ++iter) {
		int sockfd = iter->second;
    int id = iter->first;
    if( send(sockfd, sendline, sizeof(sendline), 0) < 0)
    {
      printf("Send messageg error: %s(errno: %d)\n", strerror(errno), errno);
      exit(0);
    }

    printf("send prepare(%ld) to %d for number %d log entry\n", n, id, logNum);
  }

	// create a thread to check promise
	std::thread promiseThread(&Site::checkPromise, this, logNum);
	promiseThread.detach();
}

// put promise msg into its set. when it receive response from majority,
// choose value and send accept(N,n,v) to all acceptors
void Site::accept(int N, long accNum, const std::string& accVal, int userid) {
  // put promise to a vector
  printf("receive promise(%ld, %s) from %d for number %d log entry\n",accNum, accVal.c_str(), userid, N);
  promises[N][userid] = std::make_pair(accNum, accVal);

  // if there are three promises, choose a value and send accept
  std::map<int, std::pair<long, std::string> > p(promises[N]);
  if (p.size() == majority) {
    std::string v;
    long num = 0;
    // choose a value with largest accNum number
		std::map<int, std::pair<long, std::string> >::iterator itr;
    for (itr = p.begin(); itr != p.end(); itr++) {
      if (itr->second.first > num || (itr->second.first >= num && itr->second.second.size() != 0)) {
        num = itr->second.first;
        v = itr->second.second;
      }
    }
    // if all accVal are NULL, choose its own value
    if (num == 0 && v.size() == 0) {
      num = this->n;
      v = this->value;
    }
		this->value = v;

    // send accept(n,v) to all acceptors
    char sendline[1024];
    convert(sendline, N, this->n, v, UserID, 2);
		std::map<int, int>::iterator iter;
		for (iter = socketMap.begin(); iter != socketMap.end(); ++iter) {
			int sockfd = iter->second;
			int id = iter->first;
      if( send(sockfd, sendline, sizeof(sendline), 0) < 0)
      {
        printf("send messageg error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
      }

      printf("Send accept(%ld, %s) to %d for number %d log entry\n",this->n, v.c_str(), id, N);
		}

		// create a thread
		std::thread ackThread1(&Site::checkAck, this, N);
		ackThread1.detach();
  }
}

// this site is the leader, it will ignore prepare and promise.
void Site::leaderAccept(int N) {
  this->n = 0;
  // send accept(n,v) to all acceptors
  char sendline[1024];
  convert(sendline, N, this->n, this->value, UserID,2);
	std::map<int, int>::iterator iter;
	for (iter = socketMap.begin(); iter != socketMap.end(); ++iter) {
		int sockfd = iter->second;
		int id = iter->first;
    if( send(sockfd, sendline, sizeof(sendline), 0) < 0)
    {
      printf("send messageg error: %s(errno: %d)\n", strerror(errno), errno);
      exit(0);
    }

    printf("Send accept(%ld, %s) to %d for number %d log entry\n",this->n, this->value.c_str(), id, N);
  }

	// create a thread
	std::thread ackThread2(&Site::checkAck, this, N);
	ackThread2.detach();
}

void Site::commit(int N, long accNum, const std::string& accVal, int userid) {
  // put ack to a vector
  printf("receive ack(%ld, %s) from %d for number %d log entry\n",accNum, accVal.c_str(), userid, N);
  acks[N][userid] = std::make_pair(accNum, accVal);

  // if there are three acks (majority), send commit
  if (acks[N].size() == majority) {
    // send commit(v) to all acceptors
    char sendline[1024];
    comConvert(sendline, N, value, UserID);
		std::map<int, int>::iterator iter;
		for (iter = socketMap.begin(); iter != socketMap.end(); ++iter) {
			int sockfd = iter->second;
			int id = iter->first;
      if( send(sockfd, sendline, sizeof(sendline), 0) < 0)
      {
        printf("send messageg error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
      }

      printf("Send commit(%s) to %d for number %d log entry\n", this->value.c_str(), id, N);
    }
  }
}

//==============================================================================
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
	printf("[%d][%s]\n", tw.getUser(), tw.getMsg().c_str());
}
