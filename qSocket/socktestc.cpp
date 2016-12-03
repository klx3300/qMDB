#include "qSocket.hpp"
#include <cstdio>
#include <string>
using namespace qLibrary::qSocket;
int main(void){
    StreamSocket clientSock(SocketDomain::IPV4,SocketProtocol::DEFAULT);
    clientSock.connect("127.0.0.1:45175");
    clientSock.write("Hello world!");
    std::string ax("");
    //ax=clientSock.read();
    //printf("Server says:%s\n",ax.c_str());
    clientSock.close();
    return 0;
}
