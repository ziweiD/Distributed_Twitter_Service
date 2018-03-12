#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <map>
#include <mutex>


class TweetClass {
public:
	TweetClass();
	TweetClass(int userid, const std::string &msg, const time_t t);
	time_t getTime() const { return t; }
	int getUser() const { return sender; }
	const std::string& getMsg() const { return message; }

private:
	int sender;
	std::string message;
	time_t t;
};

class Site
{
public:
	Site();

	void setID(int id);

	// operation
	void tweet(const std::string &message);
	void block(int follower);
	void unblock(int follower);
	void view();
	void setSocketMap(const std::map<int, int>& map);
	void setMajority(int number);
	void recovery(int number);
	int getN() { return N; }

	//log part
	void view_dict() const;

	//acceptor part
	void recv_prepare(int N, long n, int userid);
	void promise(int N, long accnum, const std::string& accval, int userid);
	void recv_accept(int N, long n, const std::string& value, int userid);
	void ack(int N, long accnum, const std::string& accval, int userid);
	void recv_commit(int N, const std::string& value, int userid);
	void write(const std::string& value);
	void CheckInfo(const std::string& value);

	// proposer part
	void prepare(int logNum);
  void accept(int N, long accNum, const std::string& accVal, int userid);
  void leaderAccept(int N);
  void commit(int N, long accNum, const std::string& accVal, int userid);

private:
	// helper function
	// for sending messages
	void prepConvert(char * sendline, int N, long n, int userid);
	void convert(char * sendline, int N, long n, const std::string& v, int userid, int flag);
	void comConvert(char * sendline, int N, const std::string& v, int userid);
	// for readFile
	void readFile();
	// check leader
	bool isLeader();
	// check promise and check ack
	void checkPromise(int logNum);
	void checkAck(int logNum);
	// working on timeline of tweet
	void removeTweet(int userid);
	void addTweet(int userid);
	// send prepare
	void sendPrepare(int logNum);
	int findN();

	std::ofstream myFile;
	std::vector<std::string> myLog;
	std::set<int> blockInfo;
	int UserID;
	std::vector<long> accNums;
	std::vector<std::string> accVals;
	std::string value;
	long n;
	std::vector<long> maxPrepares;
	int N;
	unsigned int majority;
	std::map<int, std::map<int, std::pair<long, std::string> > > promises;
  std::map<int, std::map<int, std::pair<long, std::string> > > acks;
	std::vector<TweetClass> tweets;
	std::map<int, int> socketMap;
	std::mutex mutex;
};

bool sortByTime(const TweetClass &a, const TweetClass &b);
void printTweet(const TweetClass &tw);
