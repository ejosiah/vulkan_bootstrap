#include "Camera.h"

#include <memory>

void configureBase(const CameraSettings& settings);
void configureBase(const CameraSettings& settings, BaseCameraSettings& baseSettings);

CameraController::CameraController(const VulkanDevice &device, uint32_t swapChainImageCount,
                                   const uint32_t &currentImageIndex, InputManager &inputManager,
                                   const CameraSettings &settings)
    : currentMode(settings.mode)
    , firstPerson(inputManager.mapToKey(Key::_1, "First Person mode", Action::detectInitialPressOnly()))
    , spectator(inputManager.mapToKey(Key::_2, "Spectator mode", Action::detectInitialPressOnly()))
    , flight(inputManager.mapToKey(Key::_3, "Flight mode", Action::detectInitialPressOnly()))
    , orbit(inputManager.mapToKey(Key::_4, "Orbit Person mode", Action::detectInitialPressOnly()))
{
    configureBase(settings);

    cameras[CameraMode::FIRST_PERSON] = std::make_unique<FirstPersonCameraController>(device, swapChainImageCount, currentImageIndex, inputManager, settings.firstPerson);
    cameras[CameraMode::SPECTATOR] = std::make_unique<SpectatorCameraController>(device, swapChainImageCount, currentImageIndex, inputManager, settings.firstPerson);
    cameras[CameraMode::FLIGHT] = std::make_unique<FlightCameraController>(device, swapChainImageCount, currentImageIndex, inputManager, settings.flight);
    cameras[CameraMode::ORBIT] = std::make_unique<OrbitingCameraController>(device, swapChainImageCount, currentImageIndex, inputManager, settings.orbit);
}

inline void configureBase(const CameraSettings& settings){
    configureBase(settings, (BaseCameraSettings &) const_cast<const OrbitingCameraSettings &>(settings.orbit));
    configureBase(settings, (BaseCameraSettings &) const_cast<const FirstPersonSpectatorCameraSettings &>(settings.firstPerson));
    configureBase(settings, (BaseCameraSettings &) const_cast<const FlightCameraSettings &>(settings.flight));
    configureBase(settings, (BaseCameraSettings &) const_cast<const OrbitingCameraSettings &>(settings.orbit));
}

inline void configureBase(const CameraSettings& settings, BaseCameraSettings& baseSettings){
    baseSettings.acceleration = settings.acceleration;
    baseSettings.velocity  = settings.velocity;
    baseSettings.rotationSpeed = settings.rotationSpeed;
    baseSettings.fieldOfView  = settings.fieldOfView;
    baseSettings.aspectRatio  = settings.aspectRatio;
    baseSettings.zNear  = settings.zNear;
    baseSettings.zFar  = settings.zFar;
    baseSettings.minZoom  = settings.minZoom;
    baseSettings.maxZoom  = settings.maxZoom;
    baseSettings.floorOffset  = settings.floorOffset;
    baseSettings.handleZoom  = settings.handleZoom;
    baseSettings.horizontalFov = settings.horizontalFov;
}

void CameraController::update(float time) {
    cameras[currentMode]->update(time);
}

void CameraController::processInput() {

    if(firstPerson.isPressed()){
        setMode(CameraMode::FIRST_PERSON);
    }
    if(spectator.isPressed()){
        setMode(CameraMode::SPECTATOR);
    }
    if(flight.isPressed()){
        setMode(CameraMode::FLIGHT);
    }
    if(orbit.isPressed()){
        setMode(CameraMode::ORBIT);
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
    for(auto& [_, cam] : cameras){
        cam->perspective(aspect);
    }
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
    for(auto& [_, cam] : cameras){
        cam->onResize(width, height);
    }
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

const glm::vec3 &CameraController::position() const {
    return cameras[currentMode]->position();
}

const glm::vec3 &CameraController::velocity() const {
    return cameras[currentMode]->velocity();
}

const glm::vec3 &CameraController::acceleration() const {
    return cameras[currentMode]->acceleration();
}

void CameraController::setMode(CameraMode mode) {
    auto prevMode = currentMode;
    currentMode = mode;

    if(prevMode == CameraMode::NONE){
        return;
    }

    auto newPos = cameras[prevMode]->position();
    switch(currentMode){
        case CameraMode::FIRST_PERSON:
            newPos.y = cameras[currentMode]->position().y;
            cameras[currentMode]->position(newPos);
            break;
        case CameraMode::SPECTATOR:
        case CameraMode::FLIGHT:
            if(prevMode != CameraMode::ORBIT){
                cameras[currentMode]->position(newPos);
            }
            break;
        case CameraMode::ORBIT: {
            auto yAxis = cameras[prevMode]->getYAxis();
            auto orbitCam = dynamic_cast<OrbitingCameraController *>(cameras[currentMode].get());
            orbitCam->updateModel(newPos);
            orbitCam->setTargetYAxis(yAxis);
            orbitCam->position(newPos);
            orbitCam->rotate(0.0f, -30.0f, 0.0f);
            auto pos = cameras[currentMode]->position();
            break;
        }
        case CameraMode::NONE:
            currentMode = CameraMode::SPECTATOR;
            break;
    }
}

inline bool CameraController::isInFirstPersonMode() const {
    return currentMode == CameraMode::FIRST_PERSON;
}


inline bool CameraController::isInFlightMode() const {
    return currentMode == CameraMode::FLIGHT;
}

inline bool CameraController::isInSpectatorMode() const {
    return currentMode == CameraMode::SPECTATOR;
}

bool CameraController::isInObitMode() const {
    return currentMode == CameraMode::ORBIT;
}

const glm::quat &CameraController::getOrientation() const {
    return cameras[currentMode]->getOrientation();
}
