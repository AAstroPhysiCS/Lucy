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
        "vendor/assimp/include"
    }

    links {
        "GLFW",
        "Glad",
        "ImGui",
        "glm",
        "assimp"
    }

    filter "platforms:win64"
        cppdialect "C++17"
        staticruntime "On"
        systemversion "latest"

        defines {
           "LUCY_WINDOWS"
        }

        postbuildcommands {
            "{COPY} vendor/assimp/assimp.lib vendor/bin/" .. outputdir .. "/assimp/"
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