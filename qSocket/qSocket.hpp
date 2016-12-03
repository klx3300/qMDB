#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>


#ifndef Q_SOCKET_H
#define Q_SOCKET_H

namespace qLibrary{
    in_addr str_to_ipv4addr(std::string addrxport){
        // slice the string
        std::string addr;
        if(addrxport.find(':')!=std::string::npos){
            addr=addrxport.substr(0,addrxport.find(':'));
        }else{
            addr=addrxport;
        }
        struct sockaddr_in stru;
        memset(&stru,0,sizeof(struct sockaddr_in));
        inet_pton(AF_INET,addr.c_str(),&stru);
        return stru.sin_addr;
    }
    namespace qSocket{
        const int MAX_DIAGRAM_LENGTH=8192;
        class SocketAddress{

        };
        namespace SocketType{
                const int STREAM=SOCK_STREAM;
                const int DIAGRAM=SOCK_DGRAM;
        };
        namespace SocketProtocol{
                const int DEFAULT=0;
        };
        namespace SocketDomain{
                const int UNIX=AF_UNIX;
                const int IPV4=AF_INET;
                const int IPV6=AF_INET6;
        };
        class SocketException{
            public:
                SocketException(std::string errmsg){this->errmsg=errmsg;};
                std::string errmsg;
        };
        class SocketBindException : public SocketException{
            public:
                SocketBindException(std::string errmsg) : SocketException(errmsg){};
        };
        class SocketInvalidParamsException : public SocketException{
            public:
                SocketInvalidParamsException(std::string errmsg) : SocketException(errmsg){};
        };
        class SocketInitializingException : public SocketException{
            public:
                SocketInitializingException(std::string errmsg) : SocketException(errmsg){};
        };
        class DiagramSocketException : public SocketException{
            public:
                DiagramSocketException(std::string errmsg) : SocketException(errmsg){};
        };
        class DiagramSocketReceiveException : public DiagramSocketException{
            public:
                DiagramSocketReceiveException(std::string errmsg) : DiagramSocketException(errmsg){};
        };
        class DiagramSocketSendException : public DiagramSocketException{
            public:
                DiagramSocketSendException(std::string errmsg) : DiagramSocketException(errmsg){};
        };
        class StreamSocketException : public SocketException{
            public:
                StreamSocketException(std::string errmsg) : SocketException(errmsg){};
        };
        class StreamSocketConnectException : public StreamSocketException{
            public:
                StreamSocketConnectException(std::string errmsg) : StreamSocketException(errmsg){};
        };
        class StreamSocketListenException : public StreamSocketException{
            public:
                StreamSocketListenException(std::string errmsg) : StreamSocketException(errmsg){};
        };
        class StreamSocketAcceptException : public StreamSocketException{
            public:
                StreamSocketAcceptException(std::string errmsg) : StreamSocketException(errmsg){};
        };
        class StreamSocketIOException : public StreamSocketException{
            public:
                StreamSocketIOException(std::string errmsg) : StreamSocketException(errmsg){};
        };
        class SocketInformation{
            public:
                int fileDescriptor;
                int domain;
                int type;
                int protocol;
        };
        class Socket{
            public:
                Socket();
                Socket(SocketInformation info){Socket::init(info);};
                Socket(int fileDescriptor){Socket::init(fileDescriptor);}; // Deprecated. Strongly recommend not to use this.
                Socket(int domain,int type,int protocol){Socket::init(domain,type,protocol);};
                Socket& init(int fileDescriptor);
                Socket& init(SocketInformation info);// Deprecated. Strongly recommend not to use this.
                Socket& init(int domain,int type,int protocol);
                Socket& bind(std::string address);
                void close(void){::shutdown(info.fileDescriptor,2);};
                SocketInformation info;
            private:
        };
        class StreamSocket : public Socket{
            public:
                StreamSocket():Socket(){};
                StreamSocket(SocketInformation info):Socket(info){};
                StreamSocket(int fileDescriptor):Socket(fileDescriptor){};
                StreamSocket(int domain,int protocol):Socket(domain,SocketType::STREAM,protocol){};
                StreamSocket& connect(std::string address);
                StreamSocket& listen(void);
                StreamSocket& listen(int unsettled_connections_limitation);
                SocketInformation accept(std::string &srcaddr);
                StreamSocket& write(std::string content);
                std::string read();
                std::string nonblockRead();
        };
        class DiagramSocket : public Socket{
            public:
                DiagramSocket():Socket(){};
                DiagramSocket(SocketInformation info):Socket(info){};
                DiagramSocket(int fileDescriptor):Socket(fileDescriptor){};
                DiagramSocket(int domain,int protocol):Socket(domain,SocketType::DIAGRAM,protocol){};
                std::string receive(std::string& who,int flags);
                Socket& send(std::string dest,std::string content,int flags);
        };

