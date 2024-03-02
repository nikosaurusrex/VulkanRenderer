# Vulkan Renderer
Learning Vulkan

## Setup
### MacOS and Linux
Install dependencies
> premake5 gmake \
> make

### Windows
set library paths in premake5.lua
> premake5 vs2022 \
> build in Visual Studio


## Todo
* Font Renderer
* Images
* Shadows
* MSAA
* Indirect Rendering
* Culling
* PBR
* Mesh Shaders?
* Raytracing?
* ImGui?
* MacOS Support
* Assets

## Dependencies
* Assimp (needs to be installed)
* GLFW (needs to be installed)
* Freetype (needs to be installed)
* VulkanSDK (installation and path variable VULKAN_SDK set)
* GLM
* stb_image.h
* miniaudio.h
* VulkanMemoryAllocator

## Assets
Not possible to include right now because of copyright concerns - will be changed


## Credits
Helpful resources and tutorials
* [VulkanTutorial](https://vulkan-tutorial.com/)
* [Niagara Youtube Series by Arseny Kapoulkine](https://www.youtube.com/watch?v=BR2my8OE1Sc&t=7727s&ab_channel=ArsenyKapoulkine)