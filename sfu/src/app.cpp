/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Leia-SFU
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-11-1
*************************************************************************/

#include <iostream>
#include <atomic>
#include "engine.h"
#include "config.h"
#include "controller/statistics_controller.hpp"
#include "controller/rooms_controller.hpp"
#include "app_component.hpp"
#include "oatpp/network/Server.hpp"
#include "oatpp/web/server/HttpRouter.hpp"

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
    oatpp::base::CommandLineArguments cmdArgs(argc, argv);
    
    if (!cmdArgs.hasArgument("--conf")) {
        SRV_LOGE("Configuration file must be provided.\neg: ./sfu --conf path-to-file/config.json");
        return -1;
    }
    std::string configFile = cmdArgs.getNamedArgumentValue("--conf", "");
    
    //MSEngine->init("/home/ubuntu/dev/mediasoup-server/install/conf/config.json");
    
    //MSEngine->init("/Users/jackie.ou/Desktop/Research/dev/mediasoup-server/install/conf/config.json");
    
    MSEngine->init(configFile);
    
    oatpp::base::Environment::init();

    run(cmdArgs);
    
    MSEngine->destroy();

    oatpp::base::Environment::destroy();

    return 0;
}
