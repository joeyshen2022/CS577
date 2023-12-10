//
//

#include <iostream>
#include <csignal>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
//#include "databox.h"
#include "redis_client.h"
#include <cpp_redis/cpp_redis>
static void signalHandler(int signum) {
    printf("handler interrupt signal (%d) received.\n", signum);
//    delete md_queue;
//    printf("%d\n",__FILE__);
//    sleep(5);
//    delete logger;
    exit(signum);
}
static void usage(void)
{
    fprintf(stderr, "\nusage:\n");
    fprintf(stderr, "./application exchangeId instId marketType\n");
    fprintf(stderr, "e.g. /application OKEX BTC-USDT-SWAP DEPTH\n");
    fprintf(stderr, "\n");
    exit(-1);
}

int main(int argc, char *argv[])
{
    if (argc != 1) {
        usage();
    }
//    ShmMDClient md_client(true);
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

//    cpp_redis::client client;
//
//    client.connect("127.0.0.1", 9379,
//                   [](const std::string &host, std::size_t port, cpp_redis::connect_state status) {
//                       if (status == cpp_redis::connect_state::dropped) {
//                           std::cout << "client disconnected from " << host << ":" << port << std::endl;
//                       }
//                   });
//    client.sync_commit();
//    client.set("hello", "42", [](cpp_redis::reply &reply) {
//        std::cout << "set hello 42: " << reply << std::endl;
//        // if (reply.is_string())
//        //   do_something_with_string(reply.as_string())
//    });
//    client.sync_commit();

    RedisMDClient r_client;
    int count = 0;

    while(1){
        count++;
        string key("foo1");
        string value = "bar_" + to_string(count);
        r_client.set(key,value);
        printf("set:%s value:%s\n",key.c_str(),value.c_str());
        sleep(1);
    }
    // init socket module for windows

    return 0;
}