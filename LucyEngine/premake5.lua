project "LucyEngine"
    location "."
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"

    targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
    objdir ("../bin-obj/" .. outputdir .. "/%{prj.name}")

    pchheader "lypch.h"
    pchsource "lypch.cpp"

    files {
        "Source/**.h",
        "Source/**.hpp",
        "Source/**.cpp"
    }

    includedirs {
        "%{LibraryPath.spdlog}/include",
        "%{LibraryPath.GLFW}/include",
        "%{LibraryPath.Glad}/include",
        "%{LibraryPath.entt}/include",
        "%{LibraryPath.glm}",
        "%{LibraryPath.stb}/include",
        "%{LibraryPath.assimp}/include",
        "%{LibraryPath.nativefiledialog}/include",
        "%{LibraryPath.VulkanInclude}",
        "%{LibraryPath.VMA}",
        "Source"
    }

    links {
        "GLFW",
        "Glad",
        "ImGui",
        "glm",
        
        "%{LibraryPath.assimp}/assimp.lib",
        "%{LibraryPath.nativefiledialog}/nfd.lib"
    }

    filter "platforms:win64"
        staticruntime "On"
        systemversion "latest"

        defines {
           "LUCY_WINDOWS"
        }

    filter "configurations:Debug"
        defines {
            "LUCY_DEBUG",
            "GLFW_INCLUDE_NONE"
        }
        symbols "On"
        runtime "Debug"

        links {
            "%{LibraryPath.VulkanLib}/vulkan-1.lib",
            "%{LibraryPath.VulkanLib}/VkLayer_utils.lib",
            "%{LibraryPath.VMADebug}",

            "%{LibraryPath.ShaderCDebug}",
            "%{LibraryPath.SPIRVDebug}",
            "%{LibraryPath.SPIRVGLSLDebug}",
            "%{LibraryPath.SPIRVTools}"
        }

    filter "configurations:Release"
        defines {
            "LUCY_RELEASE",
            "GLFW_INCLUDE_NONE"
        }
        symbols "On"
        optimize "On"
        runtime "Release"

        links {
            "%{LibraryPath.VulkanLib}/vulkan-1.lib",
            "%{LibraryPath.VulkanLib}/VkLayer_utils.lib",
            "%{LibraryPath.VMARelease}",

            "%{LibraryPath.ShaderCRelease}",
            "%{LibraryPath.SPIRVRelease}",
            "%{LibraryPath.SPIRVGLSLRelease}"
        }