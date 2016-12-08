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
#include <string.h>

// const values

const std::string serveraddr="127.0.0.1:55826"

// const values

class Slave{
    public:
        Slave();
        std::string ipaddr;//include port. e.g.127.0.0.1:54217
        int counts;
        qLibrary::qSocket::StreamSocket sock(qLibrary::qSocket::SocketDomain::IPV4,qLibrary::qSocket::SocketProtocol::DEFAULT);
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
std::vector<qSocket::StreamSocket> client_connections;
qSocket::qSelector selector;

const int GENERAL_TIMEOUT=5;
const int redundance=0;

// standard redis commands.
int operationSet(std::string key,std::string value);
int operationDelete(std::string key);
int operationExist(std::string key);
int operationGet(std::string key);
int operationMulti();

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
            (client_connections.end()-1)->info.address=buffer;
        }else{
            std::string instruction_buffer;
            // that means an request are sent by a client.
            // let check what it is.
            for( auto x : pr){
                current_sock=x;// for functions to take overall controll
                // get given instructions
                char instruct=x.readchar();
                switch(instruct){
                    case 1:
                        // set
                        unsigned int keylen=x.readuint();
                        std::string key=x.read(keylen);
                        unsigned int valuelen=x.readuint();
                        std::string value=x.read(valuelen);
                        operationSet(key,value);
                        break;
                    case 2:
                        // delete
                        unsigned int keylen=x.readuint();
                        std::string key=x.read(keylen);
                        operationDelete(key);
                        break;
                    case 3:
                        // exist
                        unsigned int keylen=x.readuint();
                        std::string key=x.read(keylen);
                        operationExist(key);
                        break;
                    case 4:
                        // get
                        unsigned int keylen=x.readuint();
                        std::string key=x.read(keylen);
                        operationGet(key);
                        break;
                    case 5:
                        // register
                        unsigned int addrlen=x.readuint();
                        std::string addr=x.read(addrlen);
                        operationRegister(addr);
                        break;
                    case 6:
                        // unregister
                        unsigned int addrlen=x.readuint();
                        std::string addr=x.read(addrlen);
                        operationUnregister(addr);
                        break;
                    case 7:
                        // sync
                        break;
                    case 8:
                        // multi
                        break;
                    case 0;
                        // that occurred when a socket attempt to close(READ ENDS WITH EOF)
                        x.close();
                        for(std::vector<qSocket::StreamSocket>::Iterator i=client_connections.begin;i!=client_connections.end();i++){
                            if((*i).info.fileDescriptor==x.fileDescriptor){
                                client_connections.erase(i);
                            }
                        }
                        break;
                    default:
                        // todo:complete this
                        break;
                    
                }               
            }
        }
        
    }

    return 0;
}

int operationSet(std::string key,std::string value){
    // check if key exists
    auto pos=keys.find(key);
    if(pos!=keys.end()){
        for(auto& ss : pos->second){
            for(auto &sr : slaves){
                if(sr.ipaddr==ss){
                    char[9+key.length()+value.length()] dt;
                    memset(dt,0,9+key.length()+value.length());
                    char i=1;
                    memcpy(dt,&i,1);
                    memcpy(dt+1,&(((unsigned int)key.length())),4);
                    memcpy(dt+5,key.c_str(),key.length());
                    memcpy(dt+5+key.length(),&(((unsigned int)value.length())),4);
                    memcpy(dt+9+key.length(),value.c_str(),value.length());
                    // send
                    std::string cmp(dt);
                    sr.sock.write(cmp);
                }
            }
        }
    }else{
        // find some new slaves to storage this
        int cntl=0;std::vector<Slave> sls;
        do{
            cntl++;
            sls.push_back(slaves.top());
            slaves.pop();
        }while(cntl<=redundance);
        for(auto& sr : sls){
            char[9+key.length()+value.length()] dt;
            memset(dt,0,9+key.length()+value.length());
            char i=1;
            memcpy(dt,&i,1);
            memcpy(dt+1,&(((unsigned int)key.length())),4);
            memcpy(dt+5,key.c_str(),key.length());
            memcpy(dt+5+key.length(),&(((unsigned int)value.length())),4);
            memcpy(dt+9+key.length(),value.c_str(),value.length());
            // send
            std::string cmp(dt);
            sr.sock.write(cmp);
            sr.counts++;
            slaves.push(sr);
        }
    }
    char code=80;
    std::string tmp;
    tmp+=code;
    current_sock.write(tmp);
    return 0;
}

