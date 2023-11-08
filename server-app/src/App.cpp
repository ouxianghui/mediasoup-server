/***************************************************************************
 *
 * Project:   ______                ______ _
 *           / _____)              / _____) |          _
 *          | /      ____ ____ ___| /     | | _   ____| |_
 *          | |     / _  |  _ (___) |     | || \ / _  |  _)
 *          | \____( ( | | | | |  | \_____| | | ( ( | | |__
 *           \______)_||_|_| |_|   \______)_| |_|\_||_|\___)
 *
 *
 * Copyright 2020-present, Leonid Stryzhevskyi <lganzzzo@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************************/

#include <iostream>
#include <atomic>
#include "engine.h"
#include "config.h"
#include "controller/StatisticsController.hpp"
#include "controller/RoomsController.hpp"
#include "AppComponent.hpp"
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
    MSEngine->init("/home/ubuntu/dev/mediasoup-server/server/config.json");
    //MSEngine->init("/Users/jackie.ou/Desktop/Research/dev/mediasoup-server/server/config.json");
    
    oatpp::base::Environment::init();

    run(oatpp::base::CommandLineArguments(argc, argv));
    
    MSEngine->destroy();

    oatpp::base::Environment::destroy();

    return 0;
}
