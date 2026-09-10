project "LucyEditor"
    location "."
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++23"
    staticruntime "off"

    targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
    objdir ("../bin-obj/" .. outputdir .. "/%{prj.name}")

    files {
        "Source/**.h",
        "Source/**.hpp",
        "Source/**.cpp",

        "%{LibraryPath.Tracy}/public/TracyClient.cpp",
    }

    includedirs {
        "%{LibraryPath.spdlog}/include",
        "%{LibraryPath.GLFW}/include",
        "%{LibraryPath.Glad}/include",
        "%{LibraryPath.entt}/include",
        "%{LibraryPath.ImGui}",
        "%{LibraryPath.glm}",
        "%{LibraryPath.assimp}/include",
        "%{LibraryPath.ImGuizmo}/include",
        "%{LibraryPath.VulkanInclude}",
        "%{LibraryPath.Tracy}/public",
        "../LucyEngine/Source"
    }

    links {
        "ImGuizmo",
        "LucyEngine"
    }

    filter "system:windows"
        systemversion "latest"

        defines {
            "LUCY_WINDOWS"
        }

        postbuildcommands {
            "{COPY} %{LibraryPath.nativefiledialog}/nfd.lib ../bin/" .. outputdir .. "/%{prj.name}"
        }
    
    filter "configurations:Debug"
        defines {
            "LUCY_DEBUG",
            "GLFW_INCLUDE_NONE",
            "TRACY_ENABLE"
        }
        postbuildcommands {
            "{COPY} %{LibraryPath.assimp}/assimp-vc143-mtd.dll ../bin/" .. outputdir .. "/%{prj.name}",
        }
        symbols "On"
        runtime "Debug"

    filter "configurations:Release"
        defines {
            "LUCY_RELEASE",
            "GLFW_INCLUDE_NONE"
        }
        postbuildcommands {
            "{COPY} %{LibraryPath.assimp}/assimp-vc143-mt.dll ../bin/" .. outputdir .. "/%{prj.name}",
        }
        symbols "On"
        optimize "On"
        runtime "Release"