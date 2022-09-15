project "LucyEditor"
    location "."
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"

    targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
    objdir ("../bin-obj/" .. outputdir .. "/%{prj.name}")

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
        "%{LibraryPath.Optick}/include",
        "%{LibraryPath.ImGui}",
        "%{LibraryPath.glm}",
        "%{LibraryPath.assimp}/include",
        "%{LibraryPath.ImGuizmo}/include",
        "%{LibraryPath.VulkanInclude}",
        "%{LibraryPath.VMA}",
        "../LucyEngine/Source"
    }

    links {
        "ImGuizmo",
        "LucyEngine"
    }

    filter "platforms:win64"
        staticruntime "On"
        systemversion "latest"

        defines {
            "LUCY_WINDOWS"
        }

        postbuildcommands {
            "{COPY} %{LibraryPath.assimp}/assimp-vc143-mt.dll ../bin/" .. outputdir .. "/%{prj.name}",
            "{COPY} %{LibraryPath.nativefiledialog}/nfd.lib ../bin/" .. outputdir .. "/%{prj.name}"
        }
    
    filter "configurations:Debug"
        defines {
            "LUCY_DEBUG",
            "GLFW_INCLUDE_NONE"
        }
        symbols "On"
        runtime "Debug"

    filter "configurations:Release"
        defines {
            "LUCY_RELEASE",
            "GLFW_INCLUDE_NONE"
        }
        symbols "On"
        optimize "On"
        runtime "Release"