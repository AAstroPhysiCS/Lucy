#pragma once

#include <queue>

#include "Core/Base.h"

#include "SchedulerCommon.h"

namespace Lucy {

	/*
	* Influenced by: https://ubm-twvideo01.s3.amazonaws.com/o1/vault/gdc2015/presentations/Gyrling_Christian_Parallelizing_The_Naughty.pdf
	* which used fibers... After reading about fibers I decided not to use it as it also comes with some hefty downsides.
	* For more, see here: https://www.open-std.org/JTC1/SC22/WG21/docs/papers/2018/p1364r0.pdf
	*/

	using TaskId = int32_t;

	struct TaskArgs {
		size_t TaskIndex = 0;
		size_t ThreadIndex = 0;
		void* SharedMemory = nullptr;
	};

	struct TaskBatchArgs {
		size_t BatchIndex = 0;
		size_t BatchOffset = 0;
	};

	using TaskFunc = std::function<void(TaskArgs)>;
	using TaskBatchFunc = std::function<void(TaskArgs, TaskBatchArgs)>;

	static_assert(std::is_invocable_v<TaskFunc, TaskArgs>&& std::is_invocable_v<TaskBatchFunc, TaskArgs, TaskBatchArgs>);

	enum class TaskPriority : uint8_t {
		Low,
		Medium,
		High
	};

	struct Task {
		TaskId Id;
		TaskArgs Args;
		TaskFunc Func;

		bool IsBatch = false;
		TaskBatchArgs BatchArgs;
		TaskBatchFunc BatchFunc;
	};
}