#ifndef MS_RTC_ICE_CANDIDATE_HPP
#define MS_RTC_ICE_CANDIDATE_HPP

#include "common.hpp"
#include "FBS/webRtcTransport.h"
#include "RTC/TcpServer.hpp"
#include "RTC/TransportTuple.hpp"
#include "RTC/UdpSocket.hpp"
#include <flatbuffers/flatbuffers.h>
#include <string>

namespace RTC
{
	class IceCandidate
	{
		using Protocol = TransportTuple::Protocol;

	public:
		enum class CandidateType
		{
			HOST = 1
		};

	public:
		enum class TcpCandidateType
		{
			PASSIVE = 1
		};

	public:
		static CandidateType CandidateTypeFromFbs(FBS::WebRtcTransport::IceCandidateType type);
		static FBS::WebRtcTransport::IceCandidateType CandidateTypeToFbs(CandidateType type);
		static TcpCandidateType TcpCandidateTypeFromFbs(FBS::WebRtcTransport::IceCandidateTcpType type);
		static FBS::WebRtcTransport::IceCandidateTcpType TcpCandidateTypeToFbs(TcpCandidateType type);

	public:
		IceCandidate(RTC::UdpSocket* udpSocket, uint32_t priority)
		  : foundation("udpcandidate"), priority(priority), ip(udpSocket->GetLocalIp()),
		    protocol(Protocol::UDP), port(udpSocket->GetLocalPort()), type(CandidateType::HOST)
		{
		}

		IceCandidate(RTC::UdpSocket* udpSocket, uint32_t priority, std::string& announcedIp)
		  : foundation("udpcandidate"), priority(priority), ip(announcedIp), protocol(Protocol::UDP),
		    port(udpSocket->GetLocalPort()), type(CandidateType::HOST)
		{
		}

		IceCandidate(RTC::TcpServer* tcpServer, uint32_t priority)
		  : foundation("tcpcandidate"), priority(priority), ip(tcpServer->GetLocalIp()),
		    protocol(Protocol::TCP), port(tcpServer->GetLocalPort()), type(CandidateType::HOST),
		    tcpType(TcpCandidateType::PASSIVE)
		{
		}

		IceCandidate(RTC::TcpServer* tcpServer, uint32_t priority, std::string& announcedIp)
		  : foundation("tcpcandidate"), priority(priority), ip(announcedIp), protocol(Protocol::TCP),
		    port(tcpServer->GetLocalPort()), type(CandidateType::HOST),
		    tcpType(TcpCandidateType::PASSIVE)
		{
		}

		flatbuffers::Offset<FBS::WebRtcTransport::IceCandidate> FillBuffer(
		  flatbuffers::FlatBufferBuilder& builder) const;

	private:
		// Others.
		std::string foundation;
		uint32_t priority{ 0u };
		std::string ip;
		Protocol protocol;
		uint16_t port{ 0u };
		CandidateType type{ CandidateType::HOST };
		TcpCandidateType tcpType{ TcpCandidateType::PASSIVE };
	};
} // namespace RTC

#endif
