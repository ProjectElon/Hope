#pragma once

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/quaternion.hpp> // quaternion
#include <glm/gtx/quaternion.hpp> // quaternion
#include <glm/ext/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/ext/scalar_constants.hpp> // glm::pi

#include "core/defines.h"

struct Camera
{
    glm::vec3 position;
    glm::quat rotation;

    F32 aspect_ratio;
    F32 field_of_view;

    F32 near_clip;
    F32 far_clip;

    glm::mat4 view;
    glm::mat4 projection;
};

void init_camera(Camera *camera, glm::vec3 position,
                 glm::quat rotation, F32 aspect_ratio,
                 F32 field_of_view = 45.0f, F32 near_clip = 0.1f, F32 far_clip = 1000.0f);

void update_camera(Camera *camera);

struct FPS_Camera_Controller
{
    F32 rotation_speed;
    F32 pitch;
    F32 yaw;

    F32 movement_speed;
};

void init_fps_camera_controller(FPS_Camera_Controller *camera_controller,
                                Camera *camera);