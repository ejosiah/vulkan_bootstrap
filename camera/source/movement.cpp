#include "movement.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <vector>

using namespace jay::camera;

Movement::Movement(Camera2 &camera)
: _camera{camera}
{

}

Movement::~Movement() = default;

void Movement::lookAt(const glm::vec3 &eye, const glm::vec3 &target, const glm::vec3 &up) {
    _camera._position = eye;
    _camera._target = target;

    auto view = glm::lookAt(eye, target, up);
    _accumPitchDegrees = glm::degrees(asinf(view[1][2]));
    _orientation = glm::quat(view);
    updateViewMatrix();
}

void Movement::rotateSmoothly(float headingDegrees, float pitchDegrees, float rollDegrees) {
    headingDegrees *= _rotationSpeed;
    pitchDegrees *= _rotationSpeed;
    rollDegrees *= _rotationSpeed;

    rotate(headingDegrees, pitchDegrees, rollDegrees);
}

void Movement::rotate(float headingDegrees, float pitchDegrees, float rollDegrees) {}

void Movement::move(float dx, float dy, float dz) {
    if(dx == 0 && dy == 0 && dz == 0) return;   // TODO use close enough

    auto& pos = _camera._position;
    auto forwards = _camera._viewDir;

    pos += _camera._xAxis * dx;
    pos += WORLD_YAXIS * dy;
    pos += forwards * dz;

    positionUpdated();
    updateViewMatrix();
}

void Movement::move(const glm::vec3 &direction, const glm::vec3 &amount) {
    _camera._position += direction * amount;
    updateViewMatrix();
}

const Camera2 &Movement::camera() const {
    return _camera;
}

void Movement::updateViewMatrix() {
    auto& view = _camera._mvp.view;
    view = glm::mat4_cast(_orientation);

    _camera._xAxis = glm::vec3(glm::row(view, 0));
    _camera._yAxis = glm::vec3(glm::row(view, 1));
    _camera._zAxis = glm::vec3(glm::row(view, 2));
    _camera._viewDir = -_camera._zAxis;

    view[3][0] = -dot(_camera._xAxis, _camera._position);
    view[3][1] = -dot(_camera._yAxis, _camera._position);
    view[3][2] =  -dot(_camera._zAxis, _camera._position);
    _moved = true;
}

void Movement::undoRoll() {
    auto position = _camera._position;
    auto target = _camera._position + _camera._viewDir;
    lookAt(position, target, WORLD_YAXIS);
}

void Movement::positionUpdated() {

}

void Movement::acceleration(glm::vec3 value) {
    _acceleration = value;
}

const glm::vec3 &Movement::acceleration() const {
    return _acceleration;
}

void Movement::velocity(glm::vec3 value) {
    _velocity = value;
}

const glm::vec3 &Movement::velocity() const {
    return _velocity;
}


SpectatorMovement::SpectatorMovement(Camera2 &camera)
        : Movement(camera)
{

}

void Movement::update(float dt) {}

void Movement::updatePosition(const glm::vec3 &direction, float dt) {
    // Moves the Camera using Newton's second law of motion. Unit mass is
    // assumed here to somewhat simplify the calculations. The direction vector
    // is in the range [-1,1].
    using namespace glm;
    if (dot(_currentVelocity, _currentVelocity) != 0.0f)
    {
        // Only move the Camera if the _velocity vector is not of zero length.
        // Doing this guards against the Camera slowly creeping around due to
        // floating point rounding errors.

        glm::vec3 displacement = (_currentVelocity * dt) +
                                 (0.5f * _acceleration * dt * dt);

        // Floating point rounding errors will slowly accumulate and cause the
        // Camera to move along each axis. To prevent any unintended movement
        // the displacement vector is clamped to zero for each direction that
        // the Camera isn't moving in. Note that the updateVelocity() method
        // will slowly decelerate the Camera's _velocity back to a stationary
        // state when the Camera is no longer moving along that direction. To
        // account for this the Camera's current _velocity is also checked.

        static auto fEquals = [](float a, float b){ return glm::epsilonEqual(a, b, 1E-3f); };

        if (direction.x == 0.0f && fEquals(_currentVelocity.x, 0.0f))
            displacement.x = 0.0f;

        if (direction.y == 0.0f && fEquals(_currentVelocity.y, 0.0f))
            displacement.y = 0.0f;

        if (direction.z == 0.0f && fEquals(_currentVelocity.z, 0.0f))
            displacement.z = 0.0f;

        move(displacement.x, displacement.y, displacement.z);
    }

    // Continuously update the Camera's _velocity vector even if the Camera
    // hasn't moved during this call. When the Camera is no longer being moved
    // the Camera is decelerating back to its stationary state.

    updateVelocity(direction, dt);
}

