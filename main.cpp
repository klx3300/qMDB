#include <iostream>
#include <cstring>
#include <cmath>
#include <ctime>
#include "qDBClient/qDBClientInterface.hpp"
#include <pthread.h>
#include <unistd.h>
using namespace qLibrary::qDBClientInterface;
using namespace std;
#define TEST_NUM 100000
#define TEST_THREADS 13
#define EPSILON 1e-5
struct testInfo{
    double doubleInfo;
    int intInfo;
} info[TEST_NUM];
pthread_t thds[TEST_THREADS];
int rets[TEST_THREADS];

inline bool testDouble(double a,double b){
    return(abs(a-b)<EPSILON);
}
void* basictest(void* vpref) {
    size_t pref=(size_t)vpref;
    redisContext* conn;
    redisReply* rep;
    char* vrep;
    clock_t t;
    double totTime;
    conn=redisConnect("127.0.0.1",45175);
    if (conn == NULL || conn->err) {
        if (conn) {
            printf("Error: %s\n", conn->errstr);
        } else {
            printf("Can't allocate redis context\n");
        }
        pthread_exit((void*)-1);
    }

    t=clock();
    for(int i=0;i<TEST_NUM;++i){
        vrep=(char*)redisCommand(conn,"SET %dk%d %b",pref,i,&info[i],sizeof(testInfo));
        if(!vrep||vrep==(char*)REDIS_ERR){
            cout<<conn->errstr<<endl;
            pthread_exit((void*)-1);
        }
        freeReplyObject((void*)vrep);
    }
    t=clock()-t;
    totTime=((double)t)/CLOCKS_PER_SEC;
    cout<<"Client "<<pref<<"'s insertion test complete in "<<totTime<<" seconds"<<endl;

    testInfo* repInfo;
    t=clock();
    for(int i=0;i<TEST_NUM;++i){
        rep=(redisReply*)redisCommand(conn,"GET %dk%d",pref,i);
        if(!rep||rep==(redisReply*)REDIS_ERR){
            cout<<conn->errstr<<endl;
            pthread_exit((void*)-1);
        }
        if(rep->type!=REDIS_REPLY_STRING){
            cout<<"Return value has a wrong type!"<<endl;
            pthread_exit((void*)-1);
        }

        repInfo=(testInfo*)rep->str;

        if(i!=repInfo->intInfo||!testDouble(info[i].doubleInfo,repInfo->doubleInfo)){
            cout<<"Returned wrong data"<<endl;
            cout<<"Original data:"<<i<<","<<info[i].doubleInfo<<endl;
            cout<<"Returned data:"<<repInfo->intInfo<<","<<repInfo->doubleInfo<<endl;
            pthread_exit((void*)-1);
        }
        freeReplyObject(rep);
    }
    t=clock()-t;
    totTime=((double)t)/CLOCKS_PER_SEC;
    cout<<"Client "<<pref<<"'s query test complete in "<<totTime<<" seconds"<<endl;

    t=clock();
    for(int i=(TEST_NUM/2)-1;i>=0;--i){
        rep=(redisReply*)redisCommand(conn,"DEL %dk%d",pref,i*2);
        if(!rep||rep==(redisReply*)REDIS_ERR){
            cout<<conn->errstr<<endl;
            pthread_exit((void*)-1);
        }
        freeReplyObject(rep);
    }

    rep=(redisReply*)redisCommand(conn,"GET %dk495",pref);
    if(!rep) {
        cout << conn->errstr << endl;
        pthread_exit((void*)-1);
    }
    if(rep->type!=REDIS_REPLY_STRING){
        cout<<"Deleted wrong key!"<<endl;
        pthread_exit((void*)-1);
    }
    freeReplyObject(rep);

    rep=(redisReply*)redisCommand(conn,"GET %dk514",pref);
    if(!rep) {
        cout << conn->errstr << endl;
        pthread_exit((void*)-1);
    }
    if(rep->type!=REDIS_REPLY_NIL){
        cout<<"Key not deleted!"<<endl;
        pthread_exit((void*)-1);
    }
    freeReplyObject(rep);

    for(int i=(TEST_NUM/2)-1;i>=0;--i){
        rep=(redisReply*)redisCommand(conn,"DEL %dk%d",pref,i*2-1);
        if(!rep||rep==(redisReply*)REDIS_ERR){
            cout<<conn->errstr<<endl;
            pthread_exit((void*)-1);
        }
        freeReplyObject(rep);
    }
    t=clock()-t;
    totTime=((double)t)/CLOCKS_PER_SEC;
    cout<<"Client "<<pref<<"'s deletion test complete in "<<totTime<<" seconds"<<endl;

    redisFree(conn);
    return 0;
}
int main(){
    for(int i=0;i<TEST_NUM;++i){
        info[i].intInfo=i;
        info[i].doubleInfo=sin(i);
    }
    int err;
    for(size_t i=0;i<TEST_THREADS;++i){
        err=pthread_create(&thds[i],NULL,basictest,(void*)(i+1));
        if(err!=0){
            cout<<strerror(err)<<endl;
            return(-1);
        }
    }
    for(size_t i=0;i<TEST_THREADS;++i){
        pthread_join(thds[i],(void**)rets[i]);
    }
    for(size_t i=0;i<TEST_THREADS;++i){
        if(rets[i]==0){
            cout<<"Client "<<i<<" exited normally."<<endl;
        }else{
            cout<<"An error occurred in client"<<i<<"."<<endl;
        }
    }
    return(0);
}
