#pragma once

#include "common.h"
#include "InputManager.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "VulkanDevice.h"
#include "AbstractCamera.hpp"

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


struct BaseCameraSettings{
    glm::vec3 acceleration = DEFAULT_ACCELERATION;
    glm::vec3 velocity = DEFAULT_VELOCITY;
    float rotationSpeed = DEFAULT_ROTATION_SPEED;
    float fieldOfView = DEFAULT_FOVX;
    float aspectRatio = 1.0F;
    float zNear = DEFAULT_ZNEAR;
    float zFar = DEFAULT_ZFAR;
    float minZoom = DEFAULT_ZOOM_MIN;
    float maxZoom = DEFAULT_ZOOM_MAX;
    float floorOffset = 0.5f;
    bool handleZoom = false;
    bool horizontalFov = false;
};

struct BaseCameraController : public AbstractCamera{
public:
    BaseCameraController(InputManager& inputManager, const BaseCameraSettings& settings = {});

    ~BaseCameraController() override = default;

    void processInput() override;

    void lookAt(const glm::vec3 &eye, const glm::vec3 &target, const glm::vec3 &up) final;

    void perspective(float fovx, float aspect, float znear, float zfar) final;

    void perspective(float aspect) final;

    void rotateSmoothly(float headingDegrees, float pitchDegrees, float rollDegrees) override;

    void move(float dx, float dy, float dz) override;

    void move(const glm::vec3 &direction, const glm::vec3 &amount) override;

    void position(const glm::vec3& pos) final;

    [[nodiscard]]
    const glm::vec3& position() const final;

    virtual void onPositionChanged();

    [[nodiscard]]
    const glm::vec3& velocity() const final;

    [[nodiscard]]
    const glm::vec3& acceleration() const final;

    void updatePosition(const glm::vec3 &direction, float elapsedTimeSec) override;

    void undoRoll() override;

    void zoom(float zoom, float minZoom, float maxZoom) override;

    void onResize(int width, int height) override;

    void setModel(const glm::mat4& model) override;

    void setTargetYAxis(const glm::vec3& axis);

    const glm::vec3& getYAxis();

    float near() override;

    float far() override;

    void push(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_VERTEX_BIT) const override;

    void push(VkCommandBuffer commandBuffer, VkPipelineLayout layout, const glm::mat4& model, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_VERTEX_BIT) override;

    [[nodiscard]]
    const Camera& cam() const final;

    [[nodiscard]]
    const glm::quat &getOrientation() const final;

public:
    virtual void updateViewMatrix();

    virtual void processMovementInput();

    virtual void processZoomInput();

    virtual void updateVelocity(const glm::vec3 &direction, float elapsedTimeSec);

    void newFrame() override;

    bool moved() const override;


    float fov;
    float aspectRatio;
    float znear;
    float zfar;
    float minZoom;
    float maxZoom;
    float rotationSpeed;
    float accumPitchDegrees;
    float floorOffset;
    bool handleZoom;
    bool horizontalFov;
    glm::vec3 eyes;
    glm::vec3 target;
    glm::vec3 targetYAxis;
    glm::vec3 xAxis;
    glm::vec3 yAxis;
    glm::vec3 zAxis;
    glm::vec3 viewDir;
    glm::vec3 _acceleration;
    glm::vec3 currentVelocity;
    glm::vec3 _velocity;
    glm::quat orientation;
    glm::vec3 direction;
    mutable Camera camera;
    const Mouse& mouse;

    float zoomAmount = 0;

    struct {
        Action* forward;
        Action* back;
        Action* left;
        Action* right;
        Action* up;
        Action* down;
    } _move{};

    Action& zoomIn;
    Action& zoomOut;

    bool _moved;
};