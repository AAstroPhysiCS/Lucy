#pragma once

#ifdef LUCY_WINDOWS
	// For EnTT
	#ifndef NOMINMAX
	#define NOMINMAX
	#endif
	
	#include <Windows.h>
#endif

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
#include <set>
#include <ranges>

#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <fstream>
#include <any>

#include "Core/Base.h"

//TODO: #define LUCY_VULKAN etc...
#include "Renderer/VulkanAPI.h"