        Socket& Socket::init(int fileDescriptor){
            this->info.fileDescriptor=fileDescriptor;
            return *this;
        }

        Socket& Socket::init(SocketInformation info){
            this->info=info;
            return *this;
        }

        Socket& Socket::init(int domain,int type,int protocol){
            info.domain=domain;
            info.type=type;
            info.protocol=protocol;
            info.fileDescriptor=socket(info.domain,info.type,info.protocol);
            if(info.fileDescriptor==-1){
                SocketInitializingException e("Func call socket() returns -1");
                throw e;
            }
            return *this;
        }

        Socket& Socket::bind(std::string address){
            if(info.domain==AF_INET){
                struct sockaddr_in addr;
                memset(&addr,0,sizeof(struct sockaddr_in));
                addr.sin_family=AF_INET;
                int separator_pos=address.find(':');
                if(separator_pos==std::string::npos){
                    SocketInvalidParamsException e("Given Address is not a valid ipv4 address!");
                    throw e;
                }
                addr.sin_addr.s_addr=htonl(INADDR_ANY);
                addr.sin_port=htons(atoi(address.substr(separator_pos+1,std::string::npos).c_str()));
                // bind
                if(::bind(info.fileDescriptor,(struct sockaddr*)&addr,sizeof(struct sockaddr_in))==-1){
                    SocketBindException e("Func call bind() returns -1");
                    throw e;
                }

            }else{
                SocketInvalidParamsException e("Invalid domain name in given params.");
                throw e;
            }
            return *this;
        }

        // DiagramSocket
        std::string DiagramSocket::receive(std::string& who,int flags){
            // check domain
            if(info.domain==AF_INET){
                char* buffer=(char*)malloc(sizeof(char)*MAX_DIAGRAM_LENGTH);
                struct sockaddr_in srcinfo;
                unsigned int fck=sizeof(struct sockaddr_in);
                memset(&srcinfo,0,sizeof(struct sockaddr_in));
                int dtlen=::recvfrom(info.fileDescriptor,buffer,MAX_DIAGRAM_LENGTH,flags,(struct sockaddr*)&srcinfo,&fck);
                if(dtlen==-1){
                    DiagramSocketReceiveException e("Func call recvfrom() returned -1.");
                    throw e;
                } 
                if(dtlen==0){
                    return "";
                }else{
                    // encode the result
                    char addrbuffer[64];
                    who=inet_ntoa(srcinfo.sin_addr);
                    // clear addrbuffer
                    memset(addrbuffer,0,64);
                    sprintf(addrbuffer,"%d",ntohs(srcinfo.sin_port));
                    who=who+":"+addrbuffer;
                    buffer[dtlen]='\0';
                    std::string retdt(buffer);
                    return retdt;
                }
            }else{
                SocketInvalidParamsException e("Invalid domain value.");
                throw e;
                return "";
            }
        }

