#pragma once

#include <string>

// deps the Worker project
#include "common.hpp"
#include "unix_stream_socket_handle.h"

namespace srv
{
	class ConsumerSocket : public UnixStreamSocketHandle
	{
	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			virtual void OnConsumerSocketMessage(ConsumerSocket* consumerSocket, char* msg, size_t msgLen) = 0;
			virtual void OnConsumerSocketClosed(ConsumerSocket* consumerSocket) = 0;
		};

	public:
		ConsumerSocket(uv_loop_t* loop, int fd, size_t bufferSize, Listener* listener);
		~ConsumerSocket();

		/* Pure virtual methods inherited from UnixStreamSocketHandle. */
	public:
		void UserOnUnixStreamRead() override;
		void UserOnUnixStreamSocketClosed() override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
	};

	class ProducerSocket : public UnixStreamSocketHandle
	{
	public:
		ProducerSocket(uv_loop_t* loop, int fd, size_t bufferSize);

		/* Pure virtual methods inherited from UnixStreamSocketHandle. */
	public:
		void UserOnUnixStreamRead() override {}
		void UserOnUnixStreamSocketClosed() override {}
	};

	class ChannelSocket : public ConsumerSocket::Listener
	{
	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
            virtual void OnChannelMessage(char* msg, size_t msgLen) = 0;
			virtual void OnChannelClosed(srv::ChannelSocket* channel) = 0;
		};

	public:
		explicit ChannelSocket(uv_loop_t* loop, int consumerFd, int producerFd);
		virtual ~ChannelSocket();

	public:
		void Close();
		void SetListener(Listener* listener);
		void Send(const uint8_t* message, uint32_t messageLen);

	private:
		void SendImpl(const uint8_t* payload, uint32_t payloadLen);

		/* Pure virtual methods inherited from ConsumerSocket::Listener. */
	public:
		void OnConsumerSocketMessage(ConsumerSocket* consumerSocket, char* msg, size_t msgLen) override;
		void OnConsumerSocketClosed(ConsumerSocket* consumerSocket) override;

	private:
		// Passed by argument.
		Listener* listener { nullptr };
        
		// Others.
		bool closed { false };
		ConsumerSocket* consumerSocket{ nullptr };
		ProducerSocket* producerSocket{ nullptr };
	};
} // namespace srv
