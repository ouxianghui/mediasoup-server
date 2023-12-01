#define MS_CLASS "DepLibUring"
// #define MS_LOG_DEV_LEVEL 3

#include "DepLibUring.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <sys/eventfd.h>
#include <sys/utsname.h>

/* Static variables. */

/* liburing instance per thread. */
thread_local DepLibUring::LibUring* DepLibUring::liburing{ nullptr };
/* Completion queue entry array used to retrieve processes tasks. */
thread_local struct io_uring_cqe* cqes[DepLibUring::QueueDepth];

/* Static methods for UV callbacks. */

inline static void onCloseFd(uv_handle_t* handle)
{
	delete reinterpret_cast<uv_poll_t*>(handle);
}

inline static void onFdEvent(uv_poll_t* handle, int status, int events)
{
	auto* liburing = static_cast<DepLibUring::LibUring*>(handle->data);
	auto count     = io_uring_peek_batch_cqe(liburing->GetRing(), cqes, DepLibUring::QueueDepth);

	// libuv uses level triggering, so we need to read from the socket to reset
	// the counter in order to avoid libuv calling this callback indefinitely.
	eventfd_t v;
	int err = eventfd_read(liburing->GetEventFd(), std::addressof(v));
	if (err < 0)
	{
		MS_ABORT("eventfd_read() failed: %s", std::strerror(-err));
	};

	for (unsigned int i{ 0 }; i < count; ++i)
	{
		struct io_uring_cqe* cqe = cqes[i];
		auto* userData           = static_cast<DepLibUring::UserData*>(io_uring_cqe_get_data(cqe));

		if (cqe->res < 0)
		{
			MS_ERROR("sending failed: %s", std::strerror(-cqe->res));

			if (userData->cb)
			{
				(*userData->cb)(false);
				delete userData->cb;
			}
		}
		else
		{
			if (userData->cb)
			{
				(*userData->cb)(true);
				delete userData->cb;
			}
		}

		io_uring_cqe_seen(liburing->GetRing(), cqe);
		liburing->ReleaseUserDataEntry(userData->idx);
	}
}

/* Static class methods */

bool DepLibUring::IsRuntimeSupported()
{
	// clang-format off
	struct utsname buffer{};
	// clang-format on

	auto err = uname(std::addressof(buffer));

	if (err != 0)
	{
		MS_THROW_ERROR("uname() failed: %s", std::strerror(err));
	}

	MS_DEBUG_TAG(info, "kernel version: %s", buffer.version);

	auto* kernelMayorCstr = buffer.release;
	auto kernelMayorLong  = strtol(kernelMayorCstr, &kernelMayorCstr, 10);

	// liburing `sento` capabilities are supported for kernel versions greather
	// than or equal to 6.
	return kernelMayorLong >= 6;
}

void DepLibUring::ClassInit()
{
	const auto mayor = io_uring_major_version();
	const auto minor = io_uring_minor_version();

	MS_DEBUG_TAG(info, "liburing version: \"%i.%i\"", mayor, minor);

	if (DepLibUring::IsRuntimeSupported())
	{
		DepLibUring::liburing = new LibUring();

		MS_DEBUG_TAG(info, "liburing supported, enabled");
	}
	else
	{
		MS_DEBUG_TAG(info, "liburing not supported, not enabled");
	}
}

void DepLibUring::ClassDestroy()
{
	MS_TRACE();

	delete DepLibUring::liburing;
}

flatbuffers::Offset<FBS::LibUring::Dump> DepLibUring::FillBuffer(flatbuffers::FlatBufferBuilder& builder)
{
	MS_TRACE();

	if (!DepLibUring::liburing)
	{
		return 0;
	}

	return DepLibUring::liburing->FillBuffer(builder);
}

void DepLibUring::StartPollingCQEs()
{
	MS_TRACE();

	if (!DepLibUring::liburing)
	{
		return;
	}

	DepLibUring::liburing->StartPollingCQEs();
}

void DepLibUring::StopPollingCQEs()
{
	MS_TRACE();

	if (!DepLibUring::liburing)
	{
		return;
	}

	DepLibUring::liburing->StopPollingCQEs();
}

bool DepLibUring::PrepareSend(
  int sockfd, const void* data, size_t len, const struct sockaddr* addr, onSendCallback* cb)
{
	MS_TRACE();

	MS_ASSERT(DepLibUring::liburing, "DepLibUring::liburing is not set");

	return DepLibUring::liburing->PrepareSend(sockfd, data, len, addr, cb);
}

bool DepLibUring::PrepareWrite(
  int sockfd, const void* data1, size_t len1, const void* data2, size_t len2, onSendCallback* cb)
{
	MS_TRACE();

	MS_ASSERT(DepLibUring::liburing, "DepLibUring::liburing is not set");

	return DepLibUring::liburing->PrepareWrite(sockfd, data1, len1, data2, len2, cb);
}

void DepLibUring::Submit()
{
	MS_TRACE();

	if (!DepLibUring::liburing)
	{
		return;
	}

	DepLibUring::liburing->Submit();
}

void DepLibUring::SetActive()
{
	MS_TRACE();

	if (!DepLibUring::liburing)
	{
		return;
	}

	DepLibUring::liburing->SetActive();
}

bool DepLibUring::IsActive()
{
	MS_TRACE();

	if (!DepLibUring::liburing)
	{
		return false;
	}

	return DepLibUring::liburing->IsActive();
}

/* Instance methods. */

