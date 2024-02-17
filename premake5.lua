workspace "VulkanRenderer"
    architecture "x64"
    configurations
    { 
        "Debug",
        "Release",
        "Dist"
    }
    startproject "VulkanRenderer"

    flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
VULKAN_SDK = os.getenv("VULKAN_SDK")

project "VulkanRenderer"
    kind "ConsoleApp"
        language "C++"
        cppdialect "C++20"
        staticruntime "off"

        targetdir ("bin/" .. outputdir)
        objdir ("bin/" .. outputdir .. "/temp")

        files
        {
            "Renderer/**.h",
            "Renderer/**.cpp",
            "Renderer/Core/**.h",
            "Renderer/Core/**.cpp",
            "Renderer/Graphics/**.h",
            "Renderer/Graphics/**.cpp",
            "Renderer/Vulkan/**.h",
            "Renderer/Vulkan/**.cpp"
        }

        includedirs
        {
            "Renderer",
            "vendor/glm",
            "vendor/sh_libs",
            "%{VULKAN_SDK}/include",
        }

        defines
        {
            "GLFW_INCLUDE_NONE"
        }

        links {
            "glfw",
            "vulkan",
            "assimp"
        }

        libdirs {
            "%{VULKAN_SDK}/lib"
        }

        filter "system:windows"
            postbuildcommands {
                "./scripts/compile_shaders.cmd"
            }

        filter "system:linux"
            postbuildcommands {
                "bash ./scripts/compile_shaders.sh"
            }

        filter "configurations:Debug"
            defines "VULKAN_RENDERER_DEBUG"
            runtime "Debug"
            symbols "on"

        filter "configurations:Release"
            defines "VULKAN_RENDERER_RELEASE"
            runtime "Release"
            optimize "on"
            
        filter "configurations:Dist"
            defines "VULKAN_RENDERER_DIST"
            runtime "Release"
            optimize "on"