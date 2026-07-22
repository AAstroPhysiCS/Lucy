project "meshoptimizer"
	location "."
	kind "StaticLib"
	language "C++"
	cppdialect "C++11"
    staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-obj/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/meshoptimizer.h",
		"src/**.cpp"
	}

	filter "system:windows"
		systemversion "latest"
		staticruntime "On"

		defines
		{
			"_CRT_SECURE_NO_WARNINGS"
		}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		runtime "Release"
		optimize "On"