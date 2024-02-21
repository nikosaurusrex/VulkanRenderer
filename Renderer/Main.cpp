#include "Common.h"

#include "Core/Camera.h"
#include "Core/Input.h"
#include "Core/Sound.h"
#include "Core/Window.h"
#include "Engine.h"

#include "Vulkan/VulkanRenderer.h"
#include "Graphics/Model.h"
#include "Graphics/SceneRenderer.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

glm::mat4 TranslateScale(glm::vec3 trans, glm::vec3 scale) {
	return  glm::translate(glm::mat4(1.0f), trans) *
		glm::scale(glm::mat4(1.0f), scale);
}

glm::mat4 TranslateRotate(glm::vec3 trans, glm::vec3 rot) {
	return  glm::translate(glm::mat4(1.0f), trans) *
		glm::toMat4(glm::quat(glm::radians(rot)));
}

glm::mat4 TranslateRotateScale(glm::vec3 trans, glm::vec3 rot, glm::vec3 scale) {
	return  glm::translate(glm::mat4(1.0f), trans) *
		glm::toMat4(glm::quat(glm::radians(rot))) *
		glm::scale(glm::mat4(1.0f), scale);
}

struct Door {
	bool open = false;
	f32 open_degree = 0.0f;
	f32 target_degree = 0.0f;
	Model *model_wall_door;
	Model *model_door;
	Sound *sound_door_open;
	Sound *sound_door_close;

	Door(Model *model_wall_door, Model *model_door) : model_wall_door(model_wall_door), model_door(model_door) {
		model_wall_door->transformation = glm::toMat4(glm::quat(glm::radians(glm::vec3(0.0f, 0.0f, 0.0f))));
		model_door->transformation = glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, 0.50f));

        sound_door_open = new Sound("Renderer/Assets/Sounds/door_open.wav");
        sound_door_close = new Sound("Renderer/Assets/Sounds/door_close.wav");
	}

	~Door() {

		delete sound_door_open;
		delete sound_door_close;
	}

	void Render(SceneRenderer *renderer, f32 delta_time) {
		const f32 speed = 66.5f * delta_time;
		if (open) {
			open_degree += speed;
			if (open_degree > 90.0f) {
				open_degree = 90.0f;
			}
		} else {
			open_degree -= speed;
			if (open_degree < 0.0f) {
				open_degree = 0.0f;
			}
		}

		renderer->RenderModel(model_wall_door);
		renderer->RenderModel(model_door);
	}

	void OpenOrClose() {
		open = !open;
		if (open) {
			sound_door_open->Play();
		} else {
			sound_door_close->Play();
		}
	}
};

