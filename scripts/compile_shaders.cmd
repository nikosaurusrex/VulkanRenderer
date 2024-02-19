for %%f in (Renderer\Assets\Shaders\*.vert) do (
    %VULKAN_SDK%\bin\glslc "%%f" -o Renderer\Assets\Shaders\%%~nf.vert.spv
)

for %%f in (Renderer\Assets\Shaders\*.frag) do (
    %VULKAN_SDK%\bin\glslc "%%f" -o Renderer\Assets\Shaders\%%~nf.frag.spv
)