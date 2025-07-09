project "LucyEngine"
    location "."
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
    objdir ("../bin-obj/" .. outputdir .. "/%{prj.name}")

    pchheader ("lypch.h")
    pchsource ("%{wks.location}/LucyEngine/Source/lypch.cpp")

    files {
        "Source/**.h",
        "Source/**.hpp",
        "Source/**.cpp",
    }

    includedirs {
        "%{LibraryPath.spdlog}/include",
        "%{LibraryPath.GLFW}/include",
        "%{LibraryPath.entt}/include",
        "%{LibraryPath.stb}/include",
        "%{LibraryPath.assimp}/include",
        "%{LibraryPath.nativefiledialog}/include",
        "%{LibraryPath.glm}",
        "%{LibraryPath.VulkanInclude}",
        "%{LibraryPath.Tracy}/public",
        "Source"
    }

    links {
        "GLFW",
        "ImGui",
        "glm",
        
        "%{LibraryPath.assimp}/assimp.lib",
        "%{LibraryPath.nativefiledialog}/nfd.lib"
    }

    filter "platforms:win64"
        systemversion "latest"

        defines {
           "LUCY_WINDOWS"
        }

    filter "configurations:Debug"
        defines {
            "LUCY_DEBUG",
            "GLFW_INCLUDE_NONE",
            "TRACY_ENABLE"
        }
        symbols "On"
        runtime "Debug"

        links {
            "%{LibraryPath.VulkanLib}/vulkan-1.lib",

            "%{LibraryPath.SlangDebug}",
            "%{LibraryPath.SlangDebugRT}",

            "%{LibraryPath.ShaderCDebug}",
            "%{LibraryPath.SPIRVDebug}",
            "%{LibraryPath.SPIRVGLSLDebug}",
            "%{LibraryPath.SPIRVTools}"
        }

    filter "configurations:Release"
        defines {
            "LUCY_RELEASE",
            "GLFW_INCLUDE_NONE",
        }
        symbols "On"
        optimize "On"
        runtime "Release"

        links {
            "%{LibraryPath.VulkanLib}/vulkan-1.lib",

            "%{LibraryPath.SlangRelease}",
            "%{LibraryPath.SlangReleaseRT}",

            "%{LibraryPath.ShaderCRelease}",
            "%{LibraryPath.SPIRVRelease}",
            "%{LibraryPath.SPIRVGLSLRelease}"
        }