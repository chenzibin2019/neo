#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <cstdlib>
#include <iostream>
#include <fcntl.h>
#include "mb-app/cache-proxy.hpp"

const char *cache_log = "examples/configs/squid/log/cache.log";        // log for debugging info
const char *access_log = "examples/configs/squid/log/access.log";       // log for client request 
const char *cache_store_log = "examples/configs/squid/log/store.log";  // log for stored cache object changes

CacheProxy::CacheProxy(const std::shared_ptr<cpptoml::table>& node)
{
    server_pid = 0;
    if (node) {
      auto node_config = node->get_as<std::string>("config"); 
      std::string squid("squid");
      auto app = node->get_as<std::string>("app");
      if (app && !squid.compare(*app)){
          auto config = node->get_as<std::string>("config");
          proxy_config = *config;
          // modify the base configuration file
      } else {
          std::cerr << "it is not a squid node" << std::endl;
      }
    } else {
      std::cerr << "node_config is empty, please check " << std::endl;
      exit(EXIT_FAILURE);
    }
    
    char *buff = new char[500]();
    curr_dir = getcwd(buff, 500);
    std::cout << "curr dir: " << curr_dir << std::endl;
    config_path = new char[500]();
    strncpy(config_path, buff, strlen(buff));
    strcpy(config_path + strlen(config_path), "/examples/configs/squid/squid.conf");
    std::cout << config_path << std::endl;
    proxy_path = "/usr/sbin/squid";
    proxy_argv.push_back(const_cast<char*>("-N"));  // non-background squid
    proxy_argv.push_back(const_cast<char*>("-f"));  // use config file
    proxy_argv.push_back(config_path);
    proxy_argv.push_back(NULL);

    std::cout << "Cache Proxy is created.." << std::endl;
}

CacheProxy::~CacheProxy()
{
    shutnclean();
    delete config_path;
    delete curr_dir;
    std::cout << "Cache Proxy is destroyed.." << std::endl;
}

/** clean cache storage and launch server in child process**/
void CacheProxy::init()
{
    if (server_pid > 0) {
        int status = shutnclean();
        if (status) return;
    }

    pid_t pid;
    if ((pid=fork()) == 0) {
        char * argv[] = {NULL};
        char ** p_argv = &proxy_argv[0];
        int status __attribute__((unused))= execve(proxy_path, p_argv, argv);
        std::cerr << "failed with launch proxy for :" << strerror(errno) << "\n" << std::endl;
    } else if (pid == -1) {
        std::cerr << "fork failure during cache proxy init" << std::endl;
    } else {
        int status; 
        int retval = waitpid(pid, &status, WNOHANG);  // TODO check cache.log
        if (!retval) std::cout << "child process is launched for squid..." << std::endl;
    } 
}

/** soft reset,  **/
void CacheProxy::reset()
{
    // TOOD reset with finding the last access.log information and try to remove corresponding cached object by calling script
    /**
    std::string reset_base("./examples/configs/squid/reset.sh");
    std::string reset_argv("host");
    std::string reset_command = reset_base + reset_argv;
    int ret = system(reset_command.c_str());
    **/

    // cached file in corresponding directory. 
    std::cout << "Start to reset last step for squid server..." << std::endl;
}

/** shutdown running proxy process and clean the cache directory, kill all
 * pinger process **/
int CacheProxy::shutnclean() {
    if (server_pid > 0) {
        std::cout << server_pid << std::endl;
        char buf[40];
        snprintf(buf, sizeof(buf), "/bin/kill -INT -%d", server_pid);
        int status = system(buf);
        if (!status) server_pid = -1;
        else std::cerr << "Error occurred during termination of proxy" << std::endl;
        snprintf(buf, sizeof(buf), "/usr/bin/pkill -f \"pinger\"");
        status = system(buf);
    } else {
        std::cout << "There is no proxy server running" << std::endl;
    } 
    const char *argv = "./examples/configs/squid/clean.sh";
    int status = system(argv);
    if (!status) {
        std::cerr << "Error occurred during executing clean.sh" << std::endl;
    }
    return 0;
}

/** append configuration to base configuration rile **/
// void parse_node(const std::string config __attribute__((unused))) {}
void parse_node() {}

