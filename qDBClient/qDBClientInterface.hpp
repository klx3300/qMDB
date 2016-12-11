#ifndef Q_QDBCLIENT_INTERFACE_H
#define Q_QDBCLIENT_INTERFACE_H

#include <cstdarg>
#include <cstdlib>
#include "sds.h"
#include <cstring>
#include <iostream>
#include <cctype>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <sys/time.h>
#include <string>
#include <sstream>
#include <cstdarg>
#include <vector>
#include <ctime>
#include "../qSocket/qSocket.hpp"

#define REDIS_ERR -1
#define REDIS_REPLY_NIL 0
#define REDIS_REPLY_STRING 1

namespace qLibrary {

namespace qDBClientInterface {

clock_t timertmp;
clock_t t_internet_r,t_internet_w,t_formatcmd,t_explaincmd;

void bsafe_add(std::string& targetstr,char* src,int size){
    for(int i=0;i<size;i++){
        targetstr+=(*(src+i));
    }
}

void appendInteger(std::string& str,int i){
	std::ostringstream os;
	os<<i;
	str+=os.str();
}

void appenduint(std::string& str,unsigned int i){
    std::ostringstream os;
    os<<i;
    str+=os.str();
};

enum redisConnectionType {
    REDIS_CONN_TCP,
    REDIS_CONN_UNIX
};

class redisReply {
    public:
    int type; /* REDIS_REPLY_* */
//    long long integer; /* The integer when type is REDIS_REPLY_INTEGER */
//    size_t len; /* Length of string */
    redisReply();
    const char* str;
    std::string fake_str; /* Used for both REDIS_REPLY_ERROR and REDIS_REPLY_STRING */
//    size_t elements; /* number of elements, for REDIS_REPLY_ARRAY */
//    redisReply **element; /* elements vector for REDIS_REPLY_ARRAY */
};

redisReply::redisReply(){
    str=NULL;
}

class redisContext {
    public:
    int err; /* Error flags, 0 when there is no error */
    const char* errstr;
    std::string fake_errstr; /* String representation of error when applicable */
    qLibrary::qSocket::StreamSocket sock;
    int flags;
    redisContext();
};

redisContext::redisContext(){
    err=0;
    errstr=NULL;
    flags=0;
    sock.info.domain=qLibrary::qSocket::SocketDomain::IPV4;
    sock.info.type=qLibrary::qSocket::SocketType::STREAM;
    sock.info.protocol=qLibrary::qSocket::SocketProtocol::DEFAULT;
    sock.open();
}
namespace qFakeHiredis{
static uint32_t countDigits(uint64_t v) {
    uint32_t result = 1;
    for (;;) {
        if (v < 10) return result;
        if (v < 100) return result + 1;
        if (v < 1000) return result + 2;
        if (v < 10000) return result + 3;
        v /= 10000U;
        result += 4;
    }
}

static size_t bulklen(size_t len) {
        return 1+countDigits(len)+2+len+2;
}



std::vector<int> redisvFormatCommand(char **target, const char *format, va_list ap) {
    const char *c = format;
    char *cmd = NULL;   /* final command */
    int pos;            /* position in final command */
    sds curarg, newarg; /* current argument */
    int touched = 0;    /* was the current argument touched? */
    char **curargv = NULL, **newargv = NULL;
    int argc = 0;
    int totlen = 0;
    int error_type = 0; /* 0 = no error; -1 = memory error; -2 = format error */
    int j;
    std::vector<int>seplen;

    /* Abort if there is not target to set */
    if (target == NULL)
        return seplen;

    /* Build the command string accordingly to protocol */
    curarg = sdsempty();
    if (curarg == NULL)
        return seplen;

    while (*c != '\0') {
        if (*c != '%' || c[1] == '\0') {
            if (*c == ' ') {
                if (touched) {
                    newargv = (char**)realloc(curargv, sizeof(char *) * (argc + 1));
                    if (newargv == NULL)
                        goto memory_err;
                    curargv = newargv;
                    curargv[argc++] = curarg;
                    totlen += bulklen(sdslen(curarg));
                    seplen.push_back(bulklen(sdslen(curarg)));

                    /* curarg is put in argv so it can be overwritten. */
                    curarg = sdsempty();
                    if (curarg == NULL)
                        goto memory_err;
                    touched = 0;
                }
            } else {
                newarg = sdscatlen(curarg, c, 1);
                if (newarg == NULL)
                    goto memory_err;
                curarg = newarg;
                touched = 1;
            }
        } else {
            char *arg;
            size_t size;

            /* Set newarg so it can be checked even if it is not touched. */
            newarg = curarg;

            switch (c[1]) {
            case 's':
                arg = va_arg(ap, char *);
                size = strlen(arg);
                if (size > 0)
                    newarg = sdscatlen(curarg, arg, size);
                break;
            case 'b':
                arg = va_arg(ap, char *);
                size = va_arg(ap, size_t);
                if (size > 0)
                    newarg = sdscatlen(curarg, arg, size);
                break;
            case '%':
                newarg = sdscat(curarg, "%");
                break;
            default:
                /* Try to detect printf format */
                {
                    static const char intfmts[] = "diouxX";
                    static const char flags[] = "#0-+ ";
                    char _format[16];
                    const char *_p = c + 1;
                    size_t _l = 0;
                    va_list _cpy;

                    /* Flags */
                    while (*_p != '\0' && strchr(flags, *_p) != NULL)
                        _p++;

                    /* Field width */
                    while (*_p != '\0' && isdigit(*_p))
                        _p++;

                    /* Precision */
                    if (*_p == '.') {
                        _p++;
                        while (*_p != '\0' && isdigit(*_p))
                            _p++;
                    }

                    /* Copy va_list before consuming with va_arg */
                    va_copy(_cpy, ap);

                    /* Integer conversion (without modifiers) */
                    if (strchr(intfmts, *_p) != NULL) {
                        va_arg(ap, int);
                        goto fmt_valid;
                    }

                    /* Double conversion (without modifiers) */
                    if (strchr("eEfFgGaA", *_p) != NULL) {
                        va_arg(ap, double);
                        goto fmt_valid;
                    }

                    /* Size: char */
                    if (_p[0] == 'h' && _p[1] == 'h') {
                        _p += 2;
                        if (*_p != '\0' && strchr(intfmts, *_p) != NULL) {
                            va_arg(ap, int); /* char gets promoted to int */
                            goto fmt_valid;
                        }
                        goto fmt_invalid;
                    }

                    /* Size: short */
                    if (_p[0] == 'h') {
                        _p += 1;
                        if (*_p != '\0' && strchr(intfmts, *_p) != NULL) {
                            va_arg(ap, int); /* short gets promoted to int */
                            goto fmt_valid;
                        }
                        goto fmt_invalid;
                    }

                    /* Size: long long */
                    if (_p[0] == 'l' && _p[1] == 'l') {
                        _p += 2;
                        if (*_p != '\0' && strchr(intfmts, *_p) != NULL) {
                            va_arg(ap, long long);
                            goto fmt_valid;
                        }
                        goto fmt_invalid;
                    }

                    /* Size: long */
                    if (_p[0] == 'l') {
                        _p += 1;
                        if (*_p != '\0' && strchr(intfmts, *_p) != NULL) {
                            va_arg(ap, long);
                            goto fmt_valid;
                        }
                        goto fmt_invalid;
                    }

                fmt_invalid:
                    va_end(_cpy);
                    goto format_err;

                fmt_valid:
                    _l = (_p + 1) - c;
                    if (_l < sizeof(_format) - 2) {
                        memcpy(_format, c, _l);
                        _format[_l] = '\0';
                        newarg = sdscatvprintf(curarg, _format, _cpy);

                        /* Update current position (note: outer blocks
                         * increment c twice so compensate here) */
                        c = _p - 1;
                    }

                    va_end(_cpy);
                    break;
                }
            }

            if (newarg == NULL)
                goto memory_err;
            curarg = newarg;

            touched = 1;
            c++;
        }
        c++;
    }

    /* Add the last argument if needed */
    if (touched) {
        newargv = (char**)realloc(curargv, sizeof(char *) * (argc + 1));
        if (newargv == NULL)
            goto memory_err;
        curargv = newargv;
        curargv[argc++] = curarg;
        totlen += bulklen(sdslen(curarg));
        seplen.push_back(bulklen(sdslen(curarg)));
    } else {
        sdsfree(curarg);
    }

    /* Clear curarg because it was put in curargv or was free'd. */
    curarg = NULL;

    /* Add bytes needed to hold multi bulk count */
    totlen += 1 + countDigits(argc) + 2;

    /* Build the command at protocol level */
    cmd = (char*)malloc(totlen + 1);
    if (cmd == NULL)
        goto memory_err;

    pos = sprintf(cmd, "*%d\r\n", argc);
    for (j = 0; j < argc; j++) {
        pos += sprintf(cmd + pos, "$%zu\r\n", sdslen(curargv[j]));
        memcpy(cmd + pos, curargv[j], sdslen(curargv[j]));
        pos += sdslen(curargv[j]);
        sdsfree(curargv[j]);
        cmd[pos++] = '\r';
        cmd[pos++] = '\n';
    }
    assert(pos == totlen);
    cmd[pos] = '\0';

    free(curargv);
    *target = cmd;
    seplen.push_back(totlen);
    return seplen;

format_err:
    error_type = -2;
    goto cleanup;

memory_err:
    error_type = -1;
    goto cleanup;

cleanup:
    if (curargv) {
        while (argc--)
            sdsfree(curargv[argc]);
        free(curargv);
    }

    sdsfree(curarg);

    /* No need to check cmd since it is the last statement that can fail,
     * but do it anyway to be as defensive as possible. */
    if (cmd != NULL)
        free(cmd);

    return seplen;
}

/* Format a command according to the Redis protocol. This function
 * takes a format similar to printf:
 *
 * %s represents a C null terminated string you want to interpolate
 * %b represents a binary safe string
 *
 * When using %b you need to provide both the pointer to the string
 * and the length in bytes as a size_t. Examples:
 *
 * len = redisFormatCommand(target, "GET %s", mykey);
 * len = redisFormatCommand(target, "SET %s %b", mykey, myval, myvallen);
 */
std::vector<int> redisFormatCommand(char **target, const char *format, ...) {
    va_list ap;
    std::vector<int> len;
    va_start(ap, format);
    len = redisvFormatCommand(target, format, ap);
    va_end(ap);

    /* The API says "-1" means bad result, but we now also return "-2" in some
     * cases.  Force the return value to always be -1. */

    return len;
}
}

// qfake ends here

redisContext* redisConnect(std::string serveraddr,int port){
    redisContext* rctx=new redisContext();
    std::string addrxport=serveraddr;
    addrxport+=":";
    appendInteger(addrxport,port);
    rctx->sock.connect(addrxport);
    std::string k;
    k+=(char)42;
    rctx->sock.write(k);
    rctx->sock.sendBeat();
    return rctx;
}

void initEfficientcyCheck(){
    t_internet_r=0;
    t_formatcmd=0;
    t_explaincmd=0;
}

void reportEfficiencyCheck(){
    std::cout << "[INFO] Internet r/w " << t_internet_r << "/" << t_internet_w << " Format " << t_formatcmd << " Explain " << t_explaincmd << std::endl;
}

void* redisCommand(redisContext* rctx,std::string fstr,...){
    va_list args;
    va_start(args,fstr);
    char *tmpfmt;
    timertmp=clock();
    std::vector<int> length=qFakeHiredis::redisvFormatCommand((char**)&tmpfmt,fstr.c_str(),args);
    t_formatcmd+=clock()-timertmp;
    va_end(args);
    std::string origstr=fstr;
    std::string fedstr;
    timertmp=clock();
    bsafe_add(fedstr,tmpfmt,length.back());
    length.pop_back();
    char instruct=fstr[0];
    // substract params
    fedstr=fedstr.substr(fedstr.find_first_of("\r\n")+2); // first line : param counts;
    fedstr=fedstr.substr(fedstr.find_first_of("\r\n")+2); // second line : first param length (command)
    fedstr=fedstr.substr(fedstr.find_first_of("\r\n")+2); // third line : command content
    redisReply* rrep=new redisReply();
    switch(instruct){
        case 'S':{
            unsigned int keylength=(unsigned int)atoi(fedstr.substr(1,fedstr.find_first_of("\r\n")-1).c_str());
            fedstr=fedstr.substr(fedstr.find_first_of("\r\n")+2);
            std::string key=fedstr.substr(0,keylength);
            fedstr=fedstr.substr(keylength+2);
            unsigned int valuelength=(unsigned int)atoi(fedstr.substr(1,fedstr.find_first_of("\r\n")-1).c_str());
            fedstr=fedstr.substr(fedstr.find_first_of("\r\n")+2);
            std::string value=fedstr.substr(0,valuelength);
            std::string dt;
            dt+=(char)1;
            bsafe_add(dt,(char*)&keylength,4);
            dt+=key;
            bsafe_add(dt,(char*)&valuelength,4);
            dt+=value;
            t_explaincmd+=clock()-timertmp;
            timertmp=clock();
            rctx->sock.write(dt);
            rctx->sock.sendBeat();
            t_internet_w+=clock()-timertmp;timertmp=clock();
            break;}
        case 'G':{
            unsigned int keylength=(unsigned int)atoi(fedstr.substr(1,fedstr.find_first_of("\r\n")-1).c_str());
            fedstr=fedstr.substr(fedstr.find_first_of("\r\n")+2);
            std::string key=fedstr.substr(0,keylength);
            std::string dt;
            dt+=(char)4;
            bsafe_add(dt,(char*)&keylength,4);
            dt+=key;
            t_explaincmd+=clock()-timertmp;
            timertmp=clock();
            rctx->sock.write(dt);
            rctx->sock.sendBeat();
            t_internet_w+=clock()-timertmp;timertmp=clock();
            break;}
        case 'D':{
            unsigned int keylength=(unsigned int)atoi(fedstr.substr(1,fedstr.find_first_of("\r\n")-1).c_str());
            fedstr=fedstr.substr(fedstr.find_first_of("\r\n")+2);
            std::string key=fedstr.substr(0,keylength);
            std::string dt;
            dt+=(char)2;
            bsafe_add(dt,(char*)&keylength,4);
            dt+=key;
            t_explaincmd+=clock()-timertmp;
            timertmp=clock();
            rctx->sock.write(dt);
            rctx->sock.sendBeat();
            t_internet_w+=clock()-timertmp;timertmp=clock();
            break;}
        case 'E':{
            unsigned int keylength=(unsigned int)atoi(fedstr.substr(1,fedstr.find_first_of("\r\n")-1).c_str());
            fedstr=fedstr.substr(fedstr.find_first_of("\r\n")+2);
            std::string key=fedstr.substr(0,keylength);
            std::string dt;
            dt+=(char)3;
            bsafe_add(dt,(char*)&keylength,4);
            dt+=key;
            t_explaincmd+=clock()-timertmp;
            timertmp=clock();
            rctx->sock.write(dt);
            rctx->sock.sendBeat();
            t_internet_w+=clock()-timertmp;timertmp=clock();
            break;}
        default:break;
    }

    // ooh-oh
    // it comes to receiving results!
    unsigned char statusno=rctx->sock.readchar();
    /*if(statusno!=80){
        rctx->fake_errstr+=statusno;
        printf("%u\n",statusno);
        rctx->errstr=rctx->fake_errstr.c_str();
        return NULL;
    }*/
    switch(statusno){
        case 80:{
            std::string retrr=rctx->sock.read(4);
            if(retrr!="FUCK"){
                unsigned int retinst=0;
                memcpy(&retinst,retrr.c_str(),4);
                rrep->fake_str=rctx->sock.read(retinst);
                rrep->str=rrep->fake_str.c_str();
                rrep->type=REDIS_REPLY_STRING;
                t_internet_r+=clock()-timertmp;
                rctx->sock.acceptBeat();
                return rrep;
            }else{
                t_internet_r+=clock()-timertmp;
                return rrep;
            }
            break;}
        case 198:{
                rrep->type=REDIS_REPLY_NIL;
                t_internet_r+=clock()-timertmp;
                rctx->sock.acceptBeat();
                return rrep;
            break;}
        default:{
            rctx->fake_errstr+=statusno;
            printf("%u\n",statusno);
            rctx->errstr=rctx->fake_errstr.c_str();
            t_internet_r+=clock()-timertmp;
            rctx->sock.acceptBeat();
            return NULL;
            break;}
    }
    

}

void redisFree(redisContext *rctx){
    rctx->sock.close();
    delete rctx;
}

void freeReplyObject(void *rrep){
    if((long)rrep!=-1){
        if(rrep!=NULL){
            delete (redisReply*)rrep;
        }
    }
}

}
}

#endif
