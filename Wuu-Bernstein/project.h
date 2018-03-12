/*  filename: project.h
    purpose: head file for TweetClass, EventRecord, Message and Client.
*/
#include <string>
#include <utility>
#include <list>
#include <vector>
#include <set>
#include <fstream>
#include <map>


class TweetClass {
public:
	TweetClass();
	TweetClass(const std::string &name, const std::string &msg, const time_t t);
	time_t getTime() const { return t; }
	const std::string& getUser() const { return user; }
	const std::string& getMsg() const { return message; }

private:
	std::string user;
	std::string message;
	time_t t;
};


class EventRecord {
public:
	EventRecord(const std::string &op, int node, int count);
	EventRecord& operator=(const EventRecord& old);
	std::pair<std::string, std::pair<std::string, std::string> > getOp() const;
	const std::string& getOperation() const{ return operation; }
	int getnode() const{ return node; }
	int getcount() const { return count; }

private:
	std::string operation;
	int node;
	int count;
};

class Message {
public:
	Message(const std::list<EventRecord> &NP,
		      const std::vector<std::vector<int> > &TimeMatrix);
	const std::list<EventRecord>& getNP() const { return NP; }
	const std::vector<std::vector<int> >& getTM() const { return TimeMatrix;}

private:
	std::list<EventRecord> NP;
	std::vector<std::vector<int> > TimeMatrix;
};

class Client {
public:
	Client();

  void setClient(const std::string& name, int n, const std::map<int, std::string> &map);
	void tweet(const std::string &message);
  void sendMsg(int nodej, int sockfd);
	void block(std::string follower);
	void unblock(std::string follower);
	void receive(char * recvline);
	void view();

	void msg2string(char* buf, const Message &msg, int node_to_send);
	Message string2msg(char * str, int &sendNode);
  TweetClass string2tw(const std::string& str);

private:
  void readFile();
  void dicErase(const std::string &user, const std::string &follower);
  void dicWrite(const std::string &user, const std::string &follower);
	void logErase(int i, int c, const std::string& op);
  void logWrite(int i, int c, const std::string& op);
  void twWrite(const TweetClass& tw);
	void tmWrite(const std::vector<std::vector<int> > &tm);

	std::string name;
	int node;
	int count;
  std::map<int, std::string> nameMap;
	std::vector<TweetClass> tweets;
	std::list<EventRecord> PartialLog;
	std::set<std::pair<std::string,std::string> > Dictionary;
	std::vector<std::vector<int> > TimeMatrix;
  std::ofstream outLog;
  std::ofstream outDic;
  std::ofstream outTw;
	std::ofstream outTm;
};

bool hasRec(std::vector<std::vector<int> > T, EventRecord eR, int k);
bool sortByTime(const TweetClass &a, const TweetClass &b);
void printTweet(const TweetClass &tw);
