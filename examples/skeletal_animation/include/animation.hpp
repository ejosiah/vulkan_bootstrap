#pragma once

#include "common.h"
#include "model.hpp"

namespace anim{

    using Tick = float;

    template<typename T>
    struct KeyFrame{
        T value{};
        Tick tick{};
    };

    using Translation = KeyFrame<glm::vec3>;
    using Scale = KeyFrame<glm::vec3>;
    using QRotation = KeyFrame<glm::quat>;


    struct BoneAnimation{
        std::string name;
        std::vector<Translation> translationKeys;
        std::vector<Scale> scaleKeys;
        std::vector<QRotation> rotationKeys;

        [[nodiscard]]
        int translationKey(Tick tick) const;

        [[nodiscard]]
        int scaleKey(Tick tick) const;

        [[nodiscard]]
        int rotationKey(Tick tick) const;
    };

    struct AnimationNode{
        int id{-1};
        std::string name;
        glm::mat4 transform{1};
        int parentId{-1};
        glm::mat4 globalTransform{1};
        std::vector<int> children;
    };

    struct AnimTransforms{
        glm::vec3 position{0};
        glm::vec3 scale{1};
        glm::quat rotation{1, 0, 0, 0};
    };

    struct AnimationNodes{
        std::vector<int> ids;
        std::vector<std::string> names;
        std::vector<glm::mat4> baseTransforms;
        std::vector<glm::mat4> nodeTransforms;
        std::vector<int> parentIds;
        std::vector<glm::mat4> globalTransforms;
        std::vector<std::vector<int>> children;
        std::vector<glm::mat4> finalTransforms;
        std::vector<std::vector<Translation>> translationKeys;
        std::vector<std::vector<Scale>> scaleKeys;
        std::vector<std::vector<QRotation>> rotationKeys;
        std::vector<AnimTransforms> transforms;
        std::vector<bool> hasBone;
        std::vector<glm::mat4> offsetMatrix;
    };

    struct Animation{
        std::string name;
        Tick duration{0};
        float ticksPerSecond{25};
        mdl::Model* model{nullptr};
        std::unordered_map<std::string, BoneAnimation> channels;
        std::vector<AnimationNode> nodes;
        AnimationNodes animNodes;
        float elapsedTimeInSeconds{0};

        void update_dod(float time);

        void update(float time);

        void update0(float time);

    private:
        glm::vec3 interpolateTranslation(const BoneAnimation& boneAnimation, Tick tick);

        glm::vec3 interpolateScale(const BoneAnimation& boneAnimation, Tick tick);

        glm::quat interpolateRotation(const BoneAnimation& boneAnimation, Tick tick);
    };

    struct Character{
        mdl::Model model;
        std::unordered_map<std::string, Animation> animations;
        std::string currentAnimation;

        void update(float time);
    };

    std::vector<Animation> load(mdl::Model* model, const std::string& path, uint32_t flags = mdl::DEFAULT_PROCESS_FLAGS);
}