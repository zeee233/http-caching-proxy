all: proxy

proxy: http_proxy.cpp helper.h 
	g++ -pthread -g -o proxy http_proxy.cpp -lboost_regex -lboost_system -lboost_filesystem

.PHONY:
	clean

clean:
	rm -rf *.o proxy
