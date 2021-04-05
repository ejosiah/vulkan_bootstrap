#include "camera_base.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>

BaseCameraController::BaseCameraController(const VulkanDevice& device, uint32_t swapChainImageCount
                                           , const uint32_t& currentImageInde, InputManager& inputManager
                                           , const BaseCameraSettings& settings)
    : fovx(settings.fieldOfView)
    , aspectRatio(settings.aspectRatio)
    , znear(settings.zNear)
    , zfar(settings.zFar)
    , minZoom(settings.minZoom)
    , maxZoom(settings.maxZoom)
    , rotationSpeed(settings.rotationSpeed)
    , accumPitchDegrees(0.0f)
    , floorOffset(settings.floorOffset)
    , handleZoom(settings.handleZoom)
    , eyes(0.0f)
    , target(0.0f)
    , targetYAxis(0.0f, 1.0f, 0.0f)
    , xAxis(1.0f, 0.0f, 0.0f)
    , yAxis(0.0f, 1.0f, 0.0f)
    , zAxis(0.0f, 0.0f, 1.0f)
    , viewDir(0.0f, 0.0f, -1.0f)
    , acceleration(settings.acceleration)
    , velocity(settings.velocity)
    , currentVelocity(0)
    , orientation(1, 0, 0, 0)
    , direction(0)
    , camera()
    , mouse(inputManager.getMouse())
    , zoomIn(inputManager.mapToMouse(MouseEvent::MoveCode::WHEEL_UP))
    , zoomOut(inputManager.mapToMouse(MouseEvent::MoveCode::WHEEL_DOWN))
    , device(device)
    , swapChainImageCount(swapChainImageCount)
    , currentImageIndex(currentImageInde)
    {
        _move.forward = &inputManager.mapToKey(Key::W, "forward", Action::Behavior::DETECT_INITIAL_PRESS_ONLY);
        _move.back = &inputManager.mapToKey(Key::S, "backward", Action::Behavior::DETECT_INITIAL_PRESS_ONLY);
        _move.left = &inputManager.mapToKey(Key::A, "left", Action::Behavior::DETECT_INITIAL_PRESS_ONLY);
        _move.right = &inputManager.mapToKey(Key::D, "right", Action::Behavior::DETECT_INITIAL_PRESS_ONLY);
        _move.up = &inputManager.mapToKey(Key::E, "up", Action::Behavior::DETECT_INITIAL_PRESS_ONLY);
        _move.down = &inputManager.mapToKey(Key::Q, "down", Action::Behavior::DETECT_INITIAL_PRESS_ONLY);
        position({0.0f, floorOffset, 0.0f});    // assuming up is y axis
        perspective(fovx, aspectRatio, znear, zfar);
    }

void BaseCameraController::processInput() {
    processMovementInput();
    processZoomInput();
}

void BaseCameraController::processMovementInput() {
    direction = glm::vec3(0);
    auto vel = currentVelocity;
    if(_move.forward->isPressed()){
        currentVelocity.x = vel.x;
        currentVelocity.y = vel.y;
        currentVelocity.z = 0;
    }else if(_move.forward->isHeld()){
        direction.z += 1.0f;
    }

    if(_move.back->isPressed()){
        currentVelocity.x = vel.x;
        currentVelocity.y = vel.y;
        currentVelocity.z = 0;
    }else if(_move.back->isHeld()){
        direction.z -= 1.0f;
    }

    if(_move.right->isPressed()){
        currentVelocity.x = 0;
        currentVelocity.y = vel.y;
        currentVelocity.z = vel.z;
    }else if(_move.right->isHeld()){
        direction.x += 1.0f;
    }

    if(_move.left->isPressed()){
        currentVelocity.x = 0;
        currentVelocity.y = vel.y;
        currentVelocity.z = vel.z;
    }else if(_move.left->isHeld()){
        direction.x -= 1.0f;
    }


    if(_move.up->isPressed()){
        currentVelocity.x = vel.x;
        currentVelocity.y = 0.0f;
        currentVelocity.z = vel.z;
    }else if(_move.up->isHeld()){
        direction.y += 1.0f;
    }

    if(_move.down->isPressed()){
        currentVelocity.x = vel.x;
        currentVelocity.y = 0.0f;
        currentVelocity.z = vel.z;
    }else if(_move.down->isHeld()){
        direction.y -= 1.0f;
    }
}

