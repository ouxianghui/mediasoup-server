/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: RTC-SFU
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-11-1
*************************************************************************/

#pragma once

#include "oatpp/Types.hpp"
#include "oatpp/macro/codegen.hpp"
#include "oatpp/data/stream/BufferStream.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class ConfigDto : public oatpp::DTO
{
public:
    DTO_INIT(ConfigDto, DTO)

    DTO_FIELD(String, statisticsUrl);

    DTO_FIELD(String, host);
    DTO_FIELD(UInt16, port);
    DTO_FIELD(Boolean, useTLS) = true;

    /**
     * Path to TLS private key file.
     */
    DTO_FIELD(String, tlsPrivateKeyPath);

    /**
     * Path to TLS certificate chain file.
     */
    DTO_FIELD(String, tlsCertificateChainPath);

    /**
     * Max size of the received bytes. (the whole MessageDto structure).
     * The actual payload is smaller.
     */
    DTO_FIELD(UInt64, maxMessageSizeBytes) = 24 * 1024; // Default - 8Kb

public:
    oatpp::String getHostString() {
        oatpp::data::stream::BufferOutputStream stream(256);
        v_uint16 defPort;
        if (useTLS) {
            defPort = 443;
        } else {
            defPort = 80;
        }
        stream << host;
        if (!port || defPort != port) {
            stream << ":" << port;
        }
        return stream.toString();
    }

    oatpp::String getCanonicalBaseUrl() {
        oatpp::data::stream::BufferOutputStream stream(256);
        v_uint16 defPort;
        if (useTLS) {
            stream << "https://";
            defPort = 443;
        } else {
            stream << "http://";
            defPort = 80;
        }
        stream << host;
        if (!port || defPort != port) {
            stream << ":" << port;
        }
        return stream.toString();
    }

    oatpp::String getWebsocketBaseUrl() {
        oatpp::data::stream::BufferOutputStream stream(256);
        if (useTLS) {
            stream << "wss://";
        } else {
            stream << "ws://";
        }
        stream << host << ":" << port;
        return stream.toString();
    }

    oatpp::String getStatsUrl() {
        return getCanonicalBaseUrl() + "/" + statisticsUrl;
    }
};

#include OATPP_CODEGEN_END(DTO)
