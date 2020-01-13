all : demo client xml

demo : src/main.cpp
	clang++ \
	-std=c++2a -stdlib=libc++ \
	src/main.cpp \
	-o demo \
	-lpthread

client : src/http_client.cpp
	clang++ \
	-g -D_LIBCPP_DEBUG \
	-std=c++2a -stdlib=libc++ \
	src/http_client.cpp \
	-o client \
	-lpthread -lssl -lcrypto

xml : src/libxml_client.cpp
	clang++ -g -std=c++17 \
	src/libxml_client.cpp \
	`xml2-config --cflags --libs` `pkg-config libxml++-5.0 --cflags --libs` \
	-o xml \
	-lcurlpp -lcurl

clean : 
	rm -f demo client xml