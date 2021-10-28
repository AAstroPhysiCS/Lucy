project "assimp"
    location "."
    kind "StaticLib"
    language "C++"

    targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
    objdir ("../bin-obj/" .. outputdir .. "/%{prj.name}")

    files {
        "include/**.h",
        "include/**.hpp",
    }

    links {
        "assimp-vc142-mt.dll"
    }

    includedirs {
        "include"
    }
