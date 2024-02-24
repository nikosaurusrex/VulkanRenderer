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

        libdirs {
            "%{VULKAN_SDK}/lib"
        }

        filter "system:windows"
            postbuildcommands {
                "call ./scripts/compile_shaders.cmd"
            }

            links {
                "opengl32.lib",
                "assimp.lib",
                "glfw3.lib",
                "vulkan-1.lib"
            }

            libdirs {
                "vendor/assimp/libs",
                "vendor/glfw/libs"
            }

            includedirs {
                "vendor/assimp/include",
                "vendor/glfw/include"
            }

        filter "system:linux"
            local shell_path = os.getenv("SHELL") or "/bin/sh"

            postbuildcommands {
                shell_path .. " ./scripts/compile_shaders.sh"
            }

            links {
                "glfw",
                "vulkan",
                "assimp"
            }

        filter "system:macosx"
            local glfw3_path = io.popen('brew --prefix glfw'):read('*a')
            glfw3_path = glfw3_path:gsub('%s+$', '')

            local assimp_path = io.popen('brew --prefix assimp'):read('*a')
            assimp_path = assimp_path:gsub('%s+$', '')

            local shell_path = os.getenv("SHELL") or "/bin/sh"

            postbuildcommands {
                shell_path .. " ./scripts/compile_shaders.sh"
            }

            libdirs {
                glfw3_path .. "/lib",
                assimp_path .. "/lib"
            }

            includedirs {
                glfw3_path .. "/include",
                assimp_path .. "/include"
            }

            links {
                "glfw",
                "vulkan",
                "assimp"
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
