#include <stdio.h>

#include "Core/Window.h"
#include "Core/Input.h"

Engine::Engine() {
    window = new Window("Engine", 1280, 720);
    window->SetEngine(this);
}

Engine::~Engine() {
    delete window;
}

void Engine::Start() {
    running = true;
}

void Engine::Update() {
    window->Update();
        
    if (window->ShouldClose()) {
        running = false;
    }
}

void Engine::HandleEvent(Event event) {
    events.push(event);
}
