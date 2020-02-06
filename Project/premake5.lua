workspace "Vulkan-Project"
	architecture "x64"
	startproject "VulkanProject"
	warnings "Extra"

	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

	configurations 
	{
		"Debug", 
		"Release",
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
			"_DEBUG",
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

	group "Dependencies"
		include "Dependencies/glfw"
	group ""

    project "VulkanProject"
        language "C++"
        cppdialect "C++17"
        systemversion "latest"
        staticruntime "on"
        kind "ConsoleApp"

		targetdir 	("Build/bin/" .. outputdir .. "/%{prj.name}")
		objdir 		("Build/bin-int/" .. outputdir .. "/%{prj.name}")

        files 
		{ 
			"src/**.h",
			"src/**.hpp",
			"src/**.c",
			"src/**.cpp",
		}

		filter "system:windows"
			links
			{
                "vulkan-1",
			}
			libdirs
			{
                "C:/VulkanSDK/1.1.108.0/Lib",
                "C:/VulkanSDK/1.1.130.0/Lib",
                "D:/VulkanSDK/1.1.108.0/Lib",
				"D:/VulkanSDK/1.1.130.0/Lib",
			}
			sysincludedirs
			{
                "C:/VulkanSDK/1.1.108.0/Include",
                "C:/VulkanSDK/1.1.130.0/Include",
                "D:/VulkanSDK/1.1.108.0/Include",
				"D:/VulkanSDK/1.1.130.0/Include",
			}
			
			prebuildcommands
			{ 
				"compile_shaders" 
			}
		filter { "system:macosx" }
			links
			{
				"vulkan.1",
				"vulkan.1.1.121",
				"Cocoa.framework",
				"OpenGL.framework",
				"IOKit.framework",
				"CoreFoundation.framework"
			}
			libdirs
			{
				"/usr/local/lib",
			}
			sysincludedirs
			{
				"/usr/local/include",
			}

			--prebuildcommands 
			--{ 
				--"./compile_shaders.command" 
			--}
		filter { "action:vs*" }
			defines
			{
				"_CRT_SECURE_NO_WARNINGS"
			}
		filter {}

		includedirs
		{
			"src",
        }
        
		sysincludedirs
		{
			"Dependencies/stb",
			"Dependencies/GLFW/include",
			"Dependencies/glm",
		}
		
		links 
		{ 
			"GLFW",
		}
    project "*"

