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
        "../LucyEngine/vendor/Glad/include"
    }

    links {
        "LucyEngine"
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
