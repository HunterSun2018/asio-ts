all : demo client

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

clean : 
	rm -f demo client