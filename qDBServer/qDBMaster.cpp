#include "../qSocket/qSocket.hpp"
#include "../qIOStructure/qSelector.hpp"
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
// I can't understand what's going on here,
// but if you comment the following three lines,
// compiler won't let you pass.
#ifndef _UNORDERED_MAP_H
#include <bits/unordered_map.h>
#endif
#include <utility>
#include <list>

// const values

const std::string serveraddr="127.0.0.1:55826"

// const values

class Slave{
    public:
        Slave();
        std::string ipaddr;//include port. e.g.127.0.0.1:54217
        int counts;
};

Slave::Slave(){
    ipaddr="";
    counts=0;
}

class SlaveComparator{
    public:
        bool operator()(const Slave &a,const Slave &b);
};

bool SlaveComparator::operator()(const Slave &a,const Slave &b){
    return a.counts>=b.counts;
}

using namespace qLibrary;

std::priority_queue<Slave,std::vector<Slave>,SlaveComparator> slaves;
std::unordered_map<std::string,std::list<std::string> > keys;
qSocket::Socket* current_sock=NULL;// for operation funcs to operate.
qSocket::Socket server_listener(qSocket::SocketDomain::IPV4,qSocket::SocketType::STREAM,qSocket::SocketProtocol::DEFAULT);// listener
std::vector<qSocket::Socket> slave_connections;
std::vector<qSocket::Socket> client_connections;
qSocket::qSelector selector;

int redundance=0;

// standard redis commands.
int operationSet(std::string key,std::string value);
int operationDelete(std::string key);
int operationExist(std::string key);
int operationGet(std::string key);
int operationMulti();
int operationExecute();

// myself commands
int operationRegister(std::string slaveaddr);// register an slave
int operationSync(std::vector<std::string> keys);// reconnected slave to sync its data with the whole network.
int operationUnregister(std::string slaveaddr);// unregister an slave

int main(void){


    return 0;
}
