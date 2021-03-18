#include "OrbitingCamera.h"

OrbitingCameraController::OrbitingCameraController(Camera &camera, InputManager& inputManager,  const OrbitingCameraSettings& settings)
: CameraController(camera, inputManager, settings)
, offsetDistance(settings.offsetDistance)
, orbitRollSpeed(settings.orbitRollSpeed)
, preferTargetYAxisOrbiting(settings.preferTargetYAxisOrbiting)
, zoomIn(inputManager.mapToMouse(MouseEvent::MoveCode::WHEEL_UP))
, zoomOut(inputManager.mapToMouse(MouseEvent::MoveCode::WHEEL_DOWN))
{
    minZoom = settings.orbitMinZoom;
    maxZoom = settings.orbitMaxZoom;
    offsetDistance = settings.offsetDistance;
    floorOffset = settings.modelHeight * 0.5f;
    position({0.0f, floorOffset, 0.0f});
    updateModel();
    auto eyes = this->eyes + zAxis * offsetDistance;
    auto target = this->eyes;
    lookAt(eyes, target, targetYAxis);
}

void OrbitingCameraController::update(float elapsedTime) {
    float dx = mouse.relativePosition.x;
    float dy = mouse.relativePosition.y;

    rotateSmoothly(dx, dy, 0.0f);

    if (!preferTargetYAxisOrbiting) {
        float dz = dz = direction.x * orbitRollSpeed * elapsedTime;
        if (dz != 0.0f) {
            rotate(0.0f, 0.0f, dz);
        }
    }

    if (zoomAmount != 0.0f) {
        zoom(zoomAmount, minZoom, maxZoom);
    }
}

void OrbitingCameraController::processInput() {
    if(zoomIn.isPressed()){
        zoomAmount = -0.01;
    }else if(zoomOut.isPressed()){
        zoomAmount = 0.01;
    }else{
        zoomAmount = 0.0f;
    }
}


void OrbitingCameraController::move(float dx, float dy, float dz) {
    // Operation Not supported
}

void OrbitingCameraController::move(const glm::vec3 &direction, const glm::vec3 &amount) {
    // Operation Not supported
}

void OrbitingCameraController::undoRoll() {
    lookAt(eyes, target, targetYAxis);
}

void OrbitingCameraController::zoom(float zoom, float minZoom, float maxZoom) {
    // Moves the Camera closer to or further away from the orbit
    // target. The zoom amounts are in world units.

    this->maxZoom = maxZoom;
    this->minZoom = minZoom;

    glm::vec3 offset = eyes - target;

    offsetDistance = glm::length(offset);
    offset = normalize(offset);
    offsetDistance += zoom;
    offsetDistance = std::min(std::max(offsetDistance, minZoom), maxZoom);

    offset *= offsetDistance;
    eyes = offset + target;

    updateViewMatrix();
}

void OrbitingCameraController::rotate(float headingDegrees, float pitchDegrees, float rollDegrees) {
    // Implements the rotation logic for the orbit style Camera mode.
    // Roll is ignored for target Y axis orbiting.
    //
    // Briefly here's how this orbit Camera implementation works. Switching to
    // the orbit Camera mode via the setBehavior() method will set the
    // Camera's orientation to match the orbit target's orientation. Calls to
    // rotateOrbit() will rotate this orientation. To turn this into a third
    // person style view the updateViewMatrix() method will move the Camera
    // position back 'orbitOffsetDistance' world units along the Camera's
    // local z axis from the orbit target's world position.
    pitchDegrees = -pitchDegrees;
    headingDegrees = -headingDegrees;
    rollDegrees = -rollDegrees;

    using namespace glm;
    glm::quat rot;

    if (preferTargetYAxisOrbiting)
    {
        if (headingDegrees != 0.0f)
        {
            rot = fromAxisAngle(targetYAxis, headingDegrees);
            orientation =  orientation * rot;
        }

        if (pitchDegrees != 0.0f)
        {
            rot = fromAxisAngle(WORLD_XAXIS, pitchDegrees);
            orientation = rot * orientation;
        }
    }
    else
    {
        rot = glm::quat({ radians(pitchDegrees), radians(headingDegrees), radians(rollDegrees) });
        orientation = rot * orientation;
    }
    updateViewMatrix();
}

void OrbitingCameraController::updateModel() {
    camera.model = glm::mat4_cast(glm::inverse(orientation));
    camera.model = glm::translate(camera.model, eyes);
}

void OrbitingCameraController::updateViewMatrix() {
    auto& view = camera.view;
    view = glm::mat4_cast(orientation);

    xAxis = glm::vec3(glm::row(view, 0));
    yAxis = glm::vec3(glm::row(view, 1));
    zAxis = glm::vec3(glm::row(view, 2));
    viewDir = -zAxis;

    eyes = target + zAxis * offsetDistance;

    view[3][0] = -dot(xAxis, eyes);
    view[3][1] = -dot(yAxis, eyes);
    view[3][2] =  -dot(zAxis, eyes);
}
