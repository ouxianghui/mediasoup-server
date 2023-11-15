/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Leia-SFU
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-11-1
*************************************************************************/

#include <sys/unistd.h>
#include <sys/wait.h>
#include <iostream>
#include <atomic>
#include "engine.h"
#include "config.h"
#include "controller/statistics_controller.hpp"
#include "controller/rooms_controller.hpp"
#include "app_component.hpp"
#include "oatpp/network/Server.hpp"
#include "oatpp/web/server/HttpRouter.hpp"

int32_t writePidFile(const std::string& pidFile)
{
    // -rw-r--r--
    // 644
    int mode = S_IRUSR | S_IWUSR |  S_IRGRP | S_IROTH;

    int fd;
    // open pid file
    if ((fd = ::open(pidFile.c_str(), O_WRONLY | O_CREAT, mode)) == -1) {
        SRV_LOGE("open pid file: %s", pidFile.c_str());
        return -1;
    }

    // require write lock
    struct flock lock;

    lock.l_type = F_WRLCK; // F_RDLCK, F_WRLCK, F_UNLCK
    lock.l_start = 0; // type offset, relative to l_whence
    lock.l_whence = SEEK_SET;  // SEEK_SET, SEEK_CUR, SEEK_END
    lock.l_len = 0;

    if (fcntl(fd, F_SETLK, &lock) == -1) {
        if (errno == EACCES || errno == EAGAIN) {
            ::close(fd);
            SRV_LOGE("sfu is already running");
            return -1;
        }
        SRV_LOGE("access to pid file: %s", pidFile.c_str());
        return -1;
    }

    // truncate file
    if (ftruncate(fd, 0) != 0) {
        SRV_LOGE("truncate pid file: %s", pidFile.c_str());
        return -1;
    }

    // write the pid
    std::string pid = std::to_string(getpid());
    if (write(fd, pid.c_str(), pid.length()) != (int)pid.length()) {
        SRV_LOGE("write pid: %s to file: %s", pid.c_str(), pidFile.c_str());
        return -1;
    }

    // auto close when fork child process.
    int val;
    if ((val = fcntl(fd, F_GETFD, 0)) < 0) {
        SRV_LOGE("fcntl fd: %d", fd);
        return -1;
    }
    val |= FD_CLOEXEC;
    if (fcntl(fd, F_SETFD, val) < 0) {
        SRV_LOGE("lock file: %s fd: %d", pidFile.c_str(), fd);
        return -1;
    }

    SRV_LOGD("write pid: %s to %s success!", pid.c_str(), pidFile.c_str());

    return 0;
}

void run(const oatpp::base::CommandLineArguments& cmdArgs)
{
    /* Register Components in scope of run() method */
    AppComponent components(cmdArgs);

    /* Get router component */
    OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router);

    /* Create RoomsController and add all of its endpoints to router */
    auto rc = std::make_shared<RoomsController>();
    rc->addEndpointsToRouter(router);

    auto sc = std::make_shared<StatisticsController>();
    sc->addEndpointsToRouter(router);

    /* Get connection handler component */
    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, connectionHandler, "http");

    /* Get connection provider component */
    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, connectionProvider);
    
    /* Create server which takes provided TCP connections and passes them to HTTP connection handler */
    oatpp::network::Server server(connectionProvider, connectionHandler);

    std::thread serverThread([&server]{
        server.run();
    });

    // std::thread pingThread([]{
    //     OATPP_COMPONENT(std::shared_ptr<Lobby>, lobby);
    //     lobby->runPingLoop(std::chrono::seconds(30));
    // });

    std::thread statThread([]{
        OATPP_COMPONENT(std::shared_ptr<Statistics>, statistics);
        statistics->runStatLoop();
    });

    OATPP_COMPONENT(oatpp::Object<ConfigDto>, appConfig);

    if (appConfig->useTLS) {
        OATPP_LOGI("canchat", "clients are expected to connect at https://%s:%d/", appConfig->host->c_str(), *appConfig->port);
    } else {
        OATPP_LOGI("canchat", "clients are expected to connect at http://%s:%d/", appConfig->host->c_str(), *appConfig->port);
    }

    OATPP_LOGI("canchat", "canonical base URL=%s", appConfig->getCanonicalBaseUrl()->c_str());
    OATPP_LOGI("canchat", "statistics URL=%s", appConfig->getStatsUrl()->c_str());

    std::thread workerThread([]{
        MSEngine->run();
    });
    
    serverThread.join();
    //pingThread.join();
    statThread.join();
    workerThread.join();
}

int main(int argc, const char * argv[])
{
    bool runAsDeamon = false;
    
    oatpp::base::CommandLineArguments cmdArgs(argc, argv);
    
    if (cmdArgs.hasArgument("--deamon")) {
        SRV_LOGE("run as deamon");
        runAsDeamon = true;
    }
    
    if (runAsDeamon) {
        int32_t pid = fork();
        
        if(pid < 0) {
            SRV_LOGE("fork father process");
            return -1;
        }
        
        // grandpa
        if(pid > 0) {
            int status = 0;
            waitpid(pid, &status, 0);
            SRV_LOGD("grandpa process exit.");
            exit(0);
        }
        
        // father
        pid = fork();
        
        if(pid < 0) {
            SRV_LOGE("fork child process");
            return -1;
        }
        
        if(pid > 0) {
            SRV_LOGD("father process exit");
            exit(0);
        }
        
        // son
        SRV_LOGD("son(daemon) process running.");
        
        writePidFile("/usr/local/sfu/bin/sfu.pid");
    }
    
    //MSEngine->init("/home/ubuntu/dev/mediasoup-server/install/conf/config.json");
    
    //MSEngine->init("/Users/jackie.ou/Desktop/Research/dev/mediasoup-server/install/conf/config.json");
    
    if (!cmdArgs.hasArgument("--conf")) {
        SRV_LOGE("configuration file must be provided.\neg: ./sfu --conf path/config.json");
        return -1;
    }
    std::string configFile = cmdArgs.getNamedArgumentValue("--conf", "");
    
    MSEngine->init(configFile);
    
    oatpp::base::Environment::init();

    run(cmdArgs);
    
    MSEngine->destroy();

    oatpp::base::Environment::destroy();

    return 0;
}
