#include "lypch.h"
#include "Memory.h"

#include "Core/Application.h"

void* operator new(size_t size) {
	using namespace Lucy;
	ApplicationMetrics& metrics = Application::GetApplicationMetrics();

#ifdef LUCY_WINDOWS
	if (void* ptr = malloc(size)) {
		metrics.m_TotalMemAllocated += size;
		return ptr;
	}
#endif

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
#ifdef LUCY_WINDOWS
	free(o);
#endif
}