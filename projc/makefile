# Author: Rajan Jana
# Email: rjana@usc.edu
# CSCI 551
# Project B

all: compile

compile: start.o

compiletest0: test.o
compiletest1: test1.o
compiletest2: test2.o
compiletest3: test3.o
compiletest4: test4.o
compiletest5: test5.o
compiletest6: test6.o
compiletest7: test7.o
compiletest8: test8.o

TIMERSC_LIB_OBJS = timers.o timers-c.o tools.o

test.o: test.o
		g++ -g -Wno-write-strings -o filetest Test/FileUtilTest.cc \
		utility/utility.h utility/utility.cc \
		utility/fileUtil.h utility/fileUtil.cc
		chmod 777 filetest

test1.o: test1.o
		g++ -g -Wno-write-strings -o proxytest Test/ProxyTest.cc \
		Proxy/proxy.h Proxy/proxy.cc \
		utility/utility.h utility/utility.cc \
		utility/socket.h utility/socket.cc
		chmod 777 proxytest

test2.o: test2.o
		g++ -g -Wno-write-strings -o ifAddrtest Test/InterfaceAddrTest.cc \
		utility/utility.h utility/utility.cc \
		utility/networkUtil.h utility/networkUtil.cc
		chmod 777 ifAddrtest
		
test3.o: test3.o
		g++ -g -Wno-write-strings -o cMsg Test/ControlMessageTest.cc \
		control/controlMessage.h control/controlMessage.cc \
		utility/networkUtil.h utility/networkUtil.cc
		chmod 777 cMsg	
			
test4.o: test4.o
		g++ -g -Wno-write-strings -o circuitData Test/CircuitDataTest.cc \
		data/circuitData.h data/circuitData.cc
		chmod 777 circuitData	

test5.o: test5.o
		g++ -g -Wno-write-strings -o circuitMap Test/CircuitMapTest.cc \
		data/circuitMap.h data/circuitMap.cc
		chmod 777 circuitMap

test6.o: test6.o
		#gcc -o encrypt.o encrypt/encrypt.c -g -c 
		g++ -Wall -g -Wno-write-strings -o keyHandler Test/KeyHandlerTest.cc \
		encrypt/keyHandler.h encrypt/keyHandler.cc \
		encrypt/encrypt.h encrypt/encrypt.c \
		-lcrypto
		chmod 777 keyHandler

test7.o: test7.o
		#gcc -o encrypt.o encrypt/encrypt.c -g -c 
		g++ -Wall -g -Wno-write-strings -o oniontest Test/OnionEncryptTest.cc \
		encrypt/keyHandler.h encrypt/keyHandler.cc \
		encrypt/encrypt.h encrypt/encrypt.c \
		data/routerData.h data/routerData.cc \
		utility/networkUtil.h utility/networkUtil.cc \
		-lcrypto
		chmod 777 oniontest
		
test8.o: test8.o
		g++ -g -Wno-write-strings -o flowCache Test/FlowCacheTest.cc \
		data/flowCache.h data/flowCache.cc
		chmod 777 flowCache
		


start.o: $(TIMERSC_LIB_OBJS) start.o
		g++ -g -Wall -Wno-write-strings -o projc $(TIMERSC_LIB_OBJS) main/start.cc \
		main/proxy.cc main/router.cc main/tunnel.cc \
		utility/fileUtil.h utility/fileUtil.cc \
		utility/utility.h utility/utility.cc \
		utility/socket.h utility/socket.cc \
		utility/networkUtil.h utility/networkUtil.cc \
		data/routerData.h data/routerData.cc \
		data/circuitData.h data/circuitData.cc \
		data/circuitMap.h data/circuitMap.cc \
		data/flowCache.h data/flowCache.cc \
		control/controlMessage.h control/controlMessage.cc \
		encrypt/keyHandler.h encrypt/keyHandler.cc \
		encrypt/encrypt.h encrypt/encrypt.c \
		-lcrypto -lpthread -lrt
		chmod 777 projc
		
tools.o: timer/tools.cc timer/tools.hh
	g++ -Wall -g -c timer/tools.cc

timers.o: timer/timers.cc timer/timers.hh
	g++ -Wall -g -c timer/timers.cc

timers-c.o: timer/timers-c.cc timer/timers-c.h
	g++ -Wall -g -c timer/timers-c.cc

clean:
	rm -rf *.o
