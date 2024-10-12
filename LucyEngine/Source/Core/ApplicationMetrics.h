#pragma once

namespace Lucy {

	struct ApplicationMetrics final {
	private:
		static inline constexpr double s_ConversionValueMB = 1e6;
	public:
		ApplicationMetrics() = default;
		~ApplicationMetrics() = default;

		///Size in megabytes
		inline double GetTotalAllocated() const { return m_TotalMemAllocated / s_ConversionValueMB; }

		///Size in megabytes
		inline double GetTotalFreed() const { return m_TotalMemFreed / s_ConversionValueMB; }

		///Size in megabytes
		inline double GetCurrentUsage() const { return m_CurrentMemUsage / s_ConversionValueMB; }

		inline double GetDeltaTime() const { return m_DeltaTime; }
		inline double GetFrameTime() const { return m_FrameTime; }
		inline double GetFrames() const { return m_Frames; }

		friend void* ::operator new(std::size_t size);
		friend void ::operator delete(void* o, std::size_t size);

		void Update();
	private:
		double m_DeltaTime = 0.0;
		double m_FrameTime = 0.0;
		double m_Frames = 0.0;

		uint64_t m_TotalMemAllocated = 0;
		uint64_t m_TotalMemFreed = 0;
		uint64_t m_CurrentMemUsage = 0;
	};
}