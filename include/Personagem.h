#include "Constantes.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

#include <cstdio>
#include <cstdlib>

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN,
    JUMP
};

using namespace glm;

#define FRONT_INICIAL vec3(0.0f, 0.0f, -1.0f)
#define WORLD_UP_INICIAL vec3(0.0f, 1.0f, 0.0f)
#define POS_INICIAL vec3(0.0f, 0.0f, 3.0f)

#define YAW         -90.0f
#define PITCH       0.0f
#define SPEED       3.0f
#define SENSITIVITY 0.1f
#define ZOOM        45.0f

class Camera {
public:
    vec3 Position;
    vec3 Front;
    vec3 Up;
    vec3 Right;
    vec3 WorldUp;

    float Yaw;
    float Pitch;

    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
    GameState gameState;

    Camera(vec3 position = POS_INICIAL, vec3 up = WORLD_UP_INICIAL,
           float yaw = YAW, float pitch = PITCH, GameState gameState = GameState::START_MENU) : Front(FRONT_INICIAL), 
           MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM), gameState(gameState) {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        this->gameState = gameState;
        updateCameraVectors(gameState);
    }

    mat4 GetViewMatrix() {
        return lookAt(Position, Position + Front, Up);
    }

    void setGhostMode(bool ghost, GameState state) {
        if (!ghost) {
            float descentSpeed = 1.0f;
            while (Position.y > GROUND_LEVEL) {
                Position.y -= descentSpeed * (1.0f / 60.0f);
            if (Position.y < GROUND_LEVEL)
                Position.y = GROUND_LEVEL;
            }

            updateCameraVectors(state);
        }

    }

    void setCtrlMode(bool press, GameState state) {
        if (!isJumping) {
            if(press) {
                isCrouched = true;
                float descentSpeed = 1.0f;
                while (Position.y > AGACHADO) {
                    Position.y -= descentSpeed * (1.0f / 60.0f);
                    if (Position.y < AGACHADO)
                        Position.y = AGACHADO;
                }
            }
            else {
                isCrouched = false;
                float ascentSpeed = 1.0f;
                while (Position.y < GROUND_LEVEL) {
                    Position.y += ascentSpeed * (1.0f / 60.0f);
                    if (Position.y > GROUND_LEVEL)
                        Position.y = GROUND_LEVEL;    
                }
            }
            updateCameraVectors(state);
        }
    }

    void ProcessKeyboard(Camera_Movement direction, float deltaTime, bool ghostMode = false) {

        float velocity = MovementSpeed * deltaTime;

        vec3 move = vec3(0.0f);
        
        if(ghostMode) {
            if (direction == FORWARD)
                move += Front * velocity;
            if (direction == BACKWARD)
                move -= Front * velocity;
            if (direction == LEFT)
                move -= Right * velocity;
            if (direction == RIGHT)
                move += Right * velocity;
            if (direction == UP)
                move += Up * velocity;
            if (direction == DOWN)
                move -= Up * velocity;
        }
        else {
            if (direction == FORWARD)
                move += normalize(vec3(Front.x,0.0f,Front.z)) * velocity;
            if (direction == BACKWARD)
                move -= normalize(vec3(Front.x,0.0f,Front.z)) * velocity;
            if (direction == LEFT)
                move -= normalize(vec3(Right.x,0.0f,Right.z)) * velocity;
            if (direction == RIGHT)
                move += normalize(vec3(Right.x,0.0f,Right.z)) * velocity;
            if (direction == JUMP && !isJumping) {
                isJumping = true;
                jumpVelocity = jumpForce;
            }
        }

        Position += move;
    }

    void ProcessMouseMovement(float xoffset, float yoffset, GameState state, bool constrainPitch = true) {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        if (constrainPitch) {
            Pitch = std::min(Pitch, 89.0f);
            Pitch = std::max(Pitch, -89.0f);
        }
        updateCameraVectors(state);
    }

    void ProcessMouseScroll(float yoffset) {
        Zoom -= (float)yoffset;
        Zoom = std::max(Zoom, 1.0f);
        Zoom = std::min(Zoom, 45.0f);
    }
    void Update(float deltaTime) {
        if (isJumping) {
            jumpVelocity += GRAVITY * deltaTime;
            Position.y += jumpVelocity * deltaTime;

            float targetYPosition = 0.0f;
            if (isCrouched)
                targetYPosition = AGACHADO;
            else
                targetYPosition = GROUND_LEVEL;

            if (Position.y <= targetYPosition) {
                Position.y = targetYPosition;
                jumpVelocity = 0.0f;
                isJumping = false;
            }
        }
    }

private:
    bool isJumping = false;
    bool isCrouched = false;
    float jumpVelocity = 0.0f;
    const float jumpForce = 8.0f;

    void updateCameraVectors(GameState state) {
        vec3 front;
        front.x = cos(radians(Yaw)) * cos(radians(Pitch));
        front.z = sin(radians(Yaw)) * cos(radians(Pitch));

        if (state == GameState::START_MENU) {
            float radius = 4.0f;
            Position.x = -radius * cos(radians(Yaw));
            Position.z = -radius * sin(radians(Yaw));
        } else {
            front.y = sin(radians(Pitch));
        }

        Front = normalize(front);
        Right = normalize(cross(Front, WorldUp));
        Up = normalize(cross(Right, Front));
    }
};