#include "Camera.h"

#include <memory>

CameraController::CameraController(const VulkanDevice &device, uint32_t swapChainImageCount,
                                   const uint32_t &currentImageIndex, InputManager &inputManager,
                                   const CameraSettings &settings)
    : currentMode(settings.mode)
    , firstPerson(inputManager.mapToKey(Key::_1, "First Person mode", Action::detectInitialPressOnly()))
    , spectator(inputManager.mapToKey(Key::_2, "Spectator mode", Action::detectInitialPressOnly()))
    , flight(inputManager.mapToKey(Key::_3, "Flight mode", Action::detectInitialPressOnly()))
    , orbit(inputManager.mapToKey(Key::_4, "Orbit Person mode", Action::detectInitialPressOnly()))
{
    cameras[CameraMode::FIRST_PERSON] = std::make_unique<FirstPersonCameraController>(device, swapChainImageCount, currentImageIndex, inputManager, settings);
    cameras[CameraMode::SPECTATOR] = std::make_unique<SpectatorCameraController>(device, swapChainImageCount, currentImageIndex, inputManager, settings);
    cameras[CameraMode::FLIGHT] = std::make_unique<FlightCameraController>(device, swapChainImageCount, currentImageIndex, inputManager, settings);
    cameras[CameraMode::ORBIT] = std::make_unique<OrbitingCameraController>(device, swapChainImageCount, currentImageIndex, inputManager, settings);
}

void CameraController::update(float time) {
    cameras[currentMode]->update(time);
}

void CameraController::processInput() {
    auto prevMode = currentMode;
    auto curPos = cameras[currentMode]->position();
    if(firstPerson.isPressed()){
        currentMode = CameraMode::FIRST_PERSON;
        if(prevMode == CameraMode::ORBIT){
            curPos.xy = cameras[currentMode]->position().xy;
        }
        curPos.y = cameras[currentMode]->position().y;
        cameras[currentMode]->position(curPos);

    }
    if(spectator.isPressed()){
        currentMode = CameraMode::SPECTATOR;
        if(prevMode == CameraMode::ORBIT){
            curPos = cameras[currentMode]->position();
        }
        cameras[currentMode]->position(curPos);
    }
    if(flight.isPressed()){
        currentMode = CameraMode::FLIGHT;
        if(prevMode == CameraMode::ORBIT){
            curPos = cameras[currentMode]->position();
        }
        cameras[currentMode]->position(curPos);
    }
    if(orbit.isPressed()){
        currentMode = CameraMode::ORBIT;
        cameras[currentMode]->position(curPos);
        cameras[currentMode]->rotate(0.0f, -30.0f, 0.0f);
    }

    cameras[currentMode]->processInput();
}

void CameraController::lookAt(const glm::vec3 &eye, const glm::vec3 &target, const glm::vec3 &up) {
    cameras[currentMode]->lookAt(eye, target, up);
}

void CameraController::perspective(float fovx, float aspect, float znear, float zfar) {
    cameras[currentMode]->perspective(fovx, aspect, znear, zfar);
}

void CameraController::perspective(float aspect) {
    cameras[currentMode]->perspective(aspect);
}

void CameraController::rotateSmoothly(float headingDegrees, float pitchDegrees, float rollDegrees) {
    cameras[currentMode]->rotateSmoothly(headingDegrees, pitchDegrees, rollDegrees);
}

void CameraController::rotate(float headingDegrees, float pitchDegrees, float rollDegrees) {
    cameras[currentMode]->rotate(headingDegrees, pitchDegrees, rollDegrees);
}

void CameraController::move(float dx, float dy, float dz) {
    cameras[currentMode]->move(dx, dy, dz);
}

void CameraController::move(const glm::vec3 &direction, const glm::vec3 &amount) {
    cameras[currentMode]->move(direction, amount);
}

void CameraController::position(const glm::vec3 &pos) {
    cameras[currentMode]->position(pos);
}

void CameraController::updatePosition(const glm::vec3 &direction, float elapsedTimeSec) {
    cameras[currentMode]->updatePosition(direction, elapsedTimeSec);
}

void CameraController::undoRoll() {
    cameras[currentMode]->undoRoll();
}

void CameraController::zoom(float zoom, float minZoom, float maxZoom) {
    cameras[currentMode]->zoom(zoom, minZoom, maxZoom);
}

void CameraController::onResize(int width, int height) {
    cameras[currentMode]->onResize(width, height);
}

void CameraController::setModel(const glm::mat4 &model) {
    cameras[currentMode]->setModel(model);
}


void CameraController::push(VkCommandBuffer commandBuffer, VkPipelineLayout layout, const glm::mat4 &model) {
    cameras[currentMode]->push(commandBuffer, layout, model);
}

void CameraController::push(VkCommandBuffer commandBuffer, VkPipelineLayout layout) const {
    cameras[currentMode]->push(commandBuffer, layout);
}

const Camera &CameraController::cam() const {
    return cameras[currentMode]->cam();
}

std::string CameraController::mode() const {
    switch(currentMode){
        case CameraMode::FIRST_PERSON: return "First Person";
        case CameraMode::SPECTATOR: return "Spectator";
        case CameraMode::FLIGHT: return "Flight";
        case CameraMode::ORBIT: return "Orbit";
        default: return "Unknown";
    }
}
