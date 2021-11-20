#pragma once

#include "camera_v2.hpp"
#include "movement.hpp"

namespace jay::camera {

    struct Button{
        bool pressed;
        bool held;
    };

    struct Rotate{
        enum class Type{ YAW, PITCH, ROLL};
        Type type;
        float amount{0};
    };

    class Input {
    public:
        Input(Movement &movement);

        virtual void processInput() = 0;

    protected:
        Movement &_movement;
    };

    class CharacterInput : public Input{
    public:
        Button _forward;
        Button _back;
        Button _left;
        Button _right;
        Button _up;
        Button _down;

        Button _zoomIn;
        Button _zoomOut;

        Rotate _yaw{Rotate::Type::YAW};
        Rotate _pitch{Rotate::Type::PITCH};
        Rotate _roll{Rotate::Type::ROLL};

        explicit CharacterInput(Movement& movement);

        void processInput() override;

    private:
        void processRotateInput();

        void processMovementInput();

        void processZoomInput();

        bool _handleZoom{false};
    };
}