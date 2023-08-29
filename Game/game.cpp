#include "core/defines.h"
#include "core/engine.h"


struct Game_State
{
	Camera camera;
	FPS_Camera_Controller camera_controller;

	Scene_Node *sponza;
	Scene_Node *flight_helmet;
};

static Game_State game_state;

extern "C" HOPE_API bool
init_game(Engine *engine)
{
	Renderer *renderer = &engine->renderer;
	Renderer_State *renderer_state = &engine->renderer_state;

    F32 aspect_ratio = (F32)renderer_state->back_buffer_width / (F32)renderer_state->back_buffer_height;
    glm::quat camera_rotation = glm::quat({ 0.0f, 0.0f, 0.0f });
    Camera *camera = &game_state.camera;
	F32 fov = 45.0f;
	F32 near = 0.1f;
	F32 far = 1000.0f;
	engine->api.init_camera(camera, { 0.0f, 2.0f, 5.0f }, camera_rotation, aspect_ratio, fov, near, far);

    FPS_Camera_Controller *camera_controller = &game_state.camera_controller;
    F32 rotation_speed = 45.0f;
    F32 base_movement_speed = 20.0f;
    F32 max_movement_speed = 40.0f;
	F32 sensitivity_x = 1.0f;
	F32 sensitivity_y = 1.0f;

	engine->api.init_fps_camera_controller(camera_controller, /*pitch=*/0.0f, /*yaw=*/0.0f,
                                           rotation_speed,
                                           base_movement_speed,
                                           max_movement_speed, sensitivity_x, sensitivity_y);

	/*game_state.sponza = engine->api.load_model("models/Sponza/Sponza.gltf", renderer, renderer_state);
    Assert(game_state.sponza);*/

    game_state.flight_helmet = engine->api.load_model("models/FlightHelmet/FlightHelmet.gltf", renderer, renderer_state);
    HOPE_Assert(game_state.flight_helmet);

    return true;
}

extern "C" HOPE_API void
on_event(Engine *engine, Event event)
{
	Platform_API *platform = &engine->platform_api;

	switch (event.type)
	{
		case EventType_Key:
		{
			if (event.pressed)
			{
				if (event.key == HE_KEY_ESCAPE)
				{
					engine->is_running = false;
				}
				else if (event.key == HE_KEY_F11)
				{
                }
                else if (event.key == HE_KEY_F10)
                {
                    engine->show_imgui = !engine->show_imgui;
                }
			}
		} break;

        case EventType_Resize:
        {
            if (event.width != 0 && event.height != 0)
            {
                game_state.camera.aspect_ratio = (F32)event.width / (F32)event.height;
                engine->api.update_camera(&game_state.camera);
            }
        } break;
	}
}

extern "C" HOPE_API void
on_update(Engine *engine, F32 delta_time)
{
	(void)delta_time;

	Input *input = &engine->input;
    Renderer *renderer = &engine->renderer;
    Renderer_State *renderer_state = &engine->renderer_state;

    Camera *camera = &game_state.camera;
    FPS_Camera_Controller *camera_controller = &game_state.camera_controller;

    FPS_Camera_Controller_Input camera_controller_input = {};
    camera_controller_input.can_control = input->button_states[HE_BUTTON_RIGHT] != InputState_Released && !engine->show_imgui;
    camera_controller_input.move_fast = input->key_states[HE_KEY_LEFT_SHIFT] != InputState_Released;
    camera_controller_input.forward = input->key_states[HE_KEY_W] != InputState_Released;
    camera_controller_input.backward = input->key_states[HE_KEY_S] != InputState_Released;
    camera_controller_input.left = input->key_states[HE_KEY_A] != InputState_Released;
    camera_controller_input.right = input->key_states[HE_KEY_D] != InputState_Released;
    camera_controller_input.up = input->key_states[HE_KEY_E] != InputState_Released;
    camera_controller_input.down = input->key_states[HE_KEY_Q] != InputState_Released;
    camera_controller_input.delta_x = -input->mouse_delta_x;
    camera_controller_input.delta_y = -input->mouse_delta_y;

    if (camera_controller_input.can_control)
    {
        engine->lock_cursor = true;
        engine->show_cursor = false;
        engine->api.control_camera(camera_controller,
                                   camera,
                                   camera_controller_input,
                                   delta_time);
    }
    else
    {
        engine->lock_cursor = false;
        engine->show_cursor = true;
    }

    if (!engine->is_minimized)
    {
        Scene_Data *scene_data = &renderer_state->scene_data;

        scene_data->view = camera->view;
        scene_data->projection = camera->projection;

        renderer->begin_frame(renderer_state, scene_data);
        // engine->api.render_scene_node(renderer,renderer_state, engine->api.sponza, glm::scale(glm::mat4(1.0f), glm::vec3(20.0f)));
        engine->api.render_scene_node(renderer, renderer_state, game_state.flight_helmet, glm::scale(glm::mat4(1.0f), glm::vec3(10.0f)));
        renderer->end_frame(renderer_state);
    }
}