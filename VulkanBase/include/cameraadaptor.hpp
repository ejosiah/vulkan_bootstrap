#pragma once

#include "cameras.hpp"
#include "InputManager.h"

class CharacterInputAdaptor : public jay::camera::CharacterInput{
public:
    CharacterInputAdaptor(jay::camera::Movement& movement, InputManager& inputManager);

    void processInput() final;

private:
    class ButtonAdaptor{
    public:
        explicit ButtonAdaptor(jay::camera::Button* button = nullptr, Action* action = nullptr);

        void fire();
    private:
        jay::camera::Button* _button;
        Action* _action;
    };
    ButtonAdaptor _aForward;
    ButtonAdaptor _aBack;
    ButtonAdaptor _aLeft;
    ButtonAdaptor _aRight;
    ButtonAdaptor _aUp;
    ButtonAdaptor _aDown;
    ButtonAdaptor _aZoomIn;
    ButtonAdaptor _aZoomOut;
    const Mouse& _mouse;
};