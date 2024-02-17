#ifndef CAMERA_H
#define CAMERA_H

#include "glm/glm.hpp"

#include "../Common.h"

#define PI 3.141592653589f

struct FPSCamera {
    glm::mat4   projection;
    glm::mat4   view;
    glm::vec3   position = {0, 0, 0};
    f32 	   	backup_y = 0.0f;

    f32         fov;
    f32         aspect;
    f32         near;
    f32         far;

    f32         yaw     = 0;
    f32         pitch   = 0;
    f32         sensitivity = 3.182f;
    f32         m_yaw   = 0.022f;
    f32         m_pitch = 0.022f;
    
    FPSCamera(f32 fov, f32 aspect, f32 near, f32 far);
    ~FPSCamera();
    
    void Calculate(s32 width, s32 height);
    void ChangeOrientation();
    void Update(f32 delta);
};

struct Window;
struct FreeCamera {
	glm::mat4  projection;
	glm::mat4  view;
	glm::vec3  lookat;
	f32        yaw = -PI / 2.0f;
	f32        pitch = PI / 4.0f;
	f32		   radius = 10.0f;
	f32		   sensitivity = 0.01f;

	FreeCamera(glm::vec3 lookat);
	~FreeCamera();

	void Calculate(s32 width, s32 height);
	void Update(Window *window, f32 delta);
	void UpdateView();
};

#endif
