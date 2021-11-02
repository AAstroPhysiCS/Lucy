project "LucyEngine"
    location "."
    kind "StaticLib"
    language "C++"

    targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
    objdir ("../bin-obj/" .. outputdir .. "/%{prj.name}")

    files {
        "src/**.h",
        "src/**.hpp",
        "src/**.cpp"
    }

    includedirs {
        "vendor/spdlog/include",
        "vendor/GLFW/include",
        "vendor/Glad/include",
        "vendor/entt/include",
        "vendor/glm",
        "vendor/stb/include",
        "vendor/assimp/include",
        "vendor/nativefiledialog/include"
    }

    links {
        "GLFW",
        "Glad",
        "ImGui",
        "glm",
        "vendor/assimp/assimp.lib",
        "vendor/nativefiledialog/nfd.lib"
    }

    filter "platforms:win64"
        cppdialect "C++17"
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

    filter "configurations:Release"
        defines {
            "LUCY_RELEASE",
            "GLFW_INCLUDE_NONE"
        }
        symbols "On"
        optimize "On"