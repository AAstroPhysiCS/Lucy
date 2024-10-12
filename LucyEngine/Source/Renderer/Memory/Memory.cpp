#include "lypch.h"
#include "Memory.h"

#include "Core/Application.h"

void* __cdecl operator new(size_t size) {
	using namespace Lucy;
	ApplicationMetrics& metrics = Application::GetApplicationMetrics();

	if (void* ptr = malloc(size)) {
		metrics.m_TotalMemAllocated += size;
		return ptr;
	}

	LUCY_CRITICAL("Operator new failed to allocate memory!");
#ifdef LUCY_WINDOWS
	__debugbreak();
#endif
	return nullptr;
}

void operator delete(void* o, size_t size) {
	using namespace Lucy;
	ApplicationMetrics& metrics = Application::GetApplicationMetrics();

	metrics.m_TotalMemFreed += size;
	free(o);
}