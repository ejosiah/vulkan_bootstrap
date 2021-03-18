#pragma once

#include "common.h"
#include "InputManager.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

static constexpr float HALF_PI = glm::half_pi<float>();
static constexpr float PI = glm::pi<float>();
static constexpr float TWO_PI = glm::two_pi<float>();

static constexpr float DEFAULT_ROTATION_SPEED = 0.3f;
static constexpr float DEFAULT_FOVX = 90.0f;
static constexpr float DEFAULT_ZNEAR = 0.1f;
static constexpr float DEFAULT_ZFAR = 1000.0f;
static constexpr float DEFAULT_ZOOM_MAX = 5.0f;
static constexpr float DEFAULT_ZOOM_MIN = 1.5f;

static constexpr glm::vec3 DEFAULT_ACCELERATION(4.0f, 4.0f, 4.0f);
static constexpr glm::vec3 DEFAULT_VELOCITY(1.0f);

constexpr glm::vec3 WORLD_XAXIS(1.0f, 0.0f, 0.0f);
constexpr glm::vec3 WORLD_YAXIS(0.0f, 1.0f, 0.0f);
constexpr glm::vec3 WORLD_ZAXIS(0.0f, 0.0f, 1.0f);

struct Camera{
    glm::mat4 model = glm::mat4(1);
    glm::mat4 view = glm::mat4(1);
    glm::mat4 proj = glm::mat4(1);
};

struct CameraSettings{
    glm::vec3 acceleration = DEFAULT_ACCELERATION;
    glm::vec3 velocity = DEFAULT_VELOCITY;
    float rotationSpeed = DEFAULT_ROTATION_SPEED;
    float fieldOfView = DEFAULT_FOVX;
    float aspectRatio = 1.0F;
    float zNear = DEFAULT_ZNEAR;
    float zFar = DEFAULT_ZFAR;
    float minZoom = DEFAULT_ZOOM_MIN;
    float maxZoom = DEFAULT_ZOOM_MAX;
};

struct CameraController{
public:
    CameraController(Camera& camera, InputManager& inputManager, const CameraSettings& settings = {});

    virtual ~CameraController() = default;

    virtual void update(float time) = 0;

    virtual void processInput();

    void lookAt(const glm::vec3 &eye, const glm::vec3 &target, const glm::vec3 &up);

    void perspective(float fovx, float aspect, float znear, float zfar);

    void rotateSmoothly(float headingDegrees, float pitchDegrees, float rollDegrees);

    virtual void rotate(float headingDegrees, float pitchDegrees, float rollDegrees);

    virtual void move(float dx, float dy, float dz);

    virtual void move(const glm::vec3 &direction, const glm::vec3 &amount);

    void position(const glm::vec3& pos);

    void updatePosition(const glm::vec3 &direction, float elapsedTimeSec);

    virtual void undoRoll();

    virtual void zoom(float zoom, float minZoom, float maxZoom);


protected:
    virtual void updateViewMatrix();

    void updateVelocity(const glm::vec3 &direction, float elapsedTimeSec);

    float fovx;
    float aspectRatio;
    float znear;
    float zfar;
    float minZoom;
    float maxZoom;
    float rotationSpeed;
    float accumPitchDegrees;
    float floorOffset;
    glm::vec3 eyes;
    glm::vec3 target;
    glm::vec3 targetYAxis;
    glm::vec3 xAxis;
    glm::vec3 yAxis;
    glm::vec3 zAxis;
    glm::vec3 viewDir;
    glm::vec3 acceleration;
    glm::vec3 currentVelocity;
    glm::vec3 velocity;
    glm::quat orientation;
    glm::vec3 direction;
    Camera& camera;
    const Mouse& mouse;
};