/**
* Author: Amalie
* Assignment: Rise of the AI
* Date due: 2024-15-9, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/


#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define ENEMY_COUNT 3
#define LEVEL1_WIDTH 20
#define LEVEL1_HEIGHT 5

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL_mixer.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"
#include "Map.h"

// ————— GAME STATE ————— //
struct GameState
{
    Entity *player;
    Entity *enemies;
    
    Map *map;
    
    Mix_Music *bgm;
    Mix_Chunk *jump_sfx;
};

enum AppStatus { RUNNING, TERMINATED };

// ————— CONSTANTS ————— //
constexpr int WINDOW_WIDTH  = 640 * 2,
          WINDOW_HEIGHT = 480 * 2;

constexpr float BG_RED     = 0.1922f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.9059f,
            BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char GAME_WINDOW_NAME[] = "Hello, Maps!";

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr char SPRITESHEET_FILEPATH[] = "assets/images/char.png",
           MAP_TILESET_FILEPATH[] = "assets/images/tileset.png",
           BGM_FILEPATH[]         = "assets/audio/CloudDancer.mp3",
           JUMP_SFX_FILEPATH[]    = "assets/audio/jump.wav",
           ENEMY_SPRITESHEET_FILEPATH[] = "assets/images/BatPig.png";


GLuint background_texture_id;
GLuint g_font_texture_id;


constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL  = 0;
constexpr GLint TEXTURE_BORDER   = 0;

unsigned int LEVEL_1_DATA[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 3, 3, 0, 0, 3, 0, 0, 0, 3, 3, 0, 0, 3, 0, 0, 0, 3, 0,
    0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 2, 1, 0, 1, 2, 1, 1, 2, 0, 1, 1, 2, 1, 1, 1, 2, 1,
    1, 1, 2, 2, 1, 0, 2, 2, 1, 1, 2, 0, 1, 1, 2, 2, 1, 1, 2, 2
};

// ————— VARIABLES ————— //
GameState g_game_state;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f,
      g_accumulator    = 0.0f;


void initialise();
void process_input();
void update();
void render();
void shutdown();

// ————— GENERAL FUNCTIONS ————— //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    GLuint texture_id;
    glGenTextures(NUMBER_OF_TEXTURES, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    stbi_image_free(image);
    
    return texture_id;
}

void initialise()
{
    // ————— GENERAL ————— //
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow(GAME_WINDOW_NAME,
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    if (context == nullptr)
    {
        LOG("ERROR: Could not create OpenGL context.\n");
        shutdown();
    }
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    // ————— VIDEO SETUP ————— //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    // ————— MAP SET-UP ————— //
    GLuint map_texture_id = load_texture(MAP_TILESET_FILEPATH);
    
    background_texture_id = load_texture("assets/images/Background.png");
    g_font_texture_id = load_texture("assets/images/font1.png");

    g_game_state.map = new Map(LEVEL1_WIDTH, LEVEL1_HEIGHT, LEVEL_1_DATA, map_texture_id, 1.0f, 4, 1);
    
    // ————— GEORGE SET-UP ————— //

    GLuint player_texture_id = load_texture(SPRITESHEET_FILEPATH);

    int player_walking_animation[4][4] =
    {
        { 1, 5, 9, 13 },  // for George to move to the left,
        { 3, 4, 5, 6 }, // for George to move to the right,
        { 11, 10, 8, 11 }, // for George to move upwards,
        { 11, 10, 9, 11 }   // for George to move downwards
    };

//    ENEMY SET-UP
    GLuint enemy_texture_id = load_texture(ENEMY_SPRITESHEET_FILEPATH);

    int batpig_fly_animation[1][4] = {
        { 0, 1, 2, 3 }
    };
  
    float tile_size = 0.5f;

    g_game_state.player = new Entity(
        player_texture_id,
        5.0f,
        glm::vec3(0.0f, -4.905f, 0.0f),
        3.0f,
        player_walking_animation,
        0.0f,
        4,
        0,
        4,
        4,
        tile_size * 0.9f,
        tile_size * 0.9f,
        PLAYER
    );

    g_game_state.player->set_position(glm::vec3(0.0f, -1.0f, 0.0f));
    g_projection_matrix = glm::ortho(-7.25f, 7.25f, -5.25f, 5.25f, -1.0f, 1.0f);

    
    g_game_state.enemies = new Entity[3];

    // enemy 1:  circular Flyer
    g_game_state.enemies[0] = Entity(enemy_texture_id, 1.0f, glm::vec3(0.0f), 0.0f, batpig_fly_animation, 0.0f, 4, 0, 4, 1, 0.9f, 0.9f, ENEMY);
    g_game_state.enemies[0].set_position(glm::vec3(2.0f, 0.2f, 0.0f));  // Placed above the platform on the far left
    g_game_state.enemies[0].set_ai_type(FLYER);
    
    //  enemy 2 : Jumping up and down
    g_game_state.enemies[1] = Entity(enemy_texture_id, 1.2f, glm::vec3(0.0f), 0.0f, batpig_fly_animation, 0.0f, 4, 0, 4, 1, 0.9f, 0.9f, ENEMY);
    g_game_state.enemies[1].set_position(glm::vec3(13.0f, 1.0f, 0.0f));
    g_game_state.enemies[1].set_ai_type(JUMPER);

    // enemy3: guarding follwoing the player 
    g_game_state.enemies[2] = Entity(enemy_texture_id, 1.0f, glm::vec3(0.0f), 0.0f, batpig_fly_animation, 0.0f, 4, 0, 4, 1, 0.9f, 0.9f, ENEMY);
    g_game_state.enemies[2].set_position(glm::vec3(7.0f,-1.3f, 0.0f));
    g_game_state.enemies[2].set_ai_type(GUARD);
   
    g_game_state.enemies[0].set_ai_state(IDLE);
    g_game_state.enemies[1].set_ai_state(IDLE);
    g_game_state.enemies[2].set_ai_state(IDLE);


    // Jumping
    g_game_state.player->set_jumping_power(5.0f);

    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
    
    g_game_state.bgm = Mix_LoadMUS(BGM_FILEPATH);
    
    Mix_PlayMusic(g_game_state.bgm, -1);
    
  
    Mix_VolumeMusic(MIX_MAX_VOLUME / 16.0f);
    
    g_game_state.jump_sfx = Mix_LoadWAV(JUMP_SFX_FILEPATH);
    
    // ————— BLENDING ————— //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
void process_input()
{
    g_game_state.player->set_movement(glm::vec3(0.0f));
    
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_app_status = TERMINATED;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        g_app_status = TERMINATED;
                        break;
                        
                    case SDLK_SPACE:
                        if (g_game_state.player->get_collided_bottom())
                        {
                            g_game_state.player->jump();
                            Mix_PlayChannel(-1, g_game_state.jump_sfx, 0);
                        }
                        break;

                    case SDLK_k:
                        // kill enemies close to the player with K
                        for (int i = 0; i < ENEMY_COUNT; i++) {
                            if (g_game_state.enemies[i].is_active() &&
                                glm::distance(g_game_state.player->get_position(), g_game_state.enemies[i].get_position()) < 1.0f) {
                                g_game_state.enemies[i].deactivate();
                            }
                        }
                        break;
                        
                    default:
                        break;
                }
                
            default:
                break;
        }
    }
    
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_LEFT])       g_game_state.player->move_left();
    else if (key_state[SDL_SCANCODE_RIGHT]) g_game_state.player->move_right();
         
    if (glm::length(g_game_state.player->get_movement()) > 1.0f)
        g_game_state.player->normalise_movement();
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    delta_time += g_accumulator;
    
    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }
    
    while (delta_time >= FIXED_TIMESTEP)
    {
        // Update the player
        g_game_state.player->update(FIXED_TIMESTEP, g_game_state.player, g_game_state.enemies, ENEMY_COUNT, g_game_state.map);

        // Update each enemy
        for (int i = 0; i < ENEMY_COUNT; ++i)
        {
            g_game_state.enemies[i].update(FIXED_TIMESTEP, g_game_state.player, g_game_state.enemies, ENEMY_COUNT, g_game_state.map);
        }

        delta_time -= FIXED_TIMESTEP;
    }
    
    g_accumulator = delta_time;
    
    // Update the camera to follow the player
    g_view_matrix = glm::mat4(1.0f);
    g_view_matrix = glm::translate(g_view_matrix, glm::vec3(-g_game_state.player->get_position().x, 0.0f, 0.0f));
    
}

constexpr int FONTBANK_SIZE = 16;

void draw_text(ShaderProgram *program, GLuint font_texture_id, std::string text,
               float font_size, float spacing, glm::vec3 position)
{
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    for (int i = 0; i < text.size(); i++) {
        int spritesheet_index = (int) text[i];
        float offset = (font_size + spacing) * i;
        
        float u_coordinate = (float) (spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float) (spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        vertices.insert(vertices.end(), {
            offset + (-0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
        });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
        });
    }

    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);
    
    program->set_model_matrix(model_matrix);
    glUseProgram(program->get_program_id());
    
    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());
    
    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int) (text.size() * 6));
    
    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Static background rendering
    g_shader_program.set_view_matrix(glm::mat4(1.0f));

    glm::mat4 background_model_matrix = glm::mat4(1.0f);
    g_shader_program.set_model_matrix(background_model_matrix);

    glBindTexture(GL_TEXTURE_2D, background_texture_id);

    float background_vertices[] = {
        -5.0f,  3.75f,   5.0f,  3.75f,   5.0f, -3.75f,
        -5.0f,  3.75f,   5.0f, -3.75f,  -5.0f, -3.75f
    };

    float background_tex_coords[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, background_vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, background_tex_coords);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    g_shader_program.set_view_matrix(g_view_matrix);

    g_game_state.map->render(&g_shader_program);
    g_game_state.player->render(&g_shader_program);

    for (int i = 0; i < ENEMY_COUNT; i++) {
        g_game_state.enemies[i].render(&g_shader_program);
    }

    // Check if the player is inactive
    if (!g_game_state.player->is_active()) {
        glm::vec3 player_position = g_game_state.player->get_position();
        draw_text(&g_shader_program, g_font_texture_id, "YOU LOSE", 0.8f, 0.1f,
                  glm::vec3(player_position.x - 2.0f, player_position.y, 0.0f));
        SDL_GL_SwapWindow(g_display_window);
        SDL_Delay(3000);
        g_app_status = TERMINATED;
        return;
    }

    // Check if all enemies are defeated
    bool all_enemies_defeated = true;
    for (int i = 0; i < ENEMY_COUNT; i++) {
        if (g_game_state.enemies[i].is_active()) {
            all_enemies_defeated = false;
            break;
        }
    }

    if (all_enemies_defeated) {
        glm::vec3 player_position = g_game_state.player->get_position();
        draw_text(&g_shader_program, g_font_texture_id, "YOU WIN", 0.8f, 0.1f,
                  glm::vec3(player_position.x - 2.0f, player_position.y, 0.0f));
        SDL_GL_SwapWindow(g_display_window);
        SDL_Delay(3000);
        g_app_status = TERMINATED;
        return;
    }

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown()
{    
    SDL_Quit();
    
    delete [] g_game_state.enemies;
    delete    g_game_state.player;
    delete    g_game_state.map;
    Mix_FreeChunk(g_game_state.jump_sfx);
    Mix_FreeMusic(g_game_state.bgm);
}

// ————— GAME LOOP ————— //
int main(int argc, char* argv[])
{
    initialise();
    
    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }
    
    shutdown();
    return 0;
}
