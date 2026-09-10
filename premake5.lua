include "Libraries.lua"

workspace "Lucy"
    architecture "x86_64"
    startproject "LucyEditor"

    configurations {
        "Debug",
        "Release"
    }

    multiprocessorcompile "On"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Libraries"
    include "LucyEngine/ThirdParty/GLFW"
    include "LucyEngine/ThirdParty/ImGui"
    include "LucyEngine/ThirdParty/glm"
    include "LucyEngine/ThirdParty/stb"
    include "LucyEngine/ThirdParty/meshoptimizer"
    include "LucyEditor/ThirdParty/ImGuizmo"
group ""
include "LucyEngine"
include "LucyEditor"