int main() {
    Engine engine;

    engine.window->EnableRawInput();
	engine.window->SetVSync(true);

    engine.Start();

	InitSound();

#ifdef VULKAN_RENDERER_DEBUG
    VulkanContext context = VulkanContext::Get(true);
#else
    VulkanContext context = VulkanContext::Get(false);
#endif

    VulkanInstance::Create(&context, engine.window->handle, "Engine");

    VulkanPhysicalDevice::Pick(&context);
    VulkanDevice::Create(&context);

    CreateVulkanAllocator();

    VulkanSwapchain swapchain;
    swapchain.Create(true);

    RenderPass render_pass;
    render_pass.Create(&swapchain);

	SceneRenderer *renderer = new SceneRenderer(&swapchain, &render_pass);

	Model *model_wall_door = ModelImporter::Load("Renderer/Assets/Models/village/Stucco_Doorway_Wide_Tall.obj", render_pass.graphics_command_pool.handle);
	Model *model_door = ModelImporter::Load("Renderer/Assets/Models/village/Wall_Prop_Door_Ornate.obj", render_pass.graphics_command_pool.handle);
	Model *model_floor = ModelImporter::Load("Renderer/Assets/Models/village/Stone_Floor_2.obj", render_pass.graphics_command_pool.handle);
	Model *model_waterwheel = ModelImporter::Load("Renderer/Assets/Models/village/Waterwheel_1.obj", render_pass.graphics_command_pool.handle);
	Model *model_well = ModelImporter::Load("Renderer/Assets/Models/village/Prop_Well_1.obj", render_pass.graphics_command_pool.handle);
	Model *model_wall_window = ModelImporter::Load("Renderer/Assets/Models/village/Kit_Window_Upper_Straight.obj", render_pass.graphics_command_pool.handle);

	model_well->transformation = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 2.0f));

	SceneData scene_data;

	FreeCamera camera(glm::vec3(0.0));
	camera.Calculate(engine.window->bounds.width, engine.window->bounds.height);

	DirectionalLight dir_light;
	dir_light.dir = glm::vec3(0.3f, 1.0f, 0.3f);
	dir_light.ambient = glm::vec4(0.2f);
	dir_light.diffuse = glm::vec4(1.0f);
	scene_data.dir_light = dir_light;

	PointLight point_light;
	point_light.pos = glm::vec3(-10.0f, 2.0f, -10.0f);
	point_light.ambient = glm::vec4(0.2f);
	point_light.diffuse = glm::vec4(1.0f);

	scene_data.point_lights[0] = point_light;
	scene_data.num_point_lights = 0;
    scene_data.projection = camera.projection;
    scene_data.view = camera.view;
    renderer->SetSceneData(&scene_data);

	bool show_editor = false;
	bool show_render_stats = false;
	bool wireframe = false;

	f64 last_time = glfwGetTime();
	f32 delta_time = 0.0f;

	f64 waterwheel_angle = 0.0f;

	Door door(model_wall_door, model_door);

    while (engine.running) {
        while (!engine.events.empty()) {
            Event event = engine.events.front();
            engine.events.pop();

            switch (event.type) {
				case Event::Resize: {
					// framebuffer.Resize(event.width, event.height);
					// font_renderer.Resize(event.width, event.height);
					camera.Calculate(event.width, event.height);
				} break;
                case Event::Key: {
					if (event.action == GLFW_RELEASE) {
						if (event.button == (int)KeyCode::E) {
							door.OpenOrClose();
						}
						if (event.button == (int)KeyCode::F11) {
							engine.window->ToggleFullscreen();
						}
						if (event.button == (int)KeyCode::F3) {
							show_render_stats = !show_render_stats;
						}
						if (event.button == (int)KeyCode::F4) {
							show_editor = !show_editor;
							if (show_editor) {
								Input::Lock();
								engine.window->SetHideCursor(false);
							} else {
								Input::Unlock();
								engine.window->SetHideCursor(true);
							}
						}
					}
                } break;
            }
        }

		if (Input::IsKeyDown(KeyCode::Q) && Input::IsKeyDown(KeyCode::LeftControl)) {
			engine.running = false;
		}

		f64 current_time = glfwGetTime();
		delta_time = f32(current_time - last_time);
		last_time = current_time;

        bool camera_moved = camera.Update(engine.window, delta_time);
		Input::Update(engine.window);
        engine.Update();

		renderer->Begin();

        if (camera_moved) {
            scene_data.projection = camera.projection;
            scene_data.view = camera.view;
		    renderer->SetSceneData(&scene_data);
        }
		renderer->RenderModel(model_well);

		model_waterwheel->transformation = TranslateRotateScale(glm::vec3(2.0f, 1.0f, -2.0f), glm::vec3(waterwheel_angle, 0.0f, 0.0f), glm::vec3(0.5f));
		renderer->RenderModel(model_waterwheel);
        waterwheel_angle += 10.0f * delta_time;

        glm::mat4 wtr = TranslateRotate(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, -90.0f, 0.0f));
		model_wall_window->transformation = wtr;
		renderer->RenderModel(model_wall_window);

        for (int x = 1; x < 5; x++) {
            for (int z = -3; z < 7; z++) {
				model_floor->transformation = glm::translate(glm::mat4(1.0f), glm::vec3(x, 0.0f, z));
				renderer->RenderModel(model_floor);
            }
        }

        door.Render(renderer, delta_time);

		renderer->End();

		RenderStats::SetTitle(engine.window->handle);
    }

    VK_CHECK(vkDeviceWaitIdle(VulkanDevice::handle));

	delete model_wall_door;
	delete model_door;
	delete model_floor;
	delete model_waterwheel;
	delete model_well;
	delete model_wall_window;
	delete renderer;

    render_pass.Destroy();

    swapchain.Destroy();

    DestroyVulkanAllocator();

    VulkanDevice::Destroy();
    VulkanInstance::Destroy();

	DeinitSound();

	return 0;
}
