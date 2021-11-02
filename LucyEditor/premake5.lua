project "LucyEditor"
    location "."
    kind "ConsoleApp"
    language "C++"

    targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
    objdir ("../bin-obj/" .. outputdir .. "/%{prj.name}")

    files {
        "src/**.h",
        "src/**.cpp"
    }

    includedirs {
        "../LucyEngine/vendor/spdlog/include",
        "../LucyEngine/src",
        "../LucyEngine/vendor/GLFW/include",
        "../LucyEngine/vendor/Glad/include",
        "../LucyEngine/vendor/ImGui",
        "../LucyEngine/vendor/glm",
        "../LucyEngine/vendor/entt/include",
        "../LucyEngine/vendor/assimp/include"
    }

    links {
        "LucyEngine"
    }

    filter "platforms:win64"
        cppdialect "C++17"
        staticruntime "On"
        systemversion "latest"

        defines {
            "LUCY_WINDOWS"
        }

        postbuildcommands {
            "{COPY} ../LucyEngine/vendor/assimp/assimp-vc142-mt.dll ../bin/" .. outputdir .. "/%{prj.name}",
            "{COPY} ../LucyEngine/vendor/nativefiledialog/nfd.lib ../bin/" .. outputdir .. "/%{prj.name}"
        }
    
    filter "configurations:Debug"
        defines {
            "LUCY_DEBUG",
            "GLFW_INCLUDE_NONE"
        }
        symbols "On"

    filter "configurations:Release"
        defines {
            "LUCY_RELEASE",
            "GLFW_INCLUDE_NONE"
        }
        symbols "On"
        optimize "On"
