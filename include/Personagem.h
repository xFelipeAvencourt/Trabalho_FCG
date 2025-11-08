#include "Constantes.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

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

    Camera(vec3 position = POS_INICIAL, vec3 up = WORLD_UP_INICIAL,
           float yaw = YAW, float pitch = PITCH) : Front(FRONT_INICIAL), 
           MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    mat4 GetViewMatrix() {
        return lookAt(Position, Position + Front, Up);
    }

    void ProcessKeyboard(Camera_Movement direction, float deltaTime) {

        float velocity = MovementSpeed * deltaTime;

        vec3 move = vec3(0.0f);

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
        if(isJumping){
            
            jumpVelocity += GRAVITY * deltaTime;
            Position.y += jumpVelocity * deltaTime;

            if (Position.y <= GROUND_LEVEL) {
                Position.y = GROUND_LEVEL;
                jumpVelocity = 0.0f;
                isJumping = false;
            }
        }


        Position += move;
    }

    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        if (constrainPitch) {
            Pitch = std::min(Pitch, 89.0f);
            Pitch = std::max(Pitch, -89.0f);
        }
        updateCameraVectors();
    }

    void ProcessMouseScroll(float yoffset) {
        Zoom -= (float)yoffset;
        Zoom = std::max(Zoom, 1.0f);
        Zoom = std::min(Zoom, 45.0f);
    }

private:
    bool isJumping = false;
    float jumpVelocity = 0.0f;
    const float jumpForce = 8.0f;

    void updateCameraVectors() {
        vec3 front;
        front.x = cos(radians(Yaw)) * cos(radians(Pitch));
        front.y = sin(radians(Pitch));
        front.z = sin(radians(Yaw)) * cos(radians(Pitch));
        Front = normalize(front);
        Right = normalize(cross(Front, WorldUp));
        Up = normalize(cross(Right, Front));
    }
};