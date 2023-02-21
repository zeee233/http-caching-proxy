all: proxy

proxy: http_proxy.cpp helper.h 
	g++ -g -o proxy http_proxy.cpp 

.PHONY:
	clean

clean:
	rm -rf *.o proxy
