#include "qSocket.hpp"
#include <cstdio>
#include <string>
using namespace qLibrary::qSocket;
int main(void){
    StreamSocket serverSock(SocketDomain::IPV4,SocketProtocol::DEFAULT);
    serverSock.bind("127.0.0.1:45175");
    printf("Server Started on port 45175.\n");
    try{
        serverSock.listen();
        std::string a;
        StreamSocket curr(serverSock.accept(a));
        printf("Connection Established with %s\n",a.c_str());
        std::string content("");
        //content=curr.nonblockRead();
        //for(;content.length()==0;content=curr.read());
        //printf("%s says %s to you!\n",a.c_str(),content.c_str());
        curr.write("You're welcome!");
        curr.close();
    }catch(SocketException e){
        printf("Except:%s\n",e.errmsg.c_str());
    }
    serverSock.close();
    return 0;
}