void BaseCameraController::processZoomInput() {
    zoomAmount = 0.0f;
    if(zoomIn.isPressed()){
        zoomAmount = -0.1;
    }else if(zoomOut.isPressed()){
        zoomAmount = 0.1;
    }
    if(handleZoom && zoomAmount != 0){
        zoom(zoomAmount, minZoom, maxZoom);
    }
}


void BaseCameraController::lookAt(const glm::vec3 &eye, const glm::vec3 &target, const glm::vec3 &up) {

    this->eyes = eye;
    this->target = target;

    auto& view = camera.view;
    view = glm::lookAt(eye, target, up);
    // Extract the pitch angle from the view matrix.
    accumPitchDegrees = glm::degrees(asinf(view[1][2]));	// TODO change this matrix is colomn matrix

    xAxis = glm::vec3(row(view, 0));
    yAxis = glm::vec3(row(view, 1));
    zAxis = glm::vec3(row(view, 2));

    viewDir = -zAxis;

    accumPitchDegrees = glm::degrees(asinf(view[1][2]));

    orientation = glm::quat(view);
    updateViewMatrix();
}

void BaseCameraController::perspective(float aspect) {
    perspective(fovx, aspect, znear, zfar);
}

void BaseCameraController::perspective(float fovx, float aspect, float znear, float zfar) {
    camera.proj = vkn::perspective(glm::radians(fovx), aspect, znear, zfar);
    this->fovx = fovx;
    aspectRatio = aspect;
    this->znear = znear;
    this->zfar = zfar;

}

void BaseCameraController::rotateSmoothly(float headingDegrees, float pitchDegrees, float rollDegrees) {
    headingDegrees *= rotationSpeed;
    pitchDegrees *= rotationSpeed;
    rollDegrees *= rotationSpeed;

    rotate(headingDegrees, pitchDegrees, rollDegrees);
}

void BaseCameraController::undoRoll() {
    lookAt(eyes, eyes + viewDir, WORLD_YAXIS);
}

void BaseCameraController::zoom(float zoom, float minZoom, float maxZoom) {
    zoom = std::min(std::max(zoom, minZoom), maxZoom);
    perspective(zoom, aspectRatio, znear, zfar);
}

void BaseCameraController::move(float dx, float dy, float dz) {
    glm::vec3 eyes = this->eyes;
    glm::vec3 forwards = viewDir;

    eyes += xAxis * dx;
    eyes += WORLD_YAXIS * dy;
    eyes += forwards * dz;

    position(eyes);
}

void BaseCameraController::move(const glm::vec3 &direction, const glm::vec3 &amount) {
    eyes.x += direction.x * amount.x;
    eyes.y += direction.y * amount.y;
    eyes.z += direction.z * amount.z;

    updateViewMatrix();
}


void BaseCameraController::position(const glm::vec3 &pos) {
    eyes = pos;
    updateViewMatrix();
}

void BaseCameraController::updatePosition(const glm::vec3 &direction, float elapsedTimeSec) {
    // Moves the Camera using Newton's second law of motion. Unit mass is
    // assumed here to somewhat simplify the calculations. The direction vector
    // is in the range [-1,1].
    using namespace glm;
    if (dot(currentVelocity, currentVelocity) != 0.0f)
    {
        // Only move the Camera if the velocity vector is not of zero length.
        // Doing this guards against the Camera slowly creeping around due to
        // floating point rounding errors.

        glm::vec3 displacement = (currentVelocity * elapsedTimeSec) +
                                 (0.5f * acceleration * elapsedTimeSec * elapsedTimeSec);

        // Floating point rounding errors will slowly accumulate and cause the
        // Camera to move along each axis. To prevent any unintended movement
        // the displacement vector is clamped to zero for each direction that
        // the Camera isn't moving in. Note that the updateVelocity() method
        // will slowly decelerate the Camera's velocity back to a stationary
        // state when the Camera is no longer moving along that direction. To
        // account for this the Camera's current velocity is also checked.

        if (direction.x == 0.0f && closeEnough(currentVelocity.x, 0.0f))
            displacement.x = 0.0f;

        if (direction.y == 0.0f && closeEnough(currentVelocity.y, 0.0f))
            displacement.y = 0.0f;

        if (direction.z == 0.0f && closeEnough(currentVelocity.z, 0.0f))
            displacement.z = 0.0f;

        move(displacement.x, displacement.y, displacement.z);
    }

    // Continuously update the Camera's velocity vector even if the Camera
    // hasn't moved during this call. When the Camera is no longer being moved
    // the Camera is decelerating back to its stationary state.

    updateVelocity(direction, elapsedTimeSec);
}