int operationDelete(std::string key){
    // check existence
    auto pos=keys.find(key);
    if(pos!=keys.end()){
         for(auto& ss : pos->second){
            for(auto& sr : slaves){
                if(sr.ipaddr==ss){
                    // get packs up
                    char[5+key.length()] dt;
                    memset(dt,0,5+key.length());
                    char ins=2;
                    memcpy(dt,&ins,1);
                    memcpy(dt+1,&(((unsigned int)key.length())),4);
                    memcpy(dt+5,key.c_str(),key.length());
                    std::string tmp(dt);
                    sr.sock.write(tmp);
                }
            }
         }
         // SUC
         char x=80;
         std::string tmp;
         tmp+=x;
         current_sock.write(tmp);
    }else{
        char x=198;
        std::string tmp;
        tmp+=x;
        current_sock.write(tmp);
    }
}

int operationExist(std::string key){

    auto pos=keys.find(key);
    if(pos!=keys.end()){
         // SUC
         char x=80;
         std::string tmp;
         tmp+=x;
         current_sock.write(tmp);
    }else{
        char x=198;
        std::string tmp;
        tmp+=x;
        current_sock.write(tmp);
    }
}

int operationGet(std::string key){
    auto pos=keys.find(key);
    if(pos!=keys.end()){
        // send requests to check
        qSocket::qSelector gselector;
        for(auto ss : pos->second){
            for(auto sr : slaves){
                if(sr.ipaddr==ss){
                    gselector.r.push_back(sr);
                    char[5+key.length()] dt;
                    memset(dt,0,5+key.length());
                    char ins=4;
                    memcpy(dt,&ins,1);
                    memcpy(dt+1,&(((unsigned int)key.length())),4);
                    memcpy(dt+5,key.c_str(),key.length());
                    std::string tmp(dt);
                    sr.sock.write(tmp);
                }
            }
        }
        // wait for response
        while(1){
            gselector.wait(5,qSocket::qSelectorFlags::READ);
            std::vector<qSocket::StreamSocket> ava=gselector.gets(qSocket::qSelectorFlags::READ);
            for(auto& a : ava){
                char x=a.readchar();
                if(x!=0){
                    std::string tmp;
                    tmp+=x;
                    if(x==80){
                        unsigned int valuelen=a.readuint();
                        std::string rdt=a.read(valuelen);
                        char* cpytmp=(char*)&valuelen;
                        for(int iii=0;iii<4;iii++){
                            tmp+=(*(cpytmp+iii));
                        }
                        tmp+=rdt;
                    }
                    current_sock.write(tmp);
                    break;
                }else{
                    for(std::vector<qSocket::StreamSocket>::Iterator iter=gselector.r.begin();iter!=gselector.r.end();iter++){
                        // enjoy
                        if((*iter).info.fileDescriptor==a.info.fileDescriptor){
                            // get you down
                            gselector.r.erase(iter);
                        }
                    }
                }
                if(gselector.r.size()==0){
                    // oh-ooh
                    std::string tmp;
                    tmp+=(char)40;
                    current_sock.write(tmp);
                    break;
                }
            }
        }
    }else{
        char x=198;
        std::string tmp;
        tmp+=x;
        current_sock.write(tmp);
    }
}

int operationRegister(std::string slaveaddr){
    Slave newservant;
    newservant.ipaddr=slaveaddr;
    try{
        newservant.sock.connect(slaveaddr);
        slaves.push(newservant);
        std::string tmp;
        tmp+=(char)80;
        current_sock.write(tmp);
    }catch(qSocket::StreamSocketConnectException e){
        std::string tmp;
        tmp+=(char)127;
        current_sock.write(tmp);
    }
}

int operationUnregister(std::string slaveaddr){
    bool FLG_ER_SUC=false;
    for(auto iter=slaves.begin();iter!=slaves.end();iter++){
       if((*iter).ipaddr==slaveaddr){
            (*iter).sock.close();
            slaves.erase(iter);
            FLG_ER_SUC=true;
            break;
       }
    }
    std::string tmp;
    if(FLG_ER_SUC){
        tmp+=(char)80;
    }else{
        tmp+=(char)127;
    }
    current_sock.write(tmp);
}
