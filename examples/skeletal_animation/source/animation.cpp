#include "animation.hpp"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include "glm_assimp_interpreter.hpp"

std::vector<anim::Animation> anim::load(mdl::Model* model, const std::string &path, uint32_t flags) {
    Assimp::Importer importer;
    const auto scene = importer.ReadFile(path.data(), flags);

    std::vector<Animation> animations;
    for(int i = 0; i < scene->mNumAnimations; i++){
        const auto aiAnim = scene->mAnimations[i];
        Animation animation{aiAnim->mName.C_Str(), float(aiAnim->mDuration), float(aiAnim->mTicksPerSecond), model };

        for(auto j = 0; j < aiAnim->mNumChannels; j++){
            const auto channel =aiAnim->mChannels[j];
            BoneAnimation boneAnimation{ channel->mNodeName.C_Str() };

            for(auto k = 0; k < channel->mNumPositionKeys; k++){
                const auto key = channel->mPositionKeys[k];
                Translation translation{ to_vec3(key.mValue), float(key.mTime) };
                boneAnimation.translationKeys.push_back(translation);
            }
            for(auto k = 0; k < channel->mNumScalingKeys; k++){
                const auto key = channel->mScalingKeys;
                Scale scale{ to_vec3(key->mValue), float(key->mTime) };
                boneAnimation.scaleKeys.push_back(scale);
            }

            for(auto k = 0; k < channel->mNumRotationKeys; k++){
                const auto key = channel->mRotationKeys;
                QRotation rotation{to_quat(key->mValue), float(key->mTime) };
                boneAnimation.rotationKeys.push_back(rotation);
            }
            animation.channels[boneAnimation.name] = boneAnimation;
        }
        animations.push_back(animation);
    }
    return animations;
}

void anim::Animation::update(float time) {
    assert(model);

    static auto applyParentTransforms = [&](mdl::Bone& bone){
        glm::mat4 transform{1};
        auto parentId = bone.parent;
        while(parentId != mdl::NULL_BONE){
            transform = model->bones[bone.parent].transform * transform;
            parentId = model->bones[bone.parent].parent;
        }
        return transform;
    };

    auto tick = glm::mod(time * ticksPerSecond, duration);
    auto& bones = model->bones;

    // Bone hierarchy is in reverse
    // so leave bones are processed last in the hierarchy
    for(auto i = bones.size() - 1; i >= 0; i--){
        auto& bone = bones[i];
        auto transform = bone.transform;
        auto itr = channels.find(bone.name);
        if(itr != channels.end()) {
            auto& animation = itr->second;
            auto position = interpolateTranslation(animation, tick);
            auto scale = interpolateScale(animation, tick);
            auto qRotate = interpolateRotation(animation, tick);
            auto rotate = glm::mat4(qRotate);

            transform =  glm::translate(transform, position) * rotate * glm::scale(transform, scale);

        }

        auto globalInverseXform = model->globalInverseTransform;
        auto transforms = reinterpret_cast<glm::mat4*>(model->buffers.boneTransforms.map());
        transforms[i] = globalInverseXform * applyParentTransforms(bone) * transform * bone.offsetMatrix;
    }
}

glm::vec3 anim::Animation::interpolateTranslation(const anim::BoneAnimation &boneAnimation, anim::Tick tick) {
    if(boneAnimation.translationKeys.size() == 1){
        return boneAnimation.translationKeys[0].value;
    }

    auto index = boneAnimation.translationKey(tick);

    auto currentKeyFrame = boneAnimation.translationKeys[index];
    auto nextKeyFrame = boneAnimation.translationKeys[index + 1];

    auto length = nextKeyFrame.tick - currentKeyFrame.tick;
    auto position = tick - currentKeyFrame.tick;
    float t = position/length;
    assert(t >= 0.f && t <= 1.f);
    return glm::mix(currentKeyFrame.value, nextKeyFrame.value, t);
}

glm::vec3 anim::Animation::interpolateScale(const anim::BoneAnimation &boneAnimation, anim::Tick tick) {
    if(boneAnimation.scaleKeys.size() == 1){
        return boneAnimation.scaleKeys[0].value;
    }

    auto index = boneAnimation.scaleKey(tick);

    auto currentKeyFrame = boneAnimation.scaleKeys[index];
    auto nextKeyFrame = boneAnimation.scaleKeys[index + 1];

    auto length = nextKeyFrame.tick - currentKeyFrame.tick;
    auto position = tick - currentKeyFrame.tick;
    float t = position/length;
    assert(t >= 0.f && t <= 1.f);
    return glm::mix(currentKeyFrame.value, nextKeyFrame.value, t);
}

glm::quat anim::Animation::interpolateRotation(const anim::BoneAnimation &boneAnimation, anim::Tick tick) {
    if(boneAnimation.rotationKeys.size() == 1){
        return boneAnimation.rotationKeys[0].value;
    }

    auto index = boneAnimation.rotationKey(tick);

    auto currentKeyFrame = boneAnimation.rotationKeys[index];
    auto nextKeyFrame = boneAnimation.rotationKeys[index + 1];

    auto length = nextKeyFrame.tick - currentKeyFrame.tick;
    auto position = tick - currentKeyFrame.tick;
    float t = position/length;
    assert(t >= 0.f && t <= 1.f);

    auto rotation = glm::slerp(currentKeyFrame.value, nextKeyFrame.value, t);

    return glm::normalize(rotation);
}

int anim::BoneAnimation::translationKey(anim::Tick tick) const {
    for(auto i = 0; i < translationKeys.size() - 1; i++){
        if(tick < translationKeys[i + 1].tick){
            return i;
        }
    }
    return -1;
}

int anim::BoneAnimation::scaleKey(anim::Tick tick) const {
    for(auto i = 0; i < scaleKeys.size() - 1; i++){
        if(tick < scaleKeys[i + 1].tick){
            return i;
        }
    }
    return -1;
}

int anim::BoneAnimation::rotationKey(anim::Tick tick) const {
    for(auto i = 0; i < rotationKeys.size() - 1; i++){
        if(tick < rotationKeys[i + 1].tick){
            return i;
        }
    }
    return -1;
}
