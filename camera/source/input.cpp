#include "input.hpp"

using namespace jay::camera;

Input::Input(Movement &movement)
        :_movement(movement)
{

}

CharacterInput::CharacterInput(Movement &movement) : Input(movement) {

}

void CharacterInput::processInput() {
    processRotateInput();
    processMovementInput();
    processZoomInput();
}

void CharacterInput::processRotateInput() {
    _movement.yaw(_yaw.amount);
    _movement.pitch(_pitch.amount);
    _movement.roll(_roll.amount);
}

void CharacterInput::processMovementInput() {
    _movement._direction = glm::vec3(0);
    auto vel = _movement._currentVelocity;
    if(_forward.pressed){
        _movement._currentVelocity.x = vel.x;
        _movement._currentVelocity.y = vel.y;
        _movement._currentVelocity.z = 0;
    }else if(_forward.held){
        _movement._direction.z += 1.0f;
    }

    if(_back.pressed){
        _movement._currentVelocity.x = vel.x;
        _movement._currentVelocity.y = vel.y;
        _movement._currentVelocity.z = 0;
    }else if(_back.held){
        _movement._direction.z -= 1.0f;
    }

    if(_right.pressed){
        _movement._currentVelocity.x = 0;
        _movement._currentVelocity.y = vel.y;
        _movement._currentVelocity.z = vel.z;
    }else if(_right.held){
        _movement._direction.x += 1.0f;
    }

    if(_left.pressed){
        _movement._currentVelocity.x = 0;
        _movement._currentVelocity.y = vel.y;
        _movement._currentVelocity.z = vel.z;
    }else if(_left.held){
        _movement._direction.x -= 1.0f;
    }


    if(_up.pressed){
        _movement._currentVelocity.x = vel.x;
        _movement._currentVelocity.y = 0.0f;
        _movement._currentVelocity.z = vel.z;
    }else if(_up.held){
        _movement._direction.y += 1.0f;
    }

    if(_down.pressed){
        _movement._currentVelocity.x = vel.x;
        _movement._currentVelocity.y = 0.0f;
        _movement._currentVelocity.z = vel.z;
    }else if(_down.held){
        _movement._direction.y -= 1.0f;
    }
}

void CharacterInput::processZoomInput() {
    float amount = 0.0f;
    if(_zoomIn.pressed){
        amount = -0.1;
    }else if(_zoomOut.pressed){
        amount = 0.1;
    }
    if(_handleZoom && amount != 0){
        _movement.zoom(amount);
    }
}



