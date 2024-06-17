/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: RTC-SFU
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-11-1
*************************************************************************/

#pragma once

#include "rooms/lobby.hpp"
#include <cstdlib>
#include <string>
#include "dto/config.hpp"
#include "utils/statistics.hpp"
#include "oatpp/Environment.hpp"
#include "oatpp-openssl/server/ConnectionProvider.hpp"
#include "oatpp/web/server/interceptor/RequestInterceptor.hpp"
#include "oatpp/web/server/AsyncHttpConnectionHandler.hpp"
#include "oatpp/web/server/HttpRouter.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"
#include "oatpp/json/ObjectMapper.hpp"
#include "oatpp/json/Serializer.hpp"
#include "oatpp/macro/component.hpp"
#include "oatpp/base/CommandLineArguments.hpp"
#include "oatpp/utils/Conversion.hpp"
#include "config.h"

/**
 *  Class which creates and holds Application components and registers components in oatpp::Environment
 *  Order of components initialization is from top to bottom
 */
class AppComponent
{
private:
    class RedirectInterceptor : public oatpp::web::server::interceptor::RequestInterceptor
    {
    private:
        OATPP_COMPONENT(oatpp::Object<ConfigDto>, appConfig);
        
    public:
        std::shared_ptr<OutgoingResponse> intercept(const std::shared_ptr<IncomingRequest>& request) override {
            auto host = request->getHeader(oatpp::web::protocol::http::Header::HOST);
            auto siteHost = appConfig->getHostString();
            if (!host || host != siteHost) {
                auto response = OutgoingResponse::createShared(oatpp::web::protocol::http::Status::CODE_301, nullptr);
                response->putHeader("Location", appConfig->getCanonicalBaseUrl() + request->getStartingLine().path.toString());
                return response;
            }
            return nullptr;
        }
    };

private:
    oatpp::base::CommandLineArguments _cmdArgs;

public:
    AppComponent(const oatpp::base::CommandLineArguments& cmdArgs)
    : _cmdArgs(cmdArgs) {}
    
public:
    /**
     * Create config component
     */
    OATPP_CREATE_COMPONENT(oatpp::Object<ConfigDto>, appConfig)([this] {
        auto params = MSConfig->params();
        assert(params);
        const std::string host = params->domain.empty() ? params->https.listenIp : params->domain;
        const std::string port = std::to_string(params->https.listenPort);
        const std::string CERT_PEM_PATH = params->https.tls.key;
        const std::string CERT_CRT_PATH = params->https.tls.cert;
        
        auto config = ConfigDto::createShared();

        config->host = std::getenv("EXTERNAL_ADDRESS");
        if (!config->host) {
            config->host = _cmdArgs.getNamedArgumentValue("--host", host.c_str());
        }

        const char* portText = std::getenv("EXTERNAL_PORT");
        if (!portText) {
            portText = _cmdArgs.getNamedArgumentValue("--port", port.c_str());
        }

        bool success;
        auto _port = oatpp::utils::Conversion::strToUInt32(portText, success);
        if (!success || _port > 65535) {
            throw std::runtime_error("Invalid port!");
        }
        config->port = (v_uint16)_port;

        config->tlsPrivateKeyPath = std::getenv("TLS_FILE_PRIVATE_KEY");
        if (!config->tlsPrivateKeyPath) {
            config->tlsPrivateKeyPath = _cmdArgs.getNamedArgumentValue("--tls-key", CERT_PEM_PATH.c_str());
        }

        config->tlsCertificateChainPath = std::getenv("TLS_FILE_CERT_CHAIN");
        if (!config->tlsCertificateChainPath) {
            config->tlsCertificateChainPath = _cmdArgs.getNamedArgumentValue("--tls-chain", CERT_CRT_PATH.c_str());
        }

        config->statisticsUrl = std::getenv("URL_STATS_PATH");
        if (!config->statisticsUrl) {
            config->statisticsUrl = _cmdArgs.getNamedArgumentValue("--url-stats", "admin/stats.json");
        }
        return config;
    }());

    /**
     * Create Async Executor
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::async::Executor>, executor)([] {
        return std::make_shared<oatpp::async::Executor>();
    }());

    /**
     *  Create ConnectionProvider component which listens on the port
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, serverConnectionProvider)([] {

        OATPP_COMPONENT(oatpp::Object<ConfigDto>, appConfig);

        std::shared_ptr<oatpp::network::ServerConnectionProvider> result;

        // TODO: use valid cert
        if (appConfig->useTLS) {
            OATPP_LOGd("oatpp::openssl::Config", "key_path='%s'", appConfig->tlsPrivateKeyPath->c_str());
            OATPP_LOGd("oatpp::openssl::Config", "chn_path='%s'", appConfig->tlsCertificateChainPath->c_str());

            auto config = oatpp::openssl::Config::createDefaultServerConfigShared(appConfig->tlsCertificateChainPath->c_str(),
                                                                                  appConfig->tlsPrivateKeyPath->c_str());
            result = oatpp::openssl::server::ConnectionProvider::createShared(config, {"0.0.0.0", appConfig->port, oatpp::network::Address::IP_4});
        } else {
            result = oatpp::network::tcp::server::ConnectionProvider::createShared({"0.0.0.0", appConfig->port, oatpp::network::Address::IP_4});
        }

        return result;
    }());

    /**
     *  Create Router component
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, httpRouter)([] {
        return oatpp::web::server::HttpRouter::createShared();
    }());

    /**
     *  Create ConnectionHandler component which uses Router component to route requests
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, serverConnectionHandler)("http", [] {
        OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router); // get Router component
        OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, executor); // get Async executor component
        auto handler = oatpp::web::server::AsyncHttpConnectionHandler::createShared(router, executor);
        handler->addRequestInterceptor(std::make_shared<RedirectInterceptor>());
        return handler;
    }());

    /**
     *  Create ObjectMapper component to serialize/deserialize DTOs in Contoller's API
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, apiObjectMapper)([] {
        auto mapper = std::make_shared<oatpp::json::ObjectMapper>();
        mapper->serializerConfig().mapper.includeNullFields = false;
        return mapper;
    }());

    /**
     *  Create statistics object
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<Statistics>, statistics)([] {
        return std::make_shared<Statistics>();
    }());

    /**
     *  Create chat lobby component.
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<Lobby>, lobby)([] {
        return std::make_shared<Lobby>();
    }());

    /**
     *  Create websocket connection handler
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, websocketConnectionHandler)("websocket", [] {
        OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, executor);
        OATPP_COMPONENT(std::shared_ptr<Lobby>, lobby);
        auto connectionHandler = oatpp::websocket::AsyncConnectionHandler::createShared(executor);
        connectionHandler->setSocketInstanceListener(lobby);
        return connectionHandler;
    }());
};
