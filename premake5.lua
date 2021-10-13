workspace "LucyEngine"
    architecture "x64"
    startproject "LucyEditor"

    configurations {
        "Debug",
        "Release"
    }

    flags {
        "MultiProcessorCompile"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "LucyEngine/vendor/Glad"
include "LucyEngine/vendor/GLFW"
include "LucyEngine/vendor/ImGui"
include "LucyEngine/vendor/glm"
include "LucyEngine/vendor/stb"
include "LucyEngine"
include "LucyEditor"