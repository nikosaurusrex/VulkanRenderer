#include "MasterRenderer.h"

MasterRenderer::MasterRenderer(RenderPass *render_pass) : render_pass(render_pass) {
    RenderStats::Create();
}

MasterRenderer::~MasterRenderer() {
    RenderStats::Destroy();

    render_images.Destroy();
}

VkCommandBuffer MasterRenderer::Begin() {
    cmd_buf = render_pass->BeginFrame(&render_images);

    RenderStats::Begin(cmd_buf);

    render_pass->Begin(&render_images);

    return cmd_buf;
}

void MasterRenderer::End() {
    render_pass->End(&render_images);

    RenderStats::EndGPU(cmd_buf);

    render_pass->EndFrame();

    RenderStats::EndCPU();
}
