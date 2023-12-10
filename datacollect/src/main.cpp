//
//
#include <iostream>
#include <csignal>


using namespace md;
using namespace rapidjson;

static void signal_handler(int signum) {
    printf("handler interrupt signal (%d) received.\n", signum);
    NanoLog::poll();

    sleep(0.1);//wait logger write data to disk
    exit(signum);
}

static void usage(void){
    fprintf(stderr, "\nusage:\n");
    fprintf(stderr, "./application json_config_file \n");
    fprintf(stderr, "\n");
    exit(-1);
}


int main(int argc, char *argv[]){
    if ( argc != 2 ){
        usage();
    }

    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);

    Config* config = Config::instance();
    config->load(argv[1]);

    string tag, logfile, logLevel;
    if(config->get_log_config("logfile",logfile)
       && config->get_string("tag",tag) && config->get_log_config("level",logLevel)){
        log_maintain(tag, logfile, logLevel);
    }
    else{
        fprintf(stderr,"not found log config info in %s,\nneed to add log config\n", argv[1]);
        exit(-1);
    }

    string program = tag;
    int currentPid = getpid();
    auto filePid = crypto::get_program_pid(program);
    if(!crypto::ensure_one_instance(program) && currentPid != filePid){
        string errormsg = tag + " with pid=" + std::to_string(filePid) + " already exists, aborted";//"another instance of " + program + " is running";
        LOG_ERROR("%s ", errormsg.c_str());
        sleep(1);
        cryptothrow(errormsg.c_str(), -1);
    }
    crypto::write_program_pid(program);

    string redisHost;
    int redisPort;
    string redisPasswd{""};
    bool hasRedisHost = config->get_redis_config("host",redisHost);
    bool hasRedisPort = config->get_redis_config("port",redisPort);
    config->get_redis_config("passwd", redisPasswd);
    if(!hasRedisHost || !hasRedisPort){
        cryptothrow("not found redis config info in %s, need to add redis config", -1);
        exit(-1);
    }
    vector<exchange_node> exchList;
    config->get_exchange_info_v3(exchList);
    sm::SecurityManager smc(redisHost.c_str(), redisPort, redisPasswd.c_str());
    DbwBase::instance().init(config->get_document_str());
    Feeder::instance().start(&exchList,&smc);

    while(1){
        log_maintain(tag, logfile, logLevel);
        sleep(1);
    }
    return 0;
}
