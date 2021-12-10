#include "animation.hpp"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include "glm_assimp_interpreter.hpp"

void loadNodes(anim::Animation& animation, const aiNode* aiNode){
    int id = int(animation.nodes.size());
    std::string name = aiNode->mName.C_Str();
    glm::mat4 transform = to_mat4(aiNode->mTransformation);

    anim::AnimationNode node{id, name, transform};
    animation.nodes.push_back(node);
    for(int i = 0; i < aiNode->mNumChildren; i++){
        auto childNode = aiNode->mChildren[i];
        loadNodes(animation, childNode);
    }
}

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
                const auto key = channel->mScalingKeys[k];

                Scale scale{ to_vec3(key.mValue), float(key.mTime) };
                boneAnimation.scaleKeys.push_back(scale);
            }

            for(auto k = 0; k < channel->mNumRotationKeys; k++){
                const auto key = channel->mRotationKeys[k];
                QRotation rotation{to_quat(key.mValue), float(key.mTime) };
                boneAnimation.rotationKeys.push_back(rotation);
            }
            animation.channels[boneAnimation.name] = boneAnimation;
        }
        loadNodes(animation, scene->mRootNode);
        animations.push_back(animation);
    }
    return animations;
}

void anim::Animation::update(float time) {
    elapsedTimeInSeconds += time;
    assert(model);

    static auto applyParentTransforms = [&](mdl::Bone& bone){
        glm::mat4 transform{1};
        auto parentId = bone.parent;
        while(parentId != mdl::NULL_BONE){
            transform = model->bones[parentId].transform * transform;
            parentId = model->bones[parentId].parent;
        }
        return transform;
    };

    auto tick = glm::mod(elapsedTimeInSeconds * ticksPerSecond, duration);
    spdlog::info("tick: {}", tick);
    auto& bones = model->bones;
    auto transforms = reinterpret_cast<glm::mat4*>(model->buffers.boneTransforms.map());
    // Bone hierarchy is in reverse
    // so leave bones are processed last in the hierarchy
    for(int i = bones.size() - 1; i >= 0; i--){
        auto& bone = bones[i];
        auto transform = bone.transform;
        auto itr = channels.find(bone.name);
        if(itr != channels.end()) {
            auto& animation = itr->second;
            auto position = interpolateTranslation(animation, tick);
            auto scale = interpolateScale(animation, tick);
            auto qRotate = interpolateRotation(animation, tick);
            auto rotate = glm::mat4(qRotate);

            transform =  glm::translate(glm::mat4(1), position) * rotate * glm::scale(glm::mat4(1), scale);

        }

        auto globalInverseXform = model->globalInverseTransform;
        transforms[i] = globalInverseXform * applyParentTransforms(bone) * transform * bone.offsetMatrix;
    }
    model->buffers.boneTransforms.unmap();
}

void anim::Animation::update0(float time) {
    elapsedTimeInSeconds += time;
    assert(model);

    auto tick = glm::mod(elapsedTimeInSeconds * ticksPerSecond, duration);
    spdlog::info("tick: {}", tick);
    auto& bones = model->bones;
    auto transforms = reinterpret_cast<glm::mat4*>(model->buffers.boneTransforms.map());
    auto globalInverseXform = model->globalInverseTransform;
    std::vector<glm::mat4> hTransforms;

    std::function<void(mdl::Bone&, glm::mat4)> walkBoneHierarchy = [&](mdl::Bone& bone, glm::mat4 parentTransform){
        auto transform = bone.transform;
        auto itr = channels.find(bone.name);
        if(itr != channels.end()) {
            auto& animation = itr->second;
            auto position = interpolateTranslation(animation, tick);
            auto scale = interpolateScale(animation, tick);
            auto qRotate = interpolateRotation(animation, tick);
            auto rotate = glm::mat4(qRotate);

            transform =  glm::translate(glm::mat4(1), position) * rotate * glm::scale(glm::mat4(1), scale);
        }
        auto globalTransform = parentTransform * transform;
        auto finalTransform = globalInverseXform * globalTransform * bone.offsetMatrix;
        hTransforms.push_back(finalTransform);

        for(int childId : bone.children){
            walkBoneHierarchy(model->bones[childId], globalTransform);
        }
    };

    walkBoneHierarchy(model->bones[model->rootBone], glm::mat4(1));
    auto dest = model->buffers.boneTransforms.map();
    std::memcpy(dest, hTransforms.data(), BYTE_SIZE(hTransforms));
    model->buffers.boneTransforms.unmap();

}

glm::vec3 anim::Animation::interpolateTranslation(const anim::BoneAnimation &boneAnimation, anim::Tick tick) {
    if(boneAnimation.translationKeys.size() == 1){
        return boneAnimation.translationKeys[0].value;
    }

    auto index = boneAnimation.translationKey(tick);
    assert(index >= 0);
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
    assert(index >= 0);
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
    assert(index >= 0);
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
