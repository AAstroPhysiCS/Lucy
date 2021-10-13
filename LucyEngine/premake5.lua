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
        "vendor/stb/include"
    }

    links {
        "GLFW",
        "Glad",
        "ImGui",
        "glm"
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