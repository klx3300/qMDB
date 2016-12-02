#include "qSocket.hpp"
#include <cstdio>
#include <string>
using namespace qLibrary::qSocket;
int main(void){
    DiagramSocket serverSock(SocketDomain::IPV4,SocketProtocol::DEFAULT);
    serverSock.bind("127.0.0.1:38986");
    printf("Server Started on port 38986.\n");
    std::string a;
    try{
        while(1){
            std::string content=serverSock.receive(a,0);
            printf("%s says %s to you!\n",a.c_str(),content.c_str());
        }
    }catch(SocketException e){
        printf("Except:%s\n",e.errmsg.c_str());
    }
    serverSock.close();
    return 0;
}
