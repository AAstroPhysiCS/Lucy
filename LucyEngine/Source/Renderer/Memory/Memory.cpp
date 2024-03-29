#include "lypch.h"
#include "Memory.h"

#include "Core/Application.h"

void* __cdecl operator new(size_t size) {
	using namespace Lucy;
	Metrics& metrics = Application::GetApplicationMetrics();

	if (void* ptr = malloc(size)) {
		metrics.MemTracker.m_TotalAllocated += size;
		return ptr;
	}

	Logger::LogCritical("Operator new failed to allocate memory!");
#ifdef LUCY_WINDOWS
	__debugbreak();
#endif
	return nullptr;
}

void operator delete(void* o, size_t size) {
	using namespace Lucy;
	Metrics& metrics = Application::GetApplicationMetrics();

	metrics.MemTracker.m_TotalFreed += size;
	free(o);
}