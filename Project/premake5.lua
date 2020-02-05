workspace "Project"
    startproject "Project"
	warnings "Extra"

	outputdir = "x64/Debug"

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

    project "Project"
        kind "ConsoleApp"
        language "C++"
        cppdialect "C++17"
		systemversion "latest"
        staticruntime "on"
        location "Project"
        architecture "x64"

		targetdir 	(outputdir)
		objdir 		(outputdir .. "/bin-int")

        files
        {
            "Project/**.h",
            "Project/**.cpp",
        }

        sysincludedirs
        {
            "%{wks.location.abspath}",
            "GLEW/include",
            "include",
            "SDL/include",
            "C:/VulkanSDK/1.1.108.0/Include",
			"C:/VulkanSDK/1.1.130.0/Include",
            "D:/VulkanSDK/1.1.108.0/Include",
			"D:/VulkanSDK/1.1.130.0/Include",
        }

        libdirs
        {
            "C:/VulkanSDK/1.1.108.0/Lib",
			"C:/VulkanSDK/1.1.130.0/Lib",
            "D:/VulkanSDK/1.1.108.0/Lib",
			"D:/VulkanSDK/1.1.130.0/Lib",
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

