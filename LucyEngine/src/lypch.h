#pragma once

#ifdef LUCY_WINDOWS
	// For EnTT
	#ifndef NOMINMAX
	#define NOMINMAX
	#endif
	
	#include <Windows.h>
#endif

#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>

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
#include "Core/Logger.h"