#ifndef WINDOW_H
#define WINDOW_H

#include "../Common.h"
#include "Engine.h"

#include "GLFW/glfw3.h"

struct WindowBounds {
    int x;
    int y;
    int width;
    int height;
};

struct GLFWwindow;
struct Window {
    const char *title;
    GLFWwindow *handle;
    bool in_fullscreen = false;
    WindowBounds bounds = {};
    WindowBounds backup_bounds = {};
    bool vsync;

    Window(const char *title, int width, int height);
    ~Window();

    void Update();
    void ToggleFullscreen();
    void SwitchToFullscreen(GLFWmonitor *monitor, const GLFWvidmode *mode);
    void SwitchToWindowed(const GLFWvidmode *mode);
    void SetVSync(bool vsync);
    void Destroy();
    bool ShouldClose();
    void SetEngine(Engine *handler);
    void SetHideCursor(bool hide);
    void GetCursorPos(f64 *xpos, f64 *ypos);
    void EnableRawInput();
    void *GetUserPointer();
    void SetUserPointer(void *ptr);
};

#endif
