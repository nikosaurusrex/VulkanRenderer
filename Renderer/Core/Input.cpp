#include "Input.h"

#include "Window.h"

InputState  Input::keys[MAX_KEYS] = {};
InputState  Input::buttons[MAX_BUTTONS] = {};
MousePos 	Input::mouse_pos = {};
MousePos 	Input::delta_mouse_pos = {};
double 		Input::scroll = 0;
bool 		Input::locked = false;

void Input::Init(Window *window) {
	window->GetCursorPos(&mouse_pos.x, &mouse_pos.y);
	delta_mouse_pos.x = 0.0;
	delta_mouse_pos.y = 0.0;
	scroll = 0;
	locked = false;
}

void Input::Update(Window *window) {
	scroll = 0;

	if (locked) {
		delta_mouse_pos.x = 0.0;
		delta_mouse_pos.y = 0.0;
		return;
	}

	f64 xpos, ypos;
	window->GetCursorPos(&xpos, &ypos);

	delta_mouse_pos.x = xpos - mouse_pos.x;
	delta_mouse_pos.y = ypos - mouse_pos.y;

	mouse_pos.x = xpos;
	mouse_pos.y = ypos;

	for (int i = 0; i < MAX_KEYS; i++) {
		int action = glfwGetKey(window->handle, i);
		if (keys[i] == InputState::None && action == GLFW_PRESS) {
			keys[i] = InputState::Pressed;
		} else if (keys[i] == InputState::Pressed) {
			keys[i] = InputState::Held;
		} else if (keys[i] == InputState::Held && action == GLFW_RELEASE) {
			keys[i] = InputState::Released;
		} else if (keys[i] == InputState::Released) {
			keys[i] = InputState::None;
		}
	}

	for (int i = 0; i < MAX_BUTTONS; i++) {
		int action = glfwGetMouseButton(window->handle, i);
		if (buttons[i] == InputState::None && action == GLFW_PRESS) {
			buttons[i] = InputState::Pressed;
		} else if (buttons[i] == InputState::Pressed) {
			buttons[i] = InputState::Held;
		} else if (buttons[i] == InputState::Held && action == GLFW_RELEASE) {
			buttons[i] = InputState::Released;
		} else if (buttons[i] == InputState::Released) {
			buttons[i] = InputState::None;
		}
	}
}

void Input::UpdateScroll(double offset) {
	if (locked) {
		return;
	}

	scroll = offset;
}

bool Input::IsKeyDown(KeyCode key) {
	if (locked) {
		return false;
	}
	InputState state = keys[(u16)key];

	return state == InputState::Pressed || state == InputState::Held;
}

bool Input::IsButtonDown(MouseButton button) {
	if (locked) {
		return false;
	}
	InputState state = buttons[(u8)button];

	return state == InputState::Pressed || state == InputState::Held;
}

InputState Input::GetKey(KeyCode key) {
	if (locked) {
		return InputState::None;
	}
	return keys[(u16)key];
}

InputState Input::GetButton(MouseButton button) {
	if (locked) {
		return InputState::None;
	}
	return buttons[(u8)button];
}

void Input::Lock() {
	locked = true;
}

void Input::Unlock() {
	locked = false;
}
