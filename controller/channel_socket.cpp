#define MS_CLASS "srv::ChannelSocket"

#include <cstring> // std::memcpy(), std::memmove()
#include <assert.h>
#include "channel_socket.h"
#include "srv_logger.h"

namespace
{
    inline static void onCloseLoop(uv_handle_t* handle)
    {
        delete reinterpret_cast<uv_loop_t*>(handle);
    }

    inline static void onWalk(uv_handle_t* handle, void* /*arg*/)
    {
        // Must use MS_ERROR_STD since at this point the Channel is already closed.
        SRV_LOGD("alive UV handle found (this shouldn't happen) [type:%s, active:%d, closing:%d, has_ref:%d]", uv_handle_type_name(handle->type), uv_is_active(handle), uv_is_closing(handle), uv_has_ref(handle));
        if (!uv_is_closing(handle)) {
            uv_close(handle, onCloseLoop);
        }
    }
}

namespace srv
{
	// Binary length for a 4194304 bytes payload.
	static constexpr size_t MessageMaxLen{ 4194308 };
	static constexpr size_t PayloadMaxLen{ 4194304 };

	/* Instance methods. */

    ChannelSocket::Loop::Loop()
    {
        _loop = new uv_loop_t();
        assert(_loop);
        
        uv_loop_init(_loop);
    }

    ChannelSocket::Loop::~Loop()
    {
        if (_loop) {
            int err;
            uv_stop(_loop);
            uv_walk(_loop, onWalk, nullptr);
            
            while (true) {
                err = uv_loop_close(_loop);
                if (err != UV_EBUSY) {
                    break;
                }
                uv_run(_loop, UV_RUN_NOWAIT);
            }
            
            if (err != 0) {
                SRV_LOGE("failed to close libuv loop: %s", uv_err_name(err));
            }
            
            delete _loop;
            _loop = nullptr;
            
            _thread.join();
        }
    }

    void ChannelSocket::Loop::run()
    {
        _thread = std::thread([this](){
            uv_run(this->_loop, uv_run_mode::UV_RUN_DEFAULT);
        });
    }

    ChannelSocket::ChannelSocket(int consumerFd, int producerFd)
    : _consumerSocket(new ConsumerSocket(_loop.get(), consumerFd, MessageMaxLen, this))
    , _producerSocket(new ProducerSocket(_loop.get(), producerFd, MessageMaxLen))
    {
        SRV_LOGD("ChannelSocket()");
        
        _loop.run();
    }

	ChannelSocket::~ChannelSocket()
	{
        SRV_LOGD("~ChannelSocket()");
        
		if (!this->_closed) {
			Close();
		}

		delete this->_consumerSocket;
		delete this->_producerSocket;
	}

	void ChannelSocket::Close()
	{
        SRV_LOGD("Close()");

		if (this->_closed) {
			return;
		}

		this->_closed = true;

		if (this->_consumerSocket) {
			this->_consumerSocket->Close();
		}

		if (this->_producerSocket) {
			this->_producerSocket->Close();
		}
	}

	void ChannelSocket::SetListener(Listener* listener)
	{
        SRV_LOGD("SetListener()");

		this->_listener = listener;
	}

	void ChannelSocket::Send(const uint8_t* message, uint32_t messageLen)
	{
        SRV_LOGD("Send()");

		if (this->_closed) {
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

        this->_producerSocket->Write(payload, payloadLen);
	}

	void ChannelSocket::OnConsumerSocketMessage(ConsumerSocket* /*consumerSocket*/, char* msg, size_t msgLen)
	{
        SRV_LOGD("OnConsumerSocketMessage()");

        this->_listener->OnChannelMessage(msg, msgLen);
	}

	void ChannelSocket::OnConsumerSocketClosed(ConsumerSocket* /*consumerSocket*/)
	{
        SRV_LOGD("OnConsumerSocketClosed()");

		this->_listener->OnChannelClosed(this);
	}

	/* Instance methods. */

	ConsumerSocket::ConsumerSocket(uv_loop_t* loop, int fd, size_t bufferSize, Listener* listener)
	  : UnixStreamSocketHandle(loop, fd, bufferSize, UnixStreamSocketHandle::Role::CONSUMER),
	    _listener(listener)
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

			this->_listener->OnConsumerSocketMessage(this,
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
		this->_listener->OnConsumerSocketClosed(this);
	}

	/* Instance methods. */

	ProducerSocket::ProducerSocket(uv_loop_t* loop, int fd, size_t bufferSize)
	  : UnixStreamSocketHandle(loop, fd, bufferSize, UnixStreamSocketHandle::Role::PRODUCER)
	{
        SRV_LOGD("ProducerSocket()");
	}
} // namespace srv
