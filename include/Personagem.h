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
    DOWN
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

        Position.y = 0.0f;
        float velocity = MovementSpeed * deltaTime;

        vec3 move = vec3(0.0f);

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

        if (length(move) > 0) {
            move = normalize(move) * velocity;
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