void BaseCameraController::updateViewMatrix() {
    auto& view = camera.view;
    view = glm::mat4_cast(orientation);

    xAxis = glm::vec3(glm::row(view, 0));
    yAxis = glm::vec3(glm::row(view, 1));
    zAxis = glm::vec3(glm::row(view, 2));
    viewDir = -zAxis;

    view[3][0] = -dot(xAxis, eyes);
    view[3][1] = -dot(yAxis, eyes);
    view[3][2] =  -dot(zAxis, eyes);
}

void BaseCameraController::updateVelocity(const glm::vec3 &direction, float elapsedTimeSec) {
    // Updates the Camera's velocity based on the supplied movement direction
    // and the elapsed time (since this method was last called). The movement
    // direction is in the range [-1,1].

    if (direction.x != 0.0f)
    {
        // Camera is moving along the x axis.
        // Linearly accelerate up to the Camera's max speed.

        currentVelocity.x += direction.x * acceleration.x * elapsedTimeSec;

        if (currentVelocity.x > velocity.x)
            currentVelocity.x = velocity.x;
        else if (currentVelocity.x < -velocity.x)
            currentVelocity.x = -velocity.x;
    }
    else
    {
        // Camera is no longer moving along the x axis.
        // Linearly decelerate back to stationary state.

        if (currentVelocity.x > 0.0f)
        {
            if ((currentVelocity.x -= acceleration.x * elapsedTimeSec) < 0.0f)
                currentVelocity.x = 0.0f;
        }
        else
        {
            if ((currentVelocity.x += acceleration.x * elapsedTimeSec) > 0.0f)
                currentVelocity.x = 0.0f;
        }
    }

    if (direction.y != 0.0f)
    {
        // Camera is moving along the y axis.
        // Linearly accelerate up to the Camera's max speed.

        currentVelocity.y += direction.y * acceleration.y * elapsedTimeSec;

        if (currentVelocity.y > velocity.y)
            currentVelocity.y = velocity.y;
        else if (currentVelocity.y < -velocity.y)
            currentVelocity.y = -velocity.y;
    }
    else
    {
        // Camera is no longer moving along the y axis.
        // Linearly decelerate back to stationary state.

        if (currentVelocity.y > 0.0f)
        {
            if ((currentVelocity.y -= acceleration.y * elapsedTimeSec) < 0.0f)
                currentVelocity.y = 0.0f;
        }
        else
        {
            if ((currentVelocity.y += acceleration.y * elapsedTimeSec) > 0.0f)
                currentVelocity.y = 0.0f;
        }
    }

    if (direction.z != 0.0f)
    {
        // Camera is moving along the z axis.
        // Linearly accelerate up to the Camera's max speed.

        currentVelocity.z += direction.z * acceleration.z * elapsedTimeSec;

        if (currentVelocity.z > velocity.z)
            currentVelocity.z = velocity.z;
        else if (currentVelocity.z < -velocity.z)
            currentVelocity.z = -velocity.z;
    }
    else
    {
        // Camera is no longer moving along the z axis.
        // Linearly decelerate back to stationary state.

        if (currentVelocity.z > 0.0f)
        {
            if ((currentVelocity.z -= acceleration.z * elapsedTimeSec) < 0.0f)
                currentVelocity.z = 0.0f;
        }
        else
        {
            if ((currentVelocity.z += acceleration.z * elapsedTimeSec) > 0.0f)
                currentVelocity.z = 0.0f;
        }
    }
}


void BaseCameraController::onResize(int width, int height) {
    perspective(static_cast<float>(width)/static_cast<float>(height));
}

void BaseCameraController::setModel(const glm::mat4& model) {
    camera.model = model;
}

void BaseCameraController::push(VkCommandBuffer commandBuffer, VkPipelineLayout layout) const {
    vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Camera), &camera);
}

void BaseCameraController::push(VkCommandBuffer commandBuffer, VkPipelineLayout layout, const glm::mat4& model) {
    camera.model = model;
    vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Camera), &camera);
}