void Movement::updateVelocity(const glm::vec3& direction, float dt) {
    // Updates the Camera's _velocity based on the supplied movement direction
    // and the elapsed time (since this method was last called). The movement
    // direction is in the range [-1,1].

    if (direction.x != 0.0f)
    {
        // Camera is moving along the x axis.
        // Linearly accelerate up to the Camera's max speed.

        _currentVelocity.x += direction.x * _acceleration.x * dt;

        if (_currentVelocity.x > _velocity.x)
            _currentVelocity.x = _velocity.x;
        else if (_currentVelocity.x < -_velocity.x)
            _currentVelocity.x = -_velocity.x;
    }
    else
    {
        // Camera is no longer moving along the x axis.
        // Linearly decelerate back to stationary state.

        if (_currentVelocity.x > 0.0f)
        {
            if ((_currentVelocity.x -= _acceleration.x * dt) < 0.0f)
                _currentVelocity.x = 0.0f;
        }
        else
        {
            if ((_currentVelocity.x += _acceleration.x * dt) > 0.0f)
                _currentVelocity.x = 0.0f;
        }
    }

    if (direction.y != 0.0f)
    {
        // Camera is moving along the y axis.
        // Linearly accelerate up to the Camera's max speed.

        _currentVelocity.y += direction.y * _acceleration.y * dt;

        if (_currentVelocity.y > _velocity.y)
            _currentVelocity.y = _velocity.y;
        else if (_currentVelocity.y < -_velocity.y)
            _currentVelocity.y = -_velocity.y;
    }
    else
    {
        // Camera is no longer moving along the y axis.
        // Linearly decelerate back to stationary state.

        if (_currentVelocity.y > 0.0f)
        {
            if ((_currentVelocity.y -= _acceleration.y * dt) < 0.0f)
                _currentVelocity.y = 0.0f;
        }
        else
        {
            if ((_currentVelocity.y += _acceleration.y * dt) > 0.0f)
                _currentVelocity.y = 0.0f;
        }
    }

    if (direction.z != 0.0f)
    {
        // Camera is moving along the z axis.
        // Linearly accelerate up to the Camera's max speed.

        _currentVelocity.z += direction.z * _acceleration.z * dt;

        if (_currentVelocity.z > _velocity.z)
            _currentVelocity.z = _velocity.z;
        else if (_currentVelocity.z < -_velocity.z)
            _currentVelocity.z = -_velocity.z;
    }
    else
    {
        // Camera is no longer moving along the z axis.
        // Linearly decelerate back to stationary state.

        if (_currentVelocity.z > 0.0f)
        {
            if ((_currentVelocity.z -= _acceleration.z * dt) < 0.0f)
                _currentVelocity.z = 0.0f;
        }
        else
        {
            if ((_currentVelocity.z += _acceleration.z * dt) > 0.0f)
                _currentVelocity.z = 0.0f;
        }
    }
}

void Movement::zoom(float amount) {
    amount = glm::min(glm::max(amount, _minZoom), _maxZoom);
    _camera.zoom(amount);
}

void Movement::pitch(float amount) {
    _pitch = amount;
}

void Movement::roll(float amount) {
    _roll = amount;
}

void Movement::yaw(float amount) {
    _yaw = amount;
}

void SpectatorMovement::rotate(float headingDegrees, float pitchDegrees, float rollDegrees) {
    if(headingDegrees == 0 && pitchDegrees == 0 && rollDegrees == 0){
        return;
    }
    // Implements the rotation logic for the first person style and
    // spectator style Camera modes. Roll is ignored.
    _accumPitchDegrees += pitchDegrees;

    static std::vector<float> pRots;
    if (pitchDegrees != 0) pRots.push_back(pitchDegrees);

    if (_accumPitchDegrees > 90.0f) {
        pitchDegrees = 90.0f - (_accumPitchDegrees - pitchDegrees);
        _accumPitchDegrees = 90.0f;
    }

    if (_accumPitchDegrees < -90.0f) {
        pitchDegrees = -90.0f - (_accumPitchDegrees - pitchDegrees);
        _accumPitchDegrees = -90.0f;
    }

    static glm::quat rot;

    // Rotate Camera about the world y axis.
    // Note the order the Orientationernions are multiplied. That is important!
    if (headingDegrees != 0.0f) {
        rot = glm::angleAxis(glm::radians(headingDegrees), WORLD_YAXIS);
        _orientation = _orientation * rot;

    }

    // Rotate Camera about its local x axis.
    // Note the order the Orientationernions are multiplied. That is important!
    if (pitchDegrees != 0.0f) {
        rot = glm::angleAxis(glm::radians(pitchDegrees), WORLD_XAXIS);
        _orientation = rot * _orientation;
    }
    updateViewMatrix();
}

