#include "Camera.h"

#include <math.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Window.h"
#include "Input.h"

FPSCamera::FPSCamera(f32 fov, f32 aspect, f32 near, f32 far) : fov(fov), aspect(aspect), near(near), far(far) {
	projection = glm::mat4();
	view = glm::mat4();
}

FPSCamera::~FPSCamera() {
}

void FPSCamera::Calculate(s32 width, s32 height) {
    aspect = (f32)width / (f32)height;  

    projection = glm::perspective(glm::radians(fov), aspect, near, far);

    ChangeOrientation();
}

void FPSCamera::ChangeOrientation() {
    glm::quat orientation = glm::quat(glm::vec3(glm::radians(-pitch), glm::radians(-yaw), 0.0f));
    
    view = glm::translate(glm::mat4(1.0f), position) * glm::toMat4(orientation);
    view = glm::inverse(view); 
}

void FPSCamera::Update(f32 delta) {
    f32 speed = 1.2f * delta;
	bool moving = false;

    if (Input::IsKeyDown(KeyCode::S)) {
        position += speed * glm::vec3(view[0][2], 0, view[2][2]);
		moving = true;
    }

    if (Input::IsKeyDown(KeyCode::W)) {
        position -= speed * glm::vec3(view[0][2], 0, view[2][2]);
		moving = true;
    }

    if (Input::IsKeyDown(KeyCode::D)) {
        position -= glm::normalize(glm::cross(glm::vec3(view[0][2], view[1][2], view[2][2]), glm::vec3(view[0][1], view[1][1], view[2][1]))) * speed;
		moving = true;
    }

    if (Input::IsKeyDown(KeyCode::A)) {
        position += glm::normalize(glm::cross(glm::vec3(view[0][2], view[1][2], view[2][2]), glm::vec3(view[0][1], view[1][1], view[2][1]))) * speed;
		moving = true;
    }

	if (!moving) {
		backup_y = position.y;
	}

	// View bobbing
	if (moving) {
		position.y += (f32) sin(glfwGetTime() * 10.0f) * delta * 0.3f;
	} else {
		position.y = backup_y;
	}

	/*
    if (Input::IsKeyDown(KeyCode::KEY_SPACE)) {
        position += glm::vec3(0.0f, 1.0f, 0.0f) * speed;
    }
    
    if (Input::IsKeyDown(KeyCode::KEY_LEFT_SHIFT)) {
        position -= glm::vec3(0.0f, 1.0f, 0.0f) * speed;
    }
	*/

    f32 dx = (f32) Input::delta_mouse_pos.x;
    f32 dy = (f32) Input::delta_mouse_pos.y;

    yaw    += dx * m_yaw * sensitivity;
    pitch  += dy * m_pitch * sensitivity;

    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    ChangeOrientation();
}

FreeCamera::FreeCamera(glm::vec3 lookat) : lookat(lookat) {
	projection = glm::mat4();
	view = glm::mat4();
}

FreeCamera::~FreeCamera() {
}

void FreeCamera::Calculate(s32 width, s32 height) {
	f32 aspect = (f32)width / (f32)height;
	f32 fov = 45.0f;
	f32 near = 0.1f;
	f32 far = 100.0f;

	projection = glm::perspective(glm::radians(fov), aspect, near, far);
	projection[1][1] *= -1;
	UpdateView();
}

void FreeCamera::Update(Window *window, f32 delta) {
    f32 dx = (f32) Input::delta_mouse_pos.x;
    f32 dy = (f32) Input::delta_mouse_pos.y;

	bool right_click = Input::IsButtonDown(MouseButton::Right);
	bool shift = Input::IsKeyDown(KeyCode::LeftShift);

	if (Input::GetButton(MouseButton::Right) == InputState::Pressed) {
		window->SetHideCursor(true);
	}

	if (Input::GetButton(MouseButton::Right) == InputState::Released) {
		window->SetHideCursor(false);
	}

	bool moved = false;

	if (Input::scroll != 0) {
		radius -= (f32) Input::scroll;

		if (radius < 2) radius = 2;
		if (radius > 30) radius = 30;

		moved = true;
	}

	if (right_click) {
		if (shift) {
			f32 move_speed = radius / 600.0f;

			lookat.x -= dx * sin(yaw) * move_speed;
			lookat.x -= dy * cos(yaw) * move_speed;

			lookat.z += dx * cos(yaw) * move_speed;
			lookat.z -= dy * sin(yaw) * move_speed;
		} else {
			yaw    += dx * sensitivity;
			pitch  += dy * sensitivity;

			yaw = fmod(yaw, 2 * PI);
			if (pitch < 0.2f) pitch = 0.2f;
			if (pitch > PI / 2.0f - 0.3f) pitch = PI / 2.0f - 0.3f;
		}

		moved = true;
	}

	if (moved) {
		UpdateView();
	}
}

void FreeCamera::UpdateView() {
	f32 y = lookat.y + sin(pitch) * radius;
	f32 h = cos(pitch) * radius;
	
	f32 x = lookat.x + cos(yaw) * h;
	f32 z = lookat.z + sin(yaw) * h;

	view = glm::lookAt(glm::vec3(x, y, z), lookat, glm::vec3(0.0f, 1.0f, 0.0f));
}
