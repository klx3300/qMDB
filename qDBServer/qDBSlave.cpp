/* ====================================================
 * qMDB Slave database
 * the master database do NOT store any data.
 * instead, the slave database stores all data.
 * the master database will have a scheduler to select which
 * insertion request will be sent to the slave database,
 * and the master database still have its ways to find
 * which server the data are stored in.
 * ===================================================
 */
// NOTICE : CMDL PARAM ARE MASTER ADDRESS!

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

using namespace qLibrary;

void bsafe_add(std::string& targetstr,char* src,int size){
    for(int i=0;i<size;i++){
        targetstr+=(*(src+i));
    }
}

std::unordered_map<std::string,std::string> data;

int operationSet(std::string key,std::string value);
int operationDelete(std::string key);
int operationGet(qSocket::StreamSocket &dtpipe,std::string key);

int main(int argc,char** argv){
    if(argc==1)
        return 0;
    std::string masteraddr(argv[1]),tmp;
    std::cout << "[INFO] Connecting to master : " << masteraddr << std::endl;
    qSocket::StreamSocket dtpipe(qSocket::SocketDomain::IPV4,qSocket::SocketProtocol::DEFAULT);
    dtpipe.connect(masteraddr);
    tmp+=(char)41;
    dtpipe.write(tmp);
    std::cout << "[INFO] Connection with master " << masteraddr << " established." << std::endl;
    while(1){
        // accept instructions
        char instruct=dtpipe.readchar();
        switch(instruct){
            case 1:{
                unsigned int keylength=dtpipe.readuint();
                std::string key=dtpipe.read(keylength);
                unsigned int valuelength=dtpipe.readuint();
                std::string value=dtpipe.read(valuelength);
                operationSet(key,value);
                break;}
            case 2:{
                unsigned int keylength=dtpipe.readuint();
                std::string key=dtpipe.read(keylength);
                operationDelete(key);
                break;}
            case 4:{
                unsigned int keylength=dtpipe.readuint();
                std::string key=dtpipe.read(keylength);
                operationGet(dtpipe,key);
                break;}
            default:
                break;
        }
    }
}

int operationSet(std::string key,std::string value){
    data[key]=value;
}

int operationDelete(std::string key){
    data.erase(key);
}

int operationGet(qSocket::StreamSocket &dtpipe,std::string key){
    if(data.find(key)!=data.end()){
        std::string valuev=data[key];
        char dt[5+valuev.length()];
        unsigned int vtmp=valuev.length();
        memset(dt,0,5+valuev.length());
        dt[0]=(char)80;
        memcpy(dt+1,&vtmp,4);
        memcpy(dt+5,valuev.c_str(),valuev.length());
        std::string tmp;
        bsafe_add(tmp,dt,5+valuev.length());
        dtpipe.write(tmp);
    }else{
        std::string tmp;
        tmp+=(char)198;
        dtpipe.write(tmp);
    }
}