void SpectatorMovement::update(float dt) {
    rotateSmoothly(_yaw, _pitch, 0.0f);
    updatePosition(_direction, dt);
}

FirstPersonMovement::FirstPersonMovement(Camera2 &camera)
        : Movement(camera)
{

}

void FirstPersonMovement::move(float dx, float dy, float dz) {
    if(dx == 0 && dy == 0 && dz == 0) return;
    auto& position = _camera._position;
    auto& xAxis = _camera._xAxis;
    // Calculate the forwards direction. Can't just use the Camera's local
    // z axis as doing so will cause the Camera to move more slowly as the
    // Camera's view approaches 90 degrees straight up and down.
    glm::vec3 forwards = normalize(cross(WORLD_YAXIS, xAxis));

    position += xAxis * dx;
    position += WORLD_YAXIS * dy;
    position += forwards * dz;

    positionUpdated();
    updateViewMatrix();
}

OrbitMovement::OrbitMovement(Camera2 &camera) : Movement(camera) {

}

void OrbitMovement::rotate(float headingDegrees, float pitchDegrees, float rollDegrees) {
    if(headingDegrees == 0 && pitchDegrees == 0 && rollDegrees == 0){
        return;
    }

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

    if (_preferTargetYAxisOrbiting)
    {
        if (headingDegrees != 0.0f)
        {
            rot = glm::angleAxis(glm::radians(headingDegrees), _targetYAxis);
            _orientation = _orientation *  rot;
        }

        if (pitchDegrees != 0.0f)
        {
            rot = glm::angleAxis(glm::radians(pitchDegrees), WORLD_XAXIS);
            _orientation = rot * _orientation;
        }
    }
    else
    {
        rot = glm::quat({ radians(pitchDegrees), radians(headingDegrees), radians(rollDegrees) });
        _orientation = rot * _orientation;
    }
    updateViewMatrix();
}

void OrbitMovement::move(float dx, float dy, float dz) {
    // Operation Not supported
}

void OrbitMovement::move(const glm::vec3 &direction, const glm::vec3 &amount) {
    // Operation Not supported
}

void OrbitMovement::positionUpdated() {
    auto newEyes = _camera._position + _camera._zAxis * _offsetDistance;
    auto newTarget = _camera._position;
    lookAt(newEyes, newTarget, _targetYAxis);
}

void OrbitMovement::update(float dt) {

    rotateSmoothly(_yaw, _pitch, 0.0f);

    if (!_preferTargetYAxisOrbiting) {
        float dz = dz = _direction.x * _orbitRollSpeed * dt;
        if (dz != 0.0f) {
            rotate(0.0f, 0.0f, dz);
        }
    }

    if (_zoomAmount != 0.0f) {
        zoom(_zoomAmount);
    }
}

void OrbitMovement::undoRoll() {
    lookAt(_camera._position, _camera._target, _targetYAxis);
}

void OrbitMovement::updateViewMatrix() {
    auto& view = _camera._mvp.view;
    view = glm::mat4_cast(_orientation);

    _camera._xAxis = glm::vec3(glm::row(view, 0));
    _camera._yAxis = glm::vec3(glm::row(view, 1));
    _camera._zAxis = glm::vec3(glm::row(view, 2));
    _camera._viewDir = -_camera._zAxis;

    auto& position = _camera._position;
    position = _camera._target + _camera._zAxis * _offsetDistance;

    view[3][0] = -dot(_camera._xAxis, position);
    view[3][1] = -dot(_camera._yAxis, position);
    view[3][2] =  -dot(_camera._zAxis, position);
    _moved = true;
}

void OrbitMovement::zoom(float amount) {
    glm::vec3 offset = _camera._target - _camera._target;

    _offsetDistance = glm::length(offset);
    offset = normalize(offset);
    _offsetDistance += amount;
    _offsetDistance = std::min(std::max(_offsetDistance, _minZoom), _maxZoom);

    offset *= _offsetDistance;
    _camera._position = offset + _camera._target;

    updateViewMatrix();
}

void OrbitMovement::updateSubject(const glm::vec3 &position, const glm::quat &orientation) {
    _subject.orientation = glm::inverse(orientation);
    _subject.position = position;
}