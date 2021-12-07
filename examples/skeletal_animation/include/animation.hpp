#pragma once

#include "common.h"
#include "model.hpp"

namespace anim{

    template<T = glm::mat4>
    struct KeyFrame{
        T value{};
        double time{};
    };

    using Translation = KeyFrame;
    using Scale = KeyFrame;
    using QRotation = KeyFrame<glm::quat>;
    using Tick = double;

    struct BoneNode{
        int id;
        std::string name;
        std::vector<int> children;
    };

    struct KeyFrame{
        Scale scale;
        Translation translation;
        QRotation rotation;
        std::string name;
    };

    struct BoneAnimation{
        std::string name;
        std::vector<Translation> translationKeys;
        std::vector<Scale> scaleKeys;
        std::vector<QRotation> rotationKeys;
    };

    struct Animation{
        std::shared_ptr<Model> model;
        std::shared_ptr<std::vector<BoneAnimation>> channels;
        std::shared_ptr<std::unordered_map<id, BoneNode>> BoneHierarchy;
        std::shared_ptr<std::vector<BoneNode>> boneNodes;
        Tick duration{0};
        double ticksPerSecond{25};

        void update(float time);
    };

    struct Character{
        std::shared_ptr<Model> model;
        std::shared_ptr<std::vector<BoneAnimation>> channels;
        std::shared_ptr<std::unordered_map<id, BoneNode>> BoneHierarchy;
        std::shared_ptr<std::vector<BoneNode>> boneNodes;
        std::map<std::string, Animation> animations;
        std::string currentAnimation;

        void update(float time);
    };

    void load(Character& character, std::string path);
}