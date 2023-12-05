#define MS_CLASS "srv::ChannelSocket"

#include "channel_socket.h"
#include <cstring> // std::memcpy(), std::memmove()
#include "srv_logger.h"

namespace srv
{
	// Binary length for a 4194304 bytes payload.
	static constexpr size_t MessageMaxLen{ 4194308 };
	static constexpr size_t PayloadMaxLen{ 4194304 };

	/* Instance methods. */

	ChannelSocket::ChannelSocket(uv_loop_t* loop, int consumerFd, int producerFd)
	  : consumerSocket(new ConsumerSocket(loop, consumerFd, MessageMaxLen, this)),
	    producerSocket(new ProducerSocket(loop, producerFd, MessageMaxLen))
	{
		SRV_LOGD("ChannelSocket()");
	}

	ChannelSocket::~ChannelSocket()
	{
        SRV_LOGD("~ChannelSocket()");

		if (!this->closed) {
			Close();
		}

		delete this->consumerSocket;
		delete this->producerSocket;
	}

	void ChannelSocket::Close()
	{
        SRV_LOGD("Close()");

		if (this->closed) {
			return;
		}

		this->closed = true;

		if (this->consumerSocket) {
			this->consumerSocket->Close();
		}

		if (this->producerSocket) {
			this->producerSocket->Close();
		}
	}

	void ChannelSocket::SetListener(Listener* listener)
	{
        SRV_LOGD("SetListener()");

		this->listener = listener;
	}

	void ChannelSocket::Send(const uint8_t* message, uint32_t messageLen)
	{
        SRV_LOGD("Send()");

		if (this->closed) {
			return;
		}

		if (messageLen > PayloadMaxLen) {
            SRV_LOGE("message too big");
			return;
		}

		SendImpl(reinterpret_cast<const uint8_t*>(message), messageLen);
	}

	inline void ChannelSocket::SendImpl(const uint8_t* payload, uint32_t payloadLen)
	{
        SRV_LOGD("SendImpl()");

        this->producerSocket->Write(payload, payloadLen);
	}

	void ChannelSocket::OnConsumerSocketMessage(ConsumerSocket* /*consumerSocket*/, char* msg, size_t msgLen)
	{
        SRV_LOGD("OnConsumerSocketMessage()");

        this->listener->OnChannelMessage(msg, msgLen);
	}

	void ChannelSocket::OnConsumerSocketClosed(ConsumerSocket* /*consumerSocket*/)
	{
        SRV_LOGD("OnConsumerSocketClosed()");

		this->listener->OnChannelClosed(this);
	}

	/* Instance methods. */

	ConsumerSocket::ConsumerSocket(uv_loop_t* loop, int fd, size_t bufferSize, Listener* listener)
	  : UnixStreamSocketHandle(loop, fd, bufferSize, UnixStreamSocketHandle::Role::CONSUMER),
	    listener(listener)
	{
        SRV_LOGD("ConsumerSocket()");
	}

	ConsumerSocket::~ConsumerSocket()
	{
        SRV_LOGD("~ConsumerSocket()");
	}

	void ConsumerSocket::UserOnUnixStreamRead()
	{
        SRV_LOGD("UserOnUnixStreamRead()");

		size_t msgStart{ 0 };

		// Be ready to parse more than a single message in a single chunk.
		while (true) {
			if (IsClosed()) {
				return;
			}

			size_t readLen = this->bufferDataLen - msgStart;

			if (readLen < sizeof(uint32_t)) {
				// Incomplete data.
				break;
			}

			uint32_t msgLen;
			// Read message length.
			std::memcpy(&msgLen, this->buffer + msgStart, sizeof(uint32_t));

			if (readLen < sizeof(uint32_t) + static_cast<size_t>(msgLen)) {
				// Incomplete data.
				break;
			}

			this->listener->OnConsumerSocketMessage(this,
                                                    reinterpret_cast<char*>(this->buffer + msgStart + sizeof(uint32_t)),
                                                    static_cast<size_t>(msgLen));

			msgStart += sizeof(uint32_t) + static_cast<size_t>(msgLen);
		}

		if (msgStart != 0) {
			this->bufferDataLen = this->bufferDataLen - msgStart;

			if (this->bufferDataLen != 0)
			{
				std::memmove(this->buffer, this->buffer + msgStart, this->bufferDataLen);
			}
		}
	}

	void ConsumerSocket::UserOnUnixStreamSocketClosed()
	{
        SRV_LOGD("UserOnUnixStreamSocketClosed()");

		// Notify the listener.
		this->listener->OnConsumerSocketClosed(this);
	}

	/* Instance methods. */

	ProducerSocket::ProducerSocket(uv_loop_t* loop, int fd, size_t bufferSize)
	  : UnixStreamSocketHandle(loop, fd, bufferSize, UnixStreamSocketHandle::Role::PRODUCER)
	{
        SRV_LOGD("ProducerSocket()");
	}
} // namespace srv
