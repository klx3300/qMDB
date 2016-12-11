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
#include <iostream>
#include <errno.h>

// const values

const std::string serveraddr="127.0.0.1:45175";

// const values

void bsafe_add(std::string& targetstr,char* src,int size){
    for(int i=0;i<size;i++){
        targetstr+=(*(src+i));
    }
}

class Slave{
    public:
        Slave();
        std::string ipaddr;//include port. e.g.127.0.0.1:54217
        int counts;
        qLibrary::qSocket::StreamSocket sock;
};

Slave::Slave(){
    sock.info.domain=qLibrary::qSocket::SocketDomain::IPV4;
    sock.info.type=qLibrary::qSocket::SocketType::STREAM;
    sock.info.protocol=qLibrary::qSocket::SocketProtocol::DEFAULT;
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
std::unordered_map<std::string,Slave> slavedb;
qSocket::StreamSocket current_sock;// for operation funcs to operate.
qSocket::StreamSocket server_listener;// listener
std::vector<qSocket::StreamSocket> client_connections;
std::vector<qSocket::StreamSocket> unidentified_connections;
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
    server_listener.info.domain=qSocket::SocketDomain::IPV4;
    server_listener.info.type=qSocket::SocketType::STREAM;
    server_listener.info.protocol=qSocket::SocketProtocol::DEFAULT;
    try{
    server_listener.open().bind(serveraddr);
    }catch(qSocket::SocketBindException e){
        std::cout << e.errmsg << " ERRNO is " << errno << std::endl;
    }
    server_listener.listen();
    while(1){
        // start listening
        selector.r.clear();
        selector.r.push_back(server_listener);
        for(auto cc : client_connections){
            selector.r.push_back(cc);
        }
        for(auto uc: unidentified_connections){
            selector.r.push_back(uc);
        }
        selector.wait(0,qSocket::qSelectorFlags::READ | qSocket::qSelectorFlags::NOEXCEED);
        std::vector<qSocket::StreamSocket> pr=selector.gets(qSocket::qSelectorFlags::READ);
        // check is listened.
        if(selector.checkListener(server_listener.info.fileDescriptor)){
            std::string buffer;
            // accept new connections
            unidentified_connections.push_back(server_listener.accept(buffer));
            std::cout << "[INFO] Connection with " << buffer << " established." << std::endl;
            (unidentified_connections.end()-1)->info.address=buffer;
        }else{
            std::string instruction_buffer;
            // that means an request are sent by a client.
            // let check what it is.
            for( auto x : pr){
                current_sock=x;// for functions to take overall controll
                // get given instructions
                char instruct=x.readchar();
                switch(instruct){
                    case 1:{
                        // set
                        unsigned int keylen=x.readuint();
                        std::string key=x.read(keylen);
                        unsigned int valuelen=x.readuint();
                        std::string value=x.read(valuelen);
                        x.acceptBeat();
                        operationSet(key,value);
                        break;}
                    case 2:{
                        // delete
                        unsigned int keylen=x.readuint();
                        std::string key=x.read(keylen);
                        x.acceptBeat();
                        operationDelete(key);
                        break;}
                    case 3:{
                        // exist
                        unsigned int keylen=x.readuint();
                        std::string key=x.read(keylen);
                        x.acceptBeat();
                        operationExist(key);
                        break;}
                    case 4:{
                        // get
                        std::cout << "[INFO] Received GET " << std::endl;
                        unsigned int keylen=x.readuint();
                        std::string key=x.read(keylen);
                        x.acceptBeat();
                        operationGet(key);
                        break;}
                    case 5:{
                        // register
                        //unsigned int addrlen=x.readuint();
                        //std::string addr=x.read(addrlen);
                        //x.acceptBeat();
                        //operationRegister(addr);
                        break;}
                    case 6:{
                        // unregister
                        unsigned int addrlen=x.readuint();
                        std::string addr=x.read(addrlen);
                        x.acceptBeat();
                        operationUnregister(addr);
                        break;}
                    case 7:{
                        // sync
                        break;}
                    case 8:{
                        // multi
                        break;}
                    case 0:{
                        // that occurred when a socket attempt to close(READ ENDS WITH EOF)
                        std::cout << "[INFO] " << x.info.address << " disconnected." << std::endl;
                        x.close();
                        bool FLG=false;
                        for(std::vector<qSocket::StreamSocket>::iterator i=client_connections.begin();i!=client_connections.end();i++){
                            if((*i).info.fileDescriptor==x.info.fileDescriptor){
                                client_connections.erase(i);FLG=true;break;
                            }
                        }
                        if(FLG)break;
                        FLG=false;
                        for(std::vector<qSocket::StreamSocket>::iterator i=unidentified_connections.begin();i!=unidentified_connections.end();i++){
                            if((*i).info.fileDescriptor==x.info.fileDescriptor){
                                unidentified_connections.erase(i);FLG=true;break;
                            }
                        }
                        if(FLG)break;
                        slavedb.erase(x.info.address);
                        break;}
                    case 41:{
                        // remove you from unauthed
                        for(std::vector<qSocket::StreamSocket>::iterator uc=unidentified_connections.begin();uc!=unidentified_connections.end();uc++){
                            if((*uc).info.fileDescriptor==x.info.fileDescriptor){
                                unidentified_connections.erase(uc);
                                break;
                            }
                        }
                        Slave sl;
                        sl.sock=x;
                        sl.ipaddr=x.info.address;
                        slaves.push(sl);
                        slavedb[sl.ipaddr]=sl;
                        std::cout << "[INFO] " << sl.ipaddr << " declared himself SLAVE." << std::endl;
                        x.acceptBeat();
                        break;}
                    case 42:{
                        for(std::vector<qSocket::StreamSocket>::iterator uc=unidentified_connections.begin();uc!=unidentified_connections.end();uc++){
                            if((*uc).info.fileDescriptor==x.info.fileDescriptor){
                                unidentified_connections.erase(uc);
                                break;
                            }
                        }
                        client_connections.push_back(x);
                        std::cout << "[INFO] " << x.info.address << " declared himself CLIENT." << std::endl;
                        x.acceptBeat();
                        break;}
                    default:
                        // todo:complete this
                        x.acceptBeat();
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
            char dt[9+key.length()+value.length()];
            memset(dt,0,9+key.length()+value.length());
            char i=1;
            unsigned int ktmp;
            memcpy(dt,&i,1);
            ktmp=(unsigned int)key.length();
            memcpy(dt+1,&ktmp,4);
            memcpy(dt+5,key.c_str(),key.length());
            ktmp=value.length();
            memcpy(dt+5+key.length(),&ktmp,4);
            memcpy(dt+9+key.length(),value.c_str(),value.length());
            // send
            std::string cmp;
            bsafe_add(cmp,dt,9+key.length()+value.length());
            if(slavedb.find(ss)!=slavedb.end()){
                slavedb[ss].sock.write(cmp);
                slavedb[ss].sock.sendBeat();
            }
        } 
        
    }else{
        // find some new slaves to storage this
        int cntl=0;std::vector<Slave> sls;
        do{
            cntl++;
            while(slavedb.find(slaves.top().ipaddr)==slavedb.end())
                slaves.pop();
            sls.push_back(slaves.top());
            slaves.pop();
        }while(cntl<=redundance);
        for(Slave sr : sls){
            char dt[9+key.length()+value.length()];
            memset(dt,0,9+key.length()+value.length());
            char i=1;
            unsigned int ktmp=key.length();
            memcpy(dt,&i,1);
            memcpy(dt+1,&ktmp,4);
            memcpy(dt+5,key.c_str(),key.length());
            ktmp=value.length();
            memcpy(dt+5+key.length(),&ktmp,4);
            memcpy(dt+9+key.length(),value.c_str(),value.length());
            // send
            std::string cmp;
            bsafe_add(cmp,dt,9+key.length()+value.length());
            sr.sock.write(cmp);
            sr.sock.sendBeat();
            sr.counts++;
            slaves.push(sr);
            keys[key].push_back(sr.ipaddr);
        }

    }
    char code=80;
    std::string tmp;
    tmp+=code;
    current_sock.write(tmp);
    current_sock.sendBeat();
    return 0;
}

int operationDelete(std::string key){
    // check existence
    auto pos=keys.find(key);
    if(pos!=keys.end()){
        for(auto& ss : pos->second){
            // get packs up
            char dt[5+key.length()];
            memset(dt,0,5+key.length());
            char ins=2;
            unsigned int ktmp=key.length();
            memcpy(dt,&ins,1);
            memcpy(dt+1,&ktmp,4);
            memcpy(dt+5,key.c_str(),key.length());
            std::string tmp;
            bsafe_add(tmp,dt,5+key.length());
            if(slavedb.find(ss)!=slavedb.end()){
                slavedb[ss].sock.write(tmp);
                slavedb[ss].sock.sendBeat();
            }
        }
        keys.erase(key);
        // SUC
        char x=80;
        std::string tmp;
        tmp+=x;
        current_sock.write(tmp);
        current_sock.sendBeat();
    }else{
        char x=198;
        std::string tmp;
        tmp+=x;
        current_sock.write(tmp);
        current_sock.sendBeat();
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
         current_sock.sendBeat();
    }else{
        char x=198;
        std::string tmp;
        tmp+=x;
        current_sock.write(tmp);
        current_sock.sendBeat();
    }
}

int operationGet(std::string key){
    auto pos=keys.find(key);
    if(pos!=keys.end()){
        // send requests to check
        qSocket::qSelector gselector;
        for(auto ss : pos->second){
            if(slavedb.find(ss)==slavedb.end())continue;
            gselector.r.push_back(slavedb[ss].sock);
            char dt[5+key.length()];
            memset(dt,0,5+key.length());
            char ins=4;
            unsigned int ktmp=key.length();
            memcpy(dt,&ins,1);
            memcpy(dt+1,&ktmp,4);
            memcpy(dt+5,key.c_str(),key.length());
            std::string tmp;
            bsafe_add(tmp,dt,5+key.length());
            slavedb[ss].sock.write(tmp);
            slavedb[ss].sock.sendBeat();
        }
        if(gselector.r.size()==0){
            std::string tmp;
            tmp+=(char)40;
            current_sock.write(tmp);
            current_sock.sendBeat();
            return 0;
        }
        // wait for response
        while(1){
            int statusno=gselector.wait(2,qSocket::qSelectorFlags::READ);
            if(statusno==0){// time limit exceeded
                std::string tmp;
                tmp+=(char)40;
                current_sock.write(tmp);
                current_sock.sendBeat();
                break;
            }
            std::vector<qSocket::StreamSocket> ava=gselector.gets(qSocket::qSelectorFlags::READ);
            for(auto& a : ava){
                char x=a.readchar();
                if(x!=0){
                    std::string tmp;
                    tmp+=x;
                    if(x==80){
                        unsigned int valuelen=a.readuint();
                        std::string rdt=a.read(valuelen);
                        a.acceptBeat();
                        char* cpytmp=(char*)&valuelen;
                        for(int iii=0;iii<4;iii++){
                            tmp+=(*(cpytmp+iii));
                        }
                        tmp+=rdt;
                    }else{
                    }
                    current_sock.write(tmp);
                    current_sock.sendBeat();
                    return 0;
                }else{
                    for(std::vector<qSocket::StreamSocket>::iterator iter=gselector.r.begin();iter!=gselector.r.end();iter++){
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
                    current_sock.sendBeat();
                    return 0;
                }
            }
        }
    }else{
        char x=198;
        std::string tmp;
        tmp+=x;
        current_sock.write(tmp);
        current_sock.sendBeat();
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
        current_sock.sendBeat();
    }catch(qSocket::StreamSocketConnectException e){
        std::string tmp;
        tmp+=(char)127;
        current_sock.write(tmp);
        current_sock.sendBeat();
    }
}

int operationUnregister(std::string slaveaddr){
    /*bool FLG_ER_SUC=false;
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
    current_sock.write(tmp);*/
}
