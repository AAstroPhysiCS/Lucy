#pragma once

#include "Core/Base.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>

namespace Lucy {
	/*
	* Influenced by: https://ubm-twvideo01.s3.amazonaws.com/o1/vault/gdc2015/presentations/Gyrling_Christian_Parallelizing_The_Naughty.pdf
	* which used fibers... After reading about fibers I decided not to use it as it also comes with some hefty downsides.
	* For more, see here: https://www.open-std.org/JTC1/SC22/WG21/docs/papers/2018/p1364r0.pdf
	*/

	struct JobArgs {
		uint32_t ThreadIndex = ~0u;
		size_t JobIndex = ~0ull;
		uint32_t GroupID = ~0u;
	};

	template <typename Param>
	using Fnc = std::function<void(Param)>;

	using JobFnc = Fnc<JobArgs>;

	struct Job {
		JobFnc JobFnc;
		JobArgs Args;
	};

	template <typename T, size_t Size = ~0ull>
	class ThreadSafeQueue {
	private:
		ThreadSafeQueue() = default;
		~ThreadSafeQueue() = default;

		friend class JobSystem;
	public:
		ThreadSafeQueue(const ThreadSafeQueue& other) = delete;
		ThreadSafeQueue(ThreadSafeQueue&& other) = delete;
		ThreadSafeQueue& operator=(const ThreadSafeQueue& other) = delete;
		ThreadSafeQueue& operator=(ThreadSafeQueue&& other) = delete;

		bool Push(const T& element) {
			std::unique_lock<std::mutex> lock(s_Mutex);
			if (GetSize() == Size)
				return false;
			m_Queue.push(element);
			return true;
		}

		bool Pop(T& element) {
			std::unique_lock<std::mutex> lock(s_Mutex);
			if (IsEmpty())
				return false;
			element = std::move(m_Queue.front());
			m_Queue.pop();
			return true;
		}

		void Clear() {
			std::unique_lock<std::mutex> lock(s_Mutex);
			for (uint32_t i = 0; i < m_Queue.size(); i++)
				m_Queue.pop();
		}

		inline auto GetSize() { return m_Queue.size(); }
		inline bool IsEmpty() { return m_Queue.empty(); }
	private:
		std::queue<T> m_Queue;
		inline static std::mutex s_Mutex; /* static is important here */
	};

	class JobSystem final {
	public:
		JobSystem(uint32_t maxThreads = std::thread::hardware_concurrency());
		~JobSystem();

		void ExecuteJob(const JobFnc& jobFnc);
		void Dispatch(uint32_t jobCount, uint32_t groupSize, const JobFnc&& jobFnc);
		void WaitIdle();
	private:
		void Join();

		ThreadSafeQueue<Job> m_JobQueue;

		std::vector<std::thread> m_Workers;

		std::condition_variable m_WakeUpCondition;
		std::condition_variable m_WaitToFinishCondition;

		std::atomic_int m_CurrentJobCounter;
		std::atomic_bool m_Running = true;
		uint32_t m_MaxThreads;

		inline static std::mutex s_Mutex; /* static is important here */
	};
}