#include "engine.h"
#include "platform.h"
#include "rendering/renderer.h"
#include "debugging.h"

extern Debug_State global_debug_state;

bool startup(Engine *engine, const Engine_Configuration &configuration, void *platform_state)
{
#ifndef HE_SHIPPING
    Logger *logger = &global_debug_state.main_logger;
    U64 channel_mask = 0xFFFFFFFFFFFFFFFF;
    bool logger_initied = init_logger(logger, "all",
                                      Verbosity_Trace, channel_mask);
    if (!logger_initied)
    {
        return false;
    }
#endif

    Mem_Size required_memory_size =
        configuration.permanent_memory_size + configuration.transient_memory_size;

    void *memory = platform_allocate_memory(required_memory_size);
    if (!memory)
    {
        return false;
    }

    U8 *permanent_memory = (U8 *)memory;
    engine->memory.permanent_memory_size = configuration.permanent_memory_size;
    engine->memory.permanent_arena = create_memory_arena(permanent_memory,
                                                         configuration.permanent_memory_size);

    U8 *transient_memory = (U8 *)memory + configuration.permanent_memory_size;
    engine->memory.transient_memory_size = configuration.transient_memory_size;
    engine->memory.transient_arena = create_memory_arena(transient_memory,
                                                         configuration.transient_memory_size);

    init_free_list_allocator(&engine->memory.free_list_allocator,
                             &engine->memory.transient_arena,
                             HE_MegaBytes(128));

    engine->show_cursor = configuration.show_cursor;
    engine->lock_cursor = configuration.lock_cursor;
    engine->window_mode = configuration.window_mode;
    engine->platform_state = platform_state;

    if (engine->window_mode == WindowMode_Fullscreen)
    {
        platform_toggle_fullscreen(engine);
    }

    bool input_inited = init_input(&engine->input);
    if (!input_inited)
    {
        return false;
    }

    bool requested = request_renderer(RenderingAPI_Vulkan, &engine->renderer);
    if (!requested)
    {
        return false;
    }

    Renderer *renderer = &engine->renderer;
    bool renderer_inited = renderer->init(&engine->renderer_state,
                                          engine,
                                          &engine->memory.permanent_arena);
    if (!renderer_inited)
    {
        return false;
    }
    Renderer_State *renderer_state = &engine->renderer_state;
    bool renderer_state_inited = init_renderer_state(renderer_state,
                                                     &engine->memory.transient_arena);
    if (!renderer_state_inited)
    {
        return false;
    }

    renderer_state->back_buffer_width = configuration.back_buffer_width;
    renderer_state->back_buffer_height = configuration.back_buffer_height;

    /*renderer_state->sponza = load_model("models/Sponza/Sponza.gltf", renderer, renderer_state,
                                        &engine->memory.transient_arena);
    Assert(renderer_state->sponza);*/

    renderer_state->flight_helmet = load_model("models/FlightHelmet/FlightHelmet.gltf", renderer, renderer_state,
                                               &engine->memory.transient_arena);
    Assert(renderer_state->flight_helmet);

    Platform_API *api = &engine->platform_api;
    api->allocate_memory = &platform_allocate_memory;
    api->deallocate_memory = &platform_deallocate_memory;
    api->open_file = &platform_open_file;
    api->is_file_handle_valid = &platform_is_file_handle_valid;
    api->read_data_from_file = &platform_read_data_from_file;
    api->write_data_to_file = &platform_write_data_to_file;
    api->close_file = &platform_close_file;
    api->debug_printf = &platform_debug_printf;
    api->toggle_fullscreen = &platform_toggle_fullscreen;

    engine->api.init_camera = &init_camera;
    engine->api.init_fps_camera_controller = &init_fps_camera_controller;
    engine->api.control_camera = &control_camera;

    engine->api.load_model = &load_model;
    engine->api.render_scene_node = &render_scene_node;

    Game_Code *game_code = &engine->game_code;
    bool game_initialized = game_code->init_game(engine);
    renderer->wait_for_gpu_to_finish_all_work(renderer_state);
    return game_initialized;
}

void game_loop(Engine *engine, F32 delta_time)
{
    Game_Code *game_code = &engine->game_code;
    game_code->on_update(engine, delta_time);
}

void shutdown(Engine *engine)
{
    (void)engine;

    Renderer *renderer = &engine->renderer;
    Renderer_State *renderer_state = &engine->renderer_state;
    renderer->wait_for_gpu_to_finish_all_work(renderer_state);

    // todo(amer): maybe we want a iterator version of this...
    for (U32 texture_index = 0; texture_index < renderer_state->texture_count; texture_index++)
    {
        Texture *texture = (Texture*)(renderer_state->textures + texture_index * renderer_state->texture_bundle_size);
        renderer->destroy_texture(texture);
    }

    for (U32 material_index = 0; material_index < renderer_state->material_count; material_index++)
    {
        Material *material = (Material *)(renderer_state->materials + material_index * renderer_state->material_bundle_size);
        renderer->destroy_material(material);
    }

    for (U32 static_mesh_index = 0; static_mesh_index < renderer_state->static_mesh_count; static_mesh_index++)
    {
        Static_Mesh *static_mesh = (Static_Mesh *)(renderer_state->static_meshes + static_mesh_index * renderer_state->static_mesh_bundle_size);
        renderer->destroy_static_mesh(static_mesh);
    }

    renderer->deinit(&engine->renderer_state);

#ifndef HE_SHIPPING
    Logger *logger = &global_debug_state.main_logger;
    deinit_logger(logger);
#endif
}

void set_game_code_to_stubs(Game_Code *game_code)
{
    game_code->init_game = &init_game_stub;
    game_code->on_event = &on_event_stub;
    game_code->on_update = &on_update_stub;
}

// todo(amer): maybe we should program a game in the stubs...
bool init_game_stub(Engine *engine)
{
    (void)engine;
    return true;
}

void on_event_stub(Engine *engine, Event event)
{
    (void)engine;
    (void)event;
}

void on_update_stub(Engine *engine, F32 delta_time)
{
    (void)engine;
    (void)delta_time;
}