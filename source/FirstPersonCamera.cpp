#include "FirstPersonCamera.h"

SpectatorCameraController::SpectatorCameraController(Camera &camera, InputManager &inputManager,  const CameraSettings &settings)
: CameraController(camera, inputManager, settings)
{

}

void SpectatorCameraController::update(float elapsedTime) {
    float dx = -mouse.relativePosition.x;
    float dy = -mouse.relativePosition.y;
    rotateSmoothly(dx, dy, 0.0f);
    updatePosition(direction, elapsedTime);
}


void SpectatorCameraController::rotate(float headingDegrees, float pitchDegrees, float rollDegrees) {
    // Implements the rotation logic for the first person style and
    // spectator style Camera modes. Roll is ignored.
    accumPitchDegrees += pitchDegrees;

    static std::vector<float> pRots;
    if (pitchDegrees != 0) pRots.push_back(pitchDegrees);

    if (accumPitchDegrees > 90.0f) {
        pitchDegrees = 90.0f - (accumPitchDegrees - pitchDegrees);
        accumPitchDegrees = 90.0f;
    }

    if (accumPitchDegrees < -90.0f) {
        pitchDegrees = -90.0f - (accumPitchDegrees - pitchDegrees);
        accumPitchDegrees = -90.0f;
    }

    glm::quat rot;

    // Rotate Camera about the world y axis.
    // Note the order the Orientationernions are multiplied. That is important!
    if (headingDegrees != 0.0f) {
        rot = fromAxisAngle(WORLD_YAXIS, headingDegrees); // TODO
        orientation = orientation * rot;

    }

    // Rotate Camera about its local x axis.
    // Note the order the Orientationernions are multiplied. That is important!
    if (pitchDegrees != 0.0f) {
        rot = fromAxisAngle(WORLD_XAXIS, pitchDegrees);
        orientation = rot * orientation;
    }
    updateViewMatrix();
}


FirstPersonCameraController::FirstPersonCameraController(Camera &camera, InputManager &inputManager, const CameraSettings &settings)
: SpectatorCameraController(camera, inputManager, settings)
{

}

void FirstPersonCameraController::move(float dx, float dy, float dz) {
    glm::vec3 eyes = this->eyes;

    // Calculate the forwards direction. Can't just use the Camera's local
    // z axis as doing so will cause the Camera to move more slowly as the
    // Camera's view approaches 90 degrees straight up and down.
    glm::vec3 forwards = normalize(cross(WORLD_YAXIS, xAxis));

    eyes += xAxis * dx;
    eyes += WORLD_YAXIS * dy;
    eyes += forwards * dz;

    position(eyes);
}
