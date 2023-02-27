all: proxy

proxy: http_proxy.cpp helper.h 
	g++ -pthread -g -o proxy http_proxy.cpp 

.PHONY:
	clean

clean:
	rm -rf *.o proxy
