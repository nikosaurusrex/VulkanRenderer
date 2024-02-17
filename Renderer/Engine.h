#ifndef ENGINE_H
#define ENGINE_H 

#include "Common.h"
#include "Core/Memory.h"

struct Event {
 
    enum EventType {
        Resize,
        MouseButton,
        MouseMove,
        MouseScroll,
        Key
    };

    EventType type;

    union {
        struct {
            s32 width;
            s32 height;
        };
        struct {
            s32 button;
            s32 action;
            s32 mods;
        };
        struct {
            f64 xpos;
            f64 ypos;
        };
    };
};

struct Window;
struct Engine {
    queue<Event> events;
    Window *window = 0;
    bool running = true;
    
    Engine();
    ~Engine();

    void Start();
    void Update();
    void HandleEvent(Event event);
};

#endif
