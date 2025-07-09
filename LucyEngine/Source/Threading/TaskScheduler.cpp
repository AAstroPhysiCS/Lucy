#include "lypch.h"
#include "TaskScheduler.h"

namespace Lucy {

	TaskScheduler::TaskScheduler(const TaskSchedulerCreateInfo& createInfo)
		: m_CreateInfo(createInfo) {
		for (size_t threadIndex = m_CreateInfo.FromThreadIndex; threadIndex < m_CreateInfo.ToThreadIndex; threadIndex++) {

			const auto& WorkerThreadFunc = [threadIndex, this]() {
				t_ThreadIndex = threadIndex;

				Task task;
				while (m_Running.load()) {
					{
						std::unique_lock lock(s_TaskQueueMutex);
						if (m_TaskQueue.empty()) {
							s_WorkerSleepCondition.wait(lock, [this]() { return !m_TaskQueue.empty() || !m_Running; });
							continue;
						}

						task = std::move(m_TaskQueue.front());
						m_TaskQueue.pop_front();

						task.Args.TaskIndex = m_TaskQueue.size();
					}

					task.Args.ThreadIndex = t_ThreadIndex;
					if (task.IsBatch)
						task.BatchFunc(task.Args, task.BatchArgs);
					else
						task.Func(task.Args);

					m_CurrentTaskCounter.fetch_sub(1, std::memory_order_release);
				}
			};

#ifdef LUCY_WINDOWS
			std::thread& thread = m_WorkerPool.emplace_back(WorkerThreadFunc);

			HANDLE handle = thread.native_handle();
			uint32_t affinity = m_CreateInfo.AllAffinity;

			SetThreadArguments(handle, std::string("LucyTaskScheduler").append(std::to_string(threadIndex)),
							   affinity == ThreadApplicationAffinityIncremental ? 1uLL << threadIndex : affinity,
							   ConvertToPlatformSpecificPrioritySystem(ThreadPriority::Highest));
#endif
		}
	}

	TaskScheduler::~TaskScheduler() {
		WaitForAllTasks();

		m_Running = false;
		s_WorkerSleepCondition.notify_all();

		for (auto& thread : m_WorkerPool) {
			LUCY_ASSERT(thread.joinable(), "Faulty thread join!");
			thread.join();
		}
	}

	TaskId TaskScheduler::Schedule(Launch launch, TaskPriority priority, TaskFunc&& taskFunc) {
		Task task;
		task.IsBatch = false;
		task.Func = std::move(taskFunc);

		return ScheduleInternal(task, priority, launch != Async);
	}

	std::vector<TaskId> TaskScheduler::ScheduleBatch(Launch launch, TaskPriority priority, TaskBatchFunc&& taskBatchFunc, size_t taskCount, size_t batchSize) {
		size_t taskIdIndex = 0;
		std::vector<TaskId> taskIds;
		taskIds.resize(taskCount * batchSize, -1);

		for (size_t batchIndex = 0; batchIndex < taskCount; batchIndex++) {
			for (size_t batchOffset = 0; batchOffset < batchSize; batchOffset++) {
				Task task;
				task.IsBatch = true;
				task.BatchFunc = taskBatchFunc;
				task.BatchArgs.BatchIndex = batchIndex;
				task.BatchArgs.BatchOffset = batchOffset;

				taskIds[taskIdIndex++] = ScheduleInternal(task, priority, false);

				s_WorkerSleepCondition.notify_one();
			}
		}

		if (launch != Async) {
			for (auto id : taskIds)
				WaitForTask(id);
		}

		return taskIds;
	}

	TaskId TaskScheduler::ScheduleInternal(Task& task, TaskPriority priority, bool isDeferred) {
		task.Id = m_CurrentTaskCounter.fetch_add(1, std::memory_order_relaxed);
		{
			std::unique_lock lock(s_TaskQueueMutex);
			if (priority > TaskPriority::Medium)
				m_TaskQueue.push_front(task);
			else
				m_TaskQueue.push_back(task);
		}

		if (isDeferred) {
			WaitForTask(task.Id);
			return -1;
		}

		return task.Id;
	}

	void TaskScheduler::WaitForAllTasks() const {
		while (m_CurrentTaskCounter.load(std::memory_order_relaxed) != 0) {
			/* Wait for all the tasks to finish */
			s_WorkerSleepCondition.notify_all();
		}
	}

	void TaskScheduler::WaitForTask(TaskId taskId) const {
		LUCY_ASSERT(taskId != -1, "Waiting for an invalid task is prohibited");
		while (m_CurrentTaskCounter.load(std::memory_order_relaxed) > taskId) {
			/* Wait for the task to finish */
			s_WorkerSleepCondition.notify_all();
		}
	}

	void TaskScheduler::WaitForTasks(const std::vector<TaskId>& taskIds) const {
		for (size_t i = 0; i < taskIds.size(); i++)
			WaitForTask(taskIds[i]);
	}
}