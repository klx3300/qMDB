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
qSocket::StreamSocket* current_sock=NULL;// for operation funcs to operate.
qSocket::StreamSocket server_listener(qSocket::SocketDomain::IPV4,qSocket::SocketProtocol::DEFAULT);// listener
std::vector<qSocket::StreamSocket> slave_connections;
std::vector<qSocket::StreamSocket> client_connections;
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
    // create listener socket
    server_listener.bind(serveraddr);
    server_listener.listen();
    selector.r.push_back(server_listener);
    while(1){
        // start listening
        selector.wait(0,qSocket::qSelectorFlags::READ | qSocket::qSelectorFlags::NOEXCEED);
        std::vector<qSocket::StreamSocket> pr=selector.gets(qSocket::qSelectorFlags::READ);
        // check is listened.
        if(selector.checkListener(server_listener.info.fileDescriptor)){
            std::string buffer;
            // accept new connections
            // warning: all accepted connections are marked as [CLIENT]
            // [SLAVE] dbs will be registered by a [CLIENT] and connected by [MASTER]
            client_connections.push_back(server_listener.accept(&buffer));
            std::cout << "[INFO] Connection with " << buffer << " established." << std::endl;
            (client_connections.end-1).info.address=buffer;
        }else{
            std::string instruction_buffer;
            // that means an request are sent by a client.
            // let check what it is.
            for( auto x : pr){
                current_sock=x;// for functions to take overall controll
                // get given instructions
            }
        }
        
    }

    return 0;
}