DepLibUring::LibUring::LibUring()
{
	MS_TRACE();

	/**
	 * IORING_SETUP_SINGLE_ISSUER: A hint to the kernel that only a single task
	 * (or thread) will submit requests, which is used for internal optimisations.
	 */

	unsigned int flags = IORING_SETUP_SINGLE_ISSUER;

	// Initialize io_uring.
	auto err = io_uring_queue_init(DepLibUring::QueueDepth, std::addressof(this->ring), flags);

	if (err < 0)
	{
		MS_THROW_ERROR("io_uring_queue_init() failed: %s", std::strerror(-err));
	}

	// Create an eventfd instance.
	this->efd = eventfd(0, 0);

	if (this->efd < 0)
	{
		MS_THROW_ERROR("eventfd() failed: %s", std::strerror(-this->efd));
	}

	err = io_uring_register_eventfd(std::addressof(this->ring), this->efd);

	if (err < 0)
	{
		MS_THROW_ERROR("io_uring_register_eventfd() failed: %s", std::strerror(-err));
	}

	// Initialize available UserData entries.
	for (size_t i{ 0 }; i < DepLibUring::QueueDepth; ++i)
	{
		this->availableUserDataEntries.push(i);
	}
}

DepLibUring::LibUring::~LibUring()
{
	MS_TRACE();

	// Close the event file descriptor.
	const auto err = close(this->efd);

	if (err != 0)
	{
		MS_ABORT("close() failed: %s", std::strerror(-err));
	}

	// Close the ring.
	io_uring_queue_exit(std::addressof(this->ring));
}

flatbuffers::Offset<FBS::LibUring::Dump> DepLibUring::LibUring::FillBuffer(
  flatbuffers::FlatBufferBuilder& builder) const
{
	MS_TRACE();

	return FBS::LibUring::CreateDump(
	  builder, this->sqeProcessCount, this->sqeMissCount, this->userDataMissCount);
}

void DepLibUring::LibUring::StartPollingCQEs()
{
	MS_TRACE();

	// Watch the event file descriptor for completions.
	this->uvHandle = new uv_poll_t;

	auto err = uv_poll_init(DepLibUV::GetLoop(), this->uvHandle, this->efd);

	if (err != 0)
	{
		delete this->uvHandle;

		MS_THROW_ERROR("uv_poll_init() failed: %s", uv_strerror(err));
	}

	this->uvHandle->data = this;

	err = uv_poll_start(this->uvHandle, UV_READABLE, static_cast<uv_poll_cb>(onFdEvent));

	if (err != 0)
	{
		MS_THROW_ERROR("uv_poll_start() failed: %s", uv_strerror(err));
	}
}

void DepLibUring::LibUring::StopPollingCQEs()
{
	MS_TRACE();

	this->uvHandle->data = nullptr;

	// Stop polling the event file descriptor.
	auto err = uv_poll_stop(this->uvHandle);

	if (err != 0)
	{
		MS_ABORT("uv_poll_stop() failed: %s", uv_strerror(err));
	}

	// NOTE: Handles that wrap file descriptors are clossed immediately.
	uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onCloseFd));
}

bool DepLibUring::LibUring::PrepareSend(
  int sockfd, const void* data, size_t len, const struct sockaddr* addr, onSendCallback* cb)
{
	MS_TRACE();

	auto* userData = this->GetUserData();

	if (!userData)
	{
		MS_WARN_DEV("no user data entry available");

		this->userDataMissCount++;

		return false;
	}

	auto* sqe = io_uring_get_sqe(std::addressof(this->ring));

	if (!sqe)
	{
		MS_WARN_DEV("no sqe available");

		this->sqeMissCount++;

		return false;
	}

	std::memcpy(userData->store, data, len);
	userData->cb = cb;

	io_uring_sqe_set_data(sqe, userData);

	socklen_t addrlen = 0;

	if (addr->sa_family == AF_INET)
	{
		addrlen = sizeof(struct sockaddr_in);
	}
	else if (addr->sa_family == AF_INET6)
	{
		addrlen = sizeof(struct sockaddr_in6);
	}

	io_uring_prep_sendto(sqe, sockfd, userData->store, len, 0, addr, addrlen);

	this->sqeProcessCount++;

	return true;
}

bool DepLibUring::LibUring::PrepareWrite(
  int sockfd, const void* data1, size_t len1, const void* data2, size_t len2, onSendCallback* cb)
{
	MS_TRACE();

	auto* userData = this->GetUserData();

	if (!userData)
	{
		MS_WARN_DEV("no user data entry available");

		this->userDataMissCount++;

		return false;
	}

	auto* sqe = io_uring_get_sqe(std::addressof(this->ring));

	if (!sqe)
	{
		MS_WARN_DEV("no sqe available");

		this->sqeMissCount++;

		return false;
	}

	std::memcpy(userData->store, data1, len1);
	std::memcpy(userData->store + len1, data2, len2);
	userData->cb = cb;

	io_uring_sqe_set_data(sqe, userData);
	io_uring_prep_write(sqe, sockfd, userData->store, len1 + len2, 0);

	this->sqeProcessCount++;

	return true;
}

void DepLibUring::LibUring::Submit()
{
	MS_TRACE();

	// Unset active flag.
	this->active = false;

	auto err = io_uring_submit(std::addressof(this->ring));

	if (err >= 0)
	{
		MS_DEBUG_DEV("%i submission queue entries submitted", err);
	}
	else
	{
		MS_ERROR("io_uring_submit() failed: %s", std::strerror(-err));
	}
}

DepLibUring::UserData* DepLibUring::LibUring::GetUserData()
{
	MS_TRACE();

	if (this->availableUserDataEntries.empty())
	{
		MS_WARN_DEV("no user data entry available");

		return nullptr;
	}

	auto idx = this->availableUserDataEntries.front();

	this->availableUserDataEntries.pop();

	auto* userData = std::addressof(this->userDataBuffer[idx]);
	userData->idx  = idx;

	return userData;
}
