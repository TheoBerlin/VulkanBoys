workspace "gl_Testbench"
    startproject "gl_Testbench"
	warnings "Extra"

	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

    configurations
    {
        "Debug",
        "Release"
    }

	platforms
	{
        "x64",
    }
    
	filter "configurations:Debug"
		symbols "On"
		runtime "Debug"
		defines 
		{
			"DEBUG",
		}
	filter "configurations:Release"
		symbols "On"
		runtime "Release"
		optimize "Full"
		defines 
		{ 
			"NDEBUG",
		}
	filter {}

    project "gl_Testbench"
        kind "ConsoleApp"
        language "C++"
        cppdialect "C++17"
		systemversion "latest"
        staticruntime "on"
        location "gl_Testbench"
        architecture "x64"

		targetdir 	("Build/bin/" .. outputdir .. "/%{prj.name}")
		objdir 		("Build/bin-int/" .. outputdir .. "/%{prj.name}")

        files
        {
            "gl_Testbench/**.h",
            "gl_Testbench/**.cpp",
        }

        sysincludedirs
        {
            "%{wks.location.abspath}",
            "GLEW/include",
            "include",
            "SDL/include",
            "C:/VulkanSDK/1.1.108.0/Include",
			"C:/VulkanSDK/1.1.130.0/Include",
        }

        libdirs
        {
            "C:/VulkanSDK/1.1.108.0/Lib",
			"C:/VulkanSDK/1.1.130.0/Lib",
            "GLEW/lib/Release/64",
            "SDL/lib/64",
        }

        links
        {
            "SDL2",
            "glew32",
            "vulkan-1",
        }
    project "*"

