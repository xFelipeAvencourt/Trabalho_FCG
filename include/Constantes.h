// TELA:
#define WIDTH       1500
#define HEIGHT      1000
#define NEARPLANE   -0.1f
#define FARPLANE    -15.0f

// Dimensoes da sala:
#define SCALE_FLOOR     5.0f
#define SCALE_WALL      2.0f
#define SCALE_TABLE     0.5f

// Constante de dificuldade
#define TOTAL_TIME 60.0f
// LUZ:
#define LIGHT_POSITION      glm::vec3(0.0f, SCALE_WALL - 0.75f, 0.0f)
#define LIGHT_COLOR         glm::vec3(1.0f, 0.95f, 0.8f)
#define LIGHT_INTENSITY     1.0f

// CHAO:
#define GROUND_LEVEL 0.0f
#define LEVER_POSITION glm::vec3{0.1f, -0.25f, 0.0f}

// TEXTOS:
#define FONT_HEIGHT      3.0f

#define PI 3.1415926535f
#define PI_2 1.57079632675f
#define GRAVITY     -25.0f
#define PLAYER_HEIGHT SCALE_WALL / 40
#define TIME_KILL 3.0f
#define AGACHADO GROUND_LEVEL - 0.5f

#define BASE_DOOR_X 0.0f
#define BASE_DOOR_Y 0.1f
#define BASE_DOOR_Z SCALE_FLOOR
#define BASE_DOOR_ANGLE 0.0f
#define DOOR_RADIUS 0.5f

#define BASE_LEVER_X 0.0f
#define BASE_LEVER_Y -0.25f
#define BASE_LEVER_Z 0.0f

#define BALL_SCALE 2.0f
#define BALL_SPEED 5.0f
#define HITS_TO_ACTIVATE_LEVER 5

#ifndef GAME_STATE
#define GAME_STATE
enum class GameState {
    START_MENU,
    GAME_PLAY,
    GHOST_MODE,
    GAME_OVER
};
#endif

#ifndef DEATH_CAUSE
#define DEATH_CAUSE
enum class DeathCause {
    NONE,
    DARDO,
    TIMEOUT
};
#endif