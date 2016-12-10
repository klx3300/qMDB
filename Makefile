
main : master slave testclient
	@echo Compliation All over

master : qSocket/qSocket.hpp qIOStructure/qSelector.hpp qDBServer/qDBMaster.cpp
	@echo Compiling Master Server
	@g++ qDBServer/qDBMaster.cpp -o master

slave : qSocket/qSocket.hpp qIOStructure/qSelector.hpp qDBServer/qDBSlave.cpp
	@echo Compiling Slave Server
	@g++ qDBServer/qDBSlave.cpp -o slave

testclient : qSocket/qSocket.hpp qIOStructure/qSelector.hpp qDBClient/qDBClientInterface.hpp qDBClient/sds.c qDBClient/sds.h qDBClient/sdsalloc.h
	@echo Compiling Test Client
	@g++ main.cpp qDBClient/sds.c -o testprog

clean :
	@echo Removing all executable files
	@rm master slave testprog
