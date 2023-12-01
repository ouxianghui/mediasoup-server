#ifndef MS_DEP_LIBURING_HPP
#define MS_DEP_LIBURING_HPP

#include "DepLibUV.hpp"
#include "FBS/liburing.h"
#include <functional>
#include <liburing.h>
#include <queue>

class DepLibUring
{
public:
	using onSendCallback = const std::function<void(bool sent)>;

	/* Struct for the user data field of SQE and CQE. */
	struct UserData
	{
		uint8_t store[1500]{};
		onSendCallback* cb{ nullptr };
		size_t idx{ 0 };
	};

	/* Number of submission queue entries (SQE). */
	static constexpr size_t QueueDepth{ 1024 * 4 };

	static bool IsRuntimeSupported();
	static void ClassInit();
	static void ClassDestroy();
	static flatbuffers::Offset<FBS::LibUring::Dump> FillBuffer(flatbuffers::FlatBufferBuilder& builder);
	static void StartPollingCQEs();
	static void StopPollingCQEs();
	static bool PrepareSend(
	  int sockfd, const void* data, size_t len, const struct sockaddr* addr, onSendCallback* cb);
	static bool PrepareWrite(
	  int sockfd, const void* data1, size_t len1, const void* data2, size_t len2, onSendCallback* cb);
	static void Submit();
	static void SetActive();
	static bool IsActive();

	class LibUring;

	thread_local static LibUring* liburing;

public:
	class LibUring
	{
	public:
		LibUring();
		~LibUring();
		flatbuffers::Offset<FBS::LibUring::Dump> FillBuffer(flatbuffers::FlatBufferBuilder& builder) const;
		void StartPollingCQEs();
		void StopPollingCQEs();
		bool PrepareSend(
		  int sockfd, const void* data, size_t len, const struct sockaddr* addr, onSendCallback* cb);
		bool PrepareWrite(
		  int sockfd, const void* data1, size_t len1, const void* data2, size_t len2, onSendCallback* cb);
		void Submit();
		void SetActive()
		{
			this->active = true;
		}
		bool IsActive() const
		{
			return this->active;
		}
		io_uring* GetRing()
		{
			return std::addressof(this->ring);
		}
		int GetEventFd() const
		{
			return this->efd;
		}
		void ReleaseUserDataEntry(size_t idx)
		{
			this->availableUserDataEntries.push(idx);
		}

	private:
		UserData* GetUserData();

	private:
		// io_uring instance.
		io_uring ring;
		// Event file descriptor to watch for completions.
		int efd;
		// libuv handle used to poll io_uring completions.
		uv_poll_t* uvHandle{ nullptr };
		// Whether we are currently sending RTP over io_uring.
		bool active{ false };
		// Pre-allocated UserData entries.
		UserData userDataBuffer[QueueDepth]{};
		// Indexes of available UserData entries.
		std::queue<size_t> availableUserDataEntries;
		// Submission queue entry process count.
		uint64_t sqeProcessCount{ 0u };
		// Submission queue entry miss count.
		uint64_t sqeMissCount{ 0u };
		// User data miss count.
		uint64_t userDataMissCount{ 0u };
	};
};

#endif
