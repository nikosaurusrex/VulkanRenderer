for shader in Renderer/Assets/Shaders/*
do
    $VULKAN_SDK/bin/glslc $shader -o ${shader%.shader}.spv
done