#pragma once

#include "Core/Base.h"
#include "SchedulerCommon.h"

namespace Lucy {

	struct RunnableThreadCreateInfo {
		std::string Name = "UnnamedLucyThread";
		uint32_t Affinity = ThreadApplicationAffinityIncremental;
		ThreadPriority Priority = ThreadPriority::Normal;
	};

	class RunnableThread : public MemoryTrackable {
	public:
		RunnableThread(const RunnableThreadCreateInfo& createInfo)
			: m_DebugName(createInfo.Name), m_Affinity(createInfo.Affinity), m_Priority(createInfo.Priority) {
		}
		virtual ~RunnableThread() = default;

		virtual bool OnInit(uint32_t threadIndex) = 0;
		virtual uint32_t OnRun() = 0;
		virtual void OnJoin() = 0;
		virtual void OnStop() = 0;

		inline const std::string& GetDebugName() const { return m_DebugName; }

		inline ThreadPriority GetPriority() const { return m_Priority; }
		inline uint32_t GetAffinity() const { return m_Affinity; }

		inline void SetID(std::thread::id id) { m_ID = id; }
		inline std::thread::id GetID() const { return m_ID; }
	private:
		std::thread::id m_ID;
		std::string m_DebugName;
		//TODO: For other platforms
#ifdef LUCY_WINDOWS
		uint32_t m_Affinity = ThreadApplicationAffinityIncremental;
		ThreadPriority m_Priority = ThreadPriority::Normal;
#endif
	};
}
