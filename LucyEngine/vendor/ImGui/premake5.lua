project "ImGui"
    location "."
    kind "StaticLib"
    language "C++"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-obj/" .. outputdir .. "/%{prj.name}")

    files {
        "./**.cpp",
        "./**.h"
    }

    includedirs {
        ".",
        "../GLFW/include",
        "../glad/include"
    }

    filter "system:windows"
        cppdialect "C++17"
        staticruntime "On"
        systemversion "latest"

        defines {
            LUCY_WINDOWS
        }

    filter "configurations:Debug"
        defines "LUCY_DEBUG"
        symbols "On"

    filter "configurations:Release"
        defines "LUCY_RELEASE"
        symbols "On"
        optimize "On"