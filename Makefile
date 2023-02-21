all: proxy

proxy: http_proxy.cpp helper.h 
	g++ -g -I/home/zz312/ece568/project2/boost_1_81_0 -o proxy http_proxy.cpp 

.PHONY:
	clean

clean:
	rm -rf *.o proxy
