include "Libraries.lua"

workspace "Lucy"
    architecture "x64"
    startproject "LucyEditor"

    configurations {
        "Debug",
        "Release"
    }

    platforms {
        "win64"
    }

    flags {
        "MultiProcessorCompile"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Libraries"
    include "LucyEngine/ThirdParty/Glad"
    include "LucyEngine/ThirdParty/GLFW"
    include "LucyEngine/ThirdParty/ImGui"
    include "LucyEngine/ThirdParty/glm"
    include "LucyEngine/ThirdParty/stb"
    include "LucyEditor/ThirdParty/ImGuizmo"
group ""
include "LucyEngine"
include "LucyEditor"