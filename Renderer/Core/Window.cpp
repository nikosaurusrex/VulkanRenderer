#include "Window.h"

#include "Input.h"
#include "Engine.h"

static void FramebufferSizeCallback(GLFWwindow *window, int nwidth, int nheight) {
    Engine *engine = static_cast<Engine *>(glfwGetWindowUserPointer(window));

    Event resize_event;
    resize_event.type = Event::Resize;
    resize_event.width = nwidth;
    resize_event.height = nheight;
    
    engine->HandleEvent(resize_event);
}

static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    Engine *engine = static_cast<Engine *>(glfwGetWindowUserPointer(window));

    Event key_event;
    key_event.type = Event::Key;
    key_event.button = key;
    key_event.mods = mods;
    key_event.action = action;
    
    engine->HandleEvent(key_event);
}

static void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
    Engine *engine = static_cast<Engine *>(glfwGetWindowUserPointer(window));

    Event e;
    e.type = Event::MouseButton;
    e.action = action;
    e.button = button;
    e.mods = mods;
    
    engine->HandleEvent(e);
}

static void CursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
    Engine *engine = static_cast<Engine *>(glfwGetWindowUserPointer(window));

    Event e;
    e.type = Event::MouseMove;
    e.xpos = xpos;
    e.ypos = ypos;
    
    engine->HandleEvent(e);
}

static void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
    Engine *engine = static_cast<Engine *>(glfwGetWindowUserPointer(window));

    Event e;
    e.type = Event::MouseScroll;
    e.xpos = xoffset;
    e.ypos = yoffset;
    
    engine->HandleEvent(e);

	Input::UpdateScroll(yoffset);
}

Window::Window(const char *title, int width, int height) : title(title) {
    if (!glfwInit()) {
        LogFatal("Failed to initialize GLFW");
    }

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    handle = glfwCreateWindow(width, height, title, 0, 0);
    if (!handle) {
        LogFatal("Failed to create GLFW window!");
    }

    const GLFWvidmode *vid_mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowPos(handle, vid_mode->width / 2 - width / 2, vid_mode->height / 2 - height / 2);
    
    glfwGetWindowPos(handle, &bounds.x, &bounds.y);
    glfwGetWindowSize(handle, &bounds.width, &bounds.height);

    vsync = true;
	glfwSwapInterval(1);

    glfwSetFramebufferSizeCallback(handle, FramebufferSizeCallback);
    glfwSetMouseButtonCallback(handle, MouseButtonCallback);
    glfwSetCursorPosCallback(handle, CursorPosCallback);
    glfwSetKeyCallback(handle, KeyCallback);
    glfwSetScrollCallback(handle, ScrollCallback);

    glfwShowWindow(handle);
}

Window::~Window() {
    Destroy();
}

void Window::Update() {
    glfwPollEvents();
}

void Window::ToggleFullscreen() {
    in_fullscreen = !in_fullscreen;
    
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    if (in_fullscreen) {
        SwitchToFullscreen(monitor, mode);
    } else {
        SwitchToWindowed(mode);
    }
}

void Window::SwitchToFullscreen(GLFWmonitor *monitor, const GLFWvidmode *mode) {
    glfwGetWindowPos(handle, &backup_bounds.x, &backup_bounds.y);
    glfwGetWindowSize(handle, &backup_bounds.width, &backup_bounds.height);
    
    glfwSetWindowMonitor(handle, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    glfwSwapInterval(vsync ? 1 : 0);
}

void Window::SwitchToWindowed(const GLFWvidmode *mode) {
    glfwSetWindowMonitor(handle, 0, backup_bounds.x, backup_bounds.y, backup_bounds.width, backup_bounds.height, mode->refreshRate);
    glfwSwapInterval(vsync ? 1 : 0);
}

void Window::SetVSync(bool vsync) {
    this->vsync = vsync;
    glfwSwapInterval(vsync ? 1 : 0);
}

void Window::Destroy() {
    glfwDestroyWindow(handle);
    glfwTerminate();
}

bool Window::ShouldClose() {
    return glfwWindowShouldClose(handle);
}

void Window::SetEngine(Engine *engine) {
    glfwSetWindowUserPointer(handle, engine);

	int width, height;
	glfwGetFramebufferSize(handle, &width, &height);

	Event resize_event;
	resize_event.type = Event::Resize;
	resize_event.width = width;
	resize_event.height = height;
	
	engine->HandleEvent(resize_event);
}

void Window::SetHideCursor(bool hide) {
    if (hide) {
        glfwSetInputMode(handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else {
        glfwSetInputMode(handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void Window::GetCursorPos(f64 *xpos, f64 *ypos) {
    glfwGetCursorPos(handle, xpos, ypos);
}

void Window::EnableRawInput() {
    bool raw_mouse_input = glfwRawMouseMotionSupported();
    if (raw_mouse_input) {
        glfwSetInputMode(handle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        LogInfo("Raw Input Enabled");
    }
}

void *Window::GetUserPointer() {
    return glfwGetWindowUserPointer(handle);
}

void Window::SetUserPointer(void *ptr) {
    glfwSetWindowUserPointer(handle, ptr);
}