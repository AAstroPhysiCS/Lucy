#pragma once

#include "Task.h"

namespace Lucy {

	struct TaskSchedulerCreateInfo {
		size_t FromThreadIndex = 0;
		size_t ToThreadIndex = std::thread::hardware_concurrency();

		uint32_t AllAffinity = ThreadApplicationAffinityIncremental;
		ThreadPriority AllPriority = ThreadPriority::Normal;
	};

	class TaskScheduler final {
	public:
		TaskScheduler(const TaskSchedulerCreateInfo& createInfo);
		~TaskScheduler();

		enum Launch : uint8_t {
			Async,
			Deferred
		};

		inline size_t GetNumWorkers() const { return m_WorkerPool.size(); }
		inline size_t GetCurrentNumTask() const { return m_TaskQueue.size(); }

		TaskId Schedule(Launch launch, TaskPriority priority, TaskFunc&& taskFunc);
		std::vector<TaskId> ScheduleBatch(Launch launch, TaskPriority priority, TaskBatchFunc&& taskBatchFunc, size_t taskCount, size_t groupSize);

		void WaitForAllTasks() const;
		void WaitForTask(TaskId taskId) const;
		void WaitForTasks(const std::vector<TaskId>& taskIds) const;
		template <size_t N>
		void WaitForTasks(const TaskId(&taskIds)[N]) const {
			for (size_t i = 0; i < N; i++)
				WaitForTask(taskIds[i]);
		}
	private:
		TaskId ScheduleInternal(Task& task, TaskPriority priority, bool isDeferred);

		TaskSchedulerCreateInfo m_CreateInfo;

		std::vector<std::thread> m_WorkerPool;
		static inline std::condition_variable s_WorkerSleepCondition;

		std::deque<Task> m_TaskQueue;
		static inline std::mutex s_TaskQueueMutex;

		std::atomic_bool m_Running = true;
		std::atomic_int m_CurrentTaskCounter;

		static inline thread_local size_t t_ThreadIndex = 0;
	};
}