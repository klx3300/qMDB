#ifndef Q_SELECTOR_H
#define Q_SELECTOR_H

#include <sys/time.h>
#include <sys/select.h>
#include <vector>
#include "../qSocket/qSocket.hpp"

namespace qLibrary{
    
    namespace qSocket{
        namespace qSelectorFlags{
            const int READ=1;
            const int WRITE=2;
            const int EXCEPTION=4;
        }
        class qSelector{
            public:
                qSelector();
                std::vector<StreamSocket> r;
                std::vector<StreamSocket> w;
                std::vector<StreamSocket> ex;
                int wait(int FLAGS);// it's save to give anything to references you don't need to use.
                std::vector<StreamSocket> gets(int FLAG);// only give one or it will fail.
            private:
                fd_set readfds;
                fd_set writefds;
                fd_set exceptfds;
        };

        qSelector::qSelector(){
        }
        
        int qSelector::wait(int FLAGS){
            FD_ZERO(&readfds);
            FD_ZERO(&writefds);
            FD_ZERO(&exceptfds);
            int largestfd=0;
            if( FLAGS & qSelectorFlags::READ ){
                for(auto x : r){
                    if(x.info.fileDescriptor > largestfd){
                        largestfd=x.info.fileDescriptor;
                    }
                    FD_SET(x.info.fileDescriptor,&readfds);
                }
            }
            if( FLAGS & qSelectorFlags::WRITE ){
                for(auto x : w){
                    if(x.info.fileDescriptor > largestfd){
                        largestfd=x.info.fileDescriptor;
                    }
                    FD_SET(x.info.fileDescriptor,&writefds);
                }
            }
            if( FLAGS & qSelectorFlags::EXCEPTION ){
                for(auto x : ex){
                    if(x.info.fileDescriptor > largestfd){
                        largestfd=x.info.fileDescriptor;
                    }
                    FD_SET(x.info.fileDescriptor,&exceptfds);
                }
            }
            return ::select(largestfd+1,&readfds,&writefds,&exceptfds,NULL);//block till some of them ready;
        }

        std::vector<StreamSocket> qSelector::gets(int FLAG){
            std::vector<StreamSocket> rets;
            if( FLAG & qSelectorFlags::READ ){
                for(auto x : r){
                    if(FD_ISSET(x.info.fileDescriptor,&readfds)){
                        rets.push_back(x);
                    }
                }
            }else 
            if( FLAG & qSelectorFlags::WRITE ){
                for(auto x : w){
                    if(FD_ISSET(x.info.fileDescriptor,&writefds)){
                        rets.push_back(x);
                    }
                }
            }else 
            if( FLAG & qSelectorFlags::EXCEPTION ){
                for(auto x : ex){
                    if(FD_ISSET(x.info.fileDescriptor,&exceptfds)){
                        rets.push_back(x);
                    }
                }
            }else{
                throw "IM ANGRY YOU DIDNT GIVE THE CORRECT FLAG!";
            }
            return rets;
        }
    }

}

#endif
