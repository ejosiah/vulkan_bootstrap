#include "cameraadaptor.hpp"

using namespace jay::camera;


CharacterInputAdaptor::CharacterInputAdaptor(Movement &movement, InputManager &inputManager)
: CharacterInput(movement)
, _mouse(inputManager.getMouse())
, _aForward(&_forward, &inputManager.mapToKey(Key::W, "forward", Action::Behavior::DETECT_INITIAL_PRESS_ONLY))
, _aBack(&_back, &inputManager.mapToKey(Key::S, "backward", Action::Behavior::DETECT_INITIAL_PRESS_ONLY))
, _aLeft(&_left, &inputManager.mapToKey(Key::A, "left", Action::Behavior::DETECT_INITIAL_PRESS_ONLY))
, _aRight(&_right, &inputManager.mapToKey(Key::D, "right", Action::Behavior::DETECT_INITIAL_PRESS_ONLY))
, _aUp(&_up, &inputManager.mapToKey(Key::E, "up", Action::Behavior::DETECT_INITIAL_PRESS_ONLY))
, _aDown(&_down, &inputManager.mapToKey(Key::Q, "down", Action::Behavior::DETECT_INITIAL_PRESS_ONLY))
, _aZoomIn(&_zoomIn, &inputManager.mapToMouse(MouseEvent::MoveCode::WHEEL_UP))
, _aZoomOut(&_zoomOut, &inputManager.mapToMouse(MouseEvent::MoveCode::WHEEL_DOWN))
{

}

void CharacterInputAdaptor::processInput() {
    _yaw.amount = -_mouse.relativePosition.x;
    _pitch.amount = -_mouse.relativePosition.y;

    _aForward.fire();
    _aBack.fire();
    _aLeft.fire();
    _aRight.fire();
    _aUp.fire();
    _aDown.fire();
    _aZoomIn.fire();
    _aZoomOut.fire();

    CharacterInput::processInput();
}

void CharacterInputAdaptor::ButtonAdaptor::fire()  {
    _button->pressed = _action->isPressed();
    _button->held = _action->isHeld();
}

CharacterInputAdaptor::ButtonAdaptor::ButtonAdaptor(jay::camera::Button* button,  Action *action)
:_button(button)
,_action(action){

}
