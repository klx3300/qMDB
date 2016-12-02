#include "qSocket.hpp"
#include <cstdio>
#include <string>
using namespace qLibrary::qSocket;
int main(void){
    DiagramSocket clientSock(SocketDomain::IPV4,SocketProtocol::DEFAULT);
    clientSock.send("127.0.0.1:38986","Hello world!",0);
    return 0;
}