        Socket& DiagramSocket::send(std::string dest,std::string content,int flags){
            // check domain
            if(info.domain==AF_INET){
                struct sockaddr_in addr;
                memset(&addr,0,sizeof(struct sockaddr_in));
                addr.sin_family=AF_INET;
                int separator_pos=dest.find(':');
                if(separator_pos==std::string::npos){
                    SocketInvalidParamsException e("Given Address is not a valid ipv4 address!");
                    throw e;
                }
                addr.sin_port=htons(atoi(dest.substr(separator_pos+1,std::string::npos).c_str()));
                addr.sin_addr=qLibrary::str_to_ipv4addr(dest);
                int sentlen=::sendto(info.fileDescriptor,content.c_str(),content.length(),flags,(struct sockaddr*)&addr,sizeof(struct sockaddr_in));
                if(sentlen<content.length()){
                    DiagramSocketSendException e("Send FAIL.");
                    throw e;
                }
                return *this;
            }else{
                SocketInvalidParamsException e("Invalid domain value.");
                throw e;
                return *this;
            }
        }
        // StreamSocket
        StreamSocket& StreamSocket::connect(std::string address){
            if(info.domain==AF_INET){
                struct sockaddr_in addr;
                memset(&addr,0,sizeof(struct sockaddr_in));
                addr.sin_family=AF_INET;
                int separator_pos=address.find(':');
                if(separator_pos==std::string::npos){
                    SocketInvalidParamsException e("Given Address is not a valid ipv4 address!");
                    throw e;
                }
                addr.sin_port=htons(atoi(address.substr(separator_pos+1,std::string::npos).c_str()));
                addr.sin_addr=qLibrary::str_to_ipv4addr(address);
                if(::connect(info.fileDescriptor,(struct sockaddr*)&addr,sizeof(struct sockaddr_in))==-1){
                    StreamSocketConnectException e("Func call connect() returned -1.");
                    throw e;
                }
            }else{
                SocketInvalidParamsException e("Invalid domain name in given params.");
                throw e;
            }
            return *this;
        }
        StreamSocket& StreamSocket::listen(void){
            return StreamSocket::listen(128);
        }
        StreamSocket& StreamSocket::listen(int limitation){
            int retflg=::listen(info.fileDescriptor,limitation);
            if(retflg==-1){
                StreamSocketListenException e("Func call listen() returns -1");
                throw e;
            }
            return *this;
        }
        SocketInformation StreamSocket::accept(std::string &who){
            SocketInformation nsinfo;
            nsinfo.domain=info.domain;
            nsinfo.protocol=info.protocol;
            nsinfo.type=info.type;
            if(info.domain==AF_INET){
                struct sockaddr_in srcaddr;
                socklen_t srcaddr_len=sizeof(struct sockaddr_in);
                nsinfo.fileDescriptor=::accept(info.fileDescriptor,(struct sockaddr*)&srcaddr,&srcaddr_len);
                if(nsinfo.fileDescriptor==-1){
                    StreamSocketAcceptException e("Func call accept() returns -1.");
                    throw e;
                }
                char addrbuffer[64];
                who=inet_ntoa(srcaddr.sin_addr);
                // clear addrbuffer
                memset(addrbuffer,0,64);
                sprintf(addrbuffer,"%d",ntohs(srcaddr.sin_port));
                who=who+":"+addrbuffer;
                return nsinfo;
            }else{
                SocketInvalidParamsException e("Not valid Domain.");
                throw e;
            }
        }
        std::string StreamSocket::read(){
            const int MAX_BUFFER_SIZE=8192;
            char buffer[MAX_BUFFER_SIZE+1];
            memset(buffer,0,MAX_BUFFER_SIZE);
            std::string content("");
            int rlength=::read(info.fileDescriptor,buffer,MAX_BUFFER_SIZE);
            buffer[rlength]='\0';
            content+=buffer;
            return content;
        }
        std::string StreamSocket::nonblockRead(){
            const int MAX_BUFFER_SIZE=8192;
            char buffer[MAX_BUFFER_SIZE+1];
            memset(buffer,0,MAX_BUFFER_SIZE);
            std::string content("");
            int rlength=0;
            do{
                rlength=::recv(info.fileDescriptor,buffer,MAX_BUFFER_SIZE,MSG_DONTWAIT);
                buffer[rlength]='\0';
                content+=buffer;
            }while(rlength==0);
            return content;
        }
        StreamSocket& StreamSocket::write(std::string content){
            int written=::write(info.fileDescriptor,content.c_str(),content.length());
            if(written==-1){
                StreamSocketIOException e("Func call write() returned -1.");
                throw e;
            }
            return *this;
        }
    }
}

#endif
