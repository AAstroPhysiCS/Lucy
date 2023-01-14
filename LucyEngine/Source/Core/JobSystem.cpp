#include "lypch.h"
#include "JobSystem.h"

namespace Lucy {

	JobSystem::JobSystem(uint32_t maxThreads)
		: m_MaxThreads(maxThreads) {
		m_Workers.reserve(m_MaxThreads);

		for (uint32_t threadIndex = 0; threadIndex < m_MaxThreads; threadIndex++) {
			m_Workers.emplace_back(([this, threadIndex]() {
				while (m_Running) {
					Job job;
					while (m_JobQueue.Pop(job)) {
						job.Args.ThreadIndex = threadIndex;
						job.JobFnc(job.Args);

						m_CurrentJobCounter.fetch_sub(1);
					}

					std::unique_lock<std::mutex> lock(s_Mutex);
					//I am not sure about the lambda function that waits for !m_Running.load().
					m_WakeUpCondition.wait(lock);
				}
			}));

			std::thread& worker = m_Workers.back();
			HANDLE handle = worker.native_handle();

			DWORD_PTR affinityMask = 1ull << threadIndex;
			DWORD_PTR affinityResult = SetThreadAffinityMask(handle, affinityMask);
			LUCY_ASSERT(affinityResult < 0);

			BOOL priorityResult = SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST);
			LUCY_ASSERT(priorityResult == 0);

			std::wstringstream wss;
			wss << "LucyJobSystem" << threadIndex;
			HRESULT hr = SetThreadDescription(handle, wss.str().c_str());
			LUCY_ASSERT(SUCCEEDED(hr));
		}
	}

	JobSystem::~JobSystem() {
		m_Running = false;
		m_WakeUpCondition.notify_all();
		Join();
	};

	void JobSystem::ExecuteJob(const JobFnc& jobFnc) {
		Job job;
		job.JobFnc = jobFnc;
		job.Args.JobIndex = m_JobQueue.GetSize();

		m_CurrentJobCounter.fetch_add(1);

		m_JobQueue.Push(job);
		m_WakeUpCondition.notify_one();
	}

	void JobSystem::Dispatch(uint32_t jobCount, uint32_t groupSize, const JobFnc&& jobFnc) {
		const uint32_t groupCount = (jobCount + groupSize - 1) / groupSize;

		m_CurrentJobCounter.fetch_add(groupCount);

		for (uint32_t groupID = 0; groupID < groupCount; groupID++) {
			for (uint32_t groupOffset = 0; groupOffset < groupSize; groupOffset++) {
				Job job;
				//ThreadIndex will be filled up later...
				job.Args = { 0u, m_JobQueue.GetSize(), groupID };
				job.JobFnc = jobFnc;
				m_JobQueue.Push(job);
			}
		}

		m_WakeUpCondition.notify_all();
	}

	void JobSystem::WaitIdle() {
		while (m_CurrentJobCounter.load() > 0) {
			m_WakeUpCondition.notify_all();
		}
	}

	void JobSystem::Join() {
		for (uint32_t threadIndex = 0; threadIndex < m_MaxThreads; threadIndex++)
			m_Workers[threadIndex].join();
	}
}