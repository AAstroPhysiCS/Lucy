project "glm"
    location "."
    kind "StaticLib"
    language "C++"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-obj/" .. outputdir .. "/%{prj.name}")

    files {
        "glm/**.h",
        "glm/**.hpp",
        "glm/**.cpp"
    }

    includedirs {
        "glm"
    }

    filter "system:windows"
        cppdialect "C++17"
        staticruntime "On"
        systemversion "latest"

    filter "configurations:Debug"
        defines "LUCY_DEBUG"
        symbols "On"

    filter "configurations:Release"
        defines "LUCY_RELEASE"
        symbols "On"
        optimize "On"