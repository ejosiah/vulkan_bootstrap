#include "animation.hpp"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include "glm_assimp_interpreter.hpp"

int loadNodes(anim::Animation& animation, const aiNode* aiNode, int parentId = -1){
    int id = int(animation.nodes.size());
    std::string name = aiNode->mName.C_Str();
    glm::mat4 transform = to_mat4(aiNode->mTransformation);

    anim::AnimationNode node{id, name, transform, parentId};
    animation.nodes.push_back(node);
    for(int i = 0; i < aiNode->mNumChildren; i++){
        auto childNode = aiNode->mChildren[i];
        auto childId = loadNodes(animation, childNode, id);
        animation.nodes[id].children.push_back(childId);
    }
    return id;
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

    static std::vector<glm::mat4> hTransforms;
    hTransforms.resize(model->bones.size());

    auto tick = glm::mod(elapsedTimeInSeconds * ticksPerSecond, duration);
    auto globalInverseXform = model->globalInverseTransform;
    for(auto& node : nodes){
        auto nodeTransform = node.transform;
        auto itr = channels.find(node.name);
        if(itr != channels.end()){
            auto& animation = itr->second;
            auto position = interpolateTranslation(animation, tick);
            auto scale = interpolateScale(animation, tick);
            auto qRotate = interpolateRotation(animation, tick);
            auto rotate = glm::mat4(qRotate);


            nodeTransform =  glm::translate(glm::mat4(1), position) * rotate * glm::scale(glm::mat4(1), scale);
        }
        glm::mat4 parentTransform = node.parentId != -1 ? nodes[node.parentId].globalTransform : glm::mat4(1);
        node.globalTransform = parentTransform * nodeTransform;

        auto bItr = model->bonesMapping.find(node.name);
        if(bItr != model->bonesMapping.end()) {
            auto& bone = model->bones[bItr->second];
            hTransforms[bone.id] = globalInverseXform * node.globalTransform * bone.offsetMatrix;
        }
    }
    model->buffers.boneTransforms.copy(hTransforms.data(), BYTE_SIZE(hTransforms));

}

void anim::Animation::update0(float time) {
    elapsedTimeInSeconds += time;
    assert(model);

    auto tick = glm::mod(elapsedTimeInSeconds * ticksPerSecond, duration);
    auto& bones = model->bones;
    auto globalInverseXform = model->globalInverseTransform;
    static std::vector<glm::mat4> hTransforms;
    hTransforms.resize(model->bones.size());

    std::function<void(anim::AnimationNode&, glm::mat4)> walkBoneHierarchy = [&](anim::AnimationNode& node, glm::mat4 parentTransform){
        auto transform = node.transform;
        auto itr = channels.find(node.name);
        if(itr != channels.end()) {
            auto& animation = itr->second;
            auto position = interpolateTranslation(animation, tick);
            auto scale = interpolateScale(animation, tick);
            auto qRotate = interpolateRotation(animation, tick);
            auto rotate = glm::mat4(qRotate);

            transform =  glm::translate(glm::mat4(1), position) * rotate * glm::scale(glm::mat4(1), scale);
        }
        node.globalTransform = parentTransform * transform;
        auto bItr = model->bonesMapping.find(node.name);
        if(bItr != model->bonesMapping.end()) {
            auto& bone = model->bones[bItr->second];
            auto finalTransform = globalInverseXform * node.globalTransform * bone.offsetMatrix;
            hTransforms[bone.id] = finalTransform;
        }


        for(int childId : node.children){
            walkBoneHierarchy(nodes[childId], node.globalTransform);
        }
    };
    walkBoneHierarchy(nodes[0], glm::mat4(1));
    model->buffers.boneTransforms.copy(hTransforms.data(), BYTE_SIZE(hTransforms));
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
        return glm::normalize(boneAnimation.rotationKeys[0].value);
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

void anim::Animation::update_dod(float time) {
    assert(model);
    elapsedTimeInSeconds += time;
    auto tick = glm::mod(elapsedTimeInSeconds * ticksPerSecond, duration);

    int size = animNodes.translationKeys.size();
    std::vector<AnimTransforms>& transforms = animNodes.transforms;

    for(int nodeId = 0; nodeId < size; nodeId++){
        if(animNodes.translationKeys[nodeId].size() == 1){
            transforms[nodeId].position = animNodes.translationKeys[nodeId][0].value;
        }else{
            for(int keyId = 0; keyId < animNodes.translationKeys[nodeId].size() - 1; keyId++){
                if(tick < animNodes.translationKeys[nodeId][keyId + 1].tick){
                    auto currentKeyFrame = animNodes.translationKeys[nodeId][keyId];
                    auto nextKeyFrame = animNodes.translationKeys[nodeId][keyId + 1];

                    auto length = nextKeyFrame.tick - currentKeyFrame.tick;
                    auto position = tick - currentKeyFrame.tick;
                    float t = position/length;
                    assert(t >= 0.f && t <= 1.f);
                    transforms[nodeId].position = mix(currentKeyFrame.value, nextKeyFrame.value, t);
                    break;
                }
            }
        }
    }

    for(int nodeId = 0; nodeId < size; nodeId++){
        if(animNodes.scaleKeys[nodeId].size() == 1){
            transforms[nodeId].scale = animNodes.scaleKeys[nodeId][0].value;
        }else{
            for(int keyId = 0; keyId < animNodes.scaleKeys[nodeId].size() - 1; keyId++){
                if(tick < animNodes.scaleKeys[nodeId][keyId + 1].tick){
                    auto currentKeyFrame = animNodes.scaleKeys[nodeId][keyId];
                    auto nextKeyFrame = animNodes.scaleKeys[nodeId][keyId + 1];

                    auto length = nextKeyFrame.tick - currentKeyFrame.tick;
                    auto position = tick - currentKeyFrame.tick;
                    float t = position/length;
                    assert(t >= 0.f && t <= 1.f);
                    transforms[nodeId].scale = mix(currentKeyFrame.value, nextKeyFrame.value, t);
                    break;
                }
            }
        }
    }

    for(int nodeId = 0; nodeId < size; nodeId++){
        if(animNodes.rotationKeys[nodeId].size() == 1){
            auto qRotate = animNodes.rotationKeys[nodeId][0].value;
            transforms[nodeId].rotation = glm::normalize(qRotate);
        }else{
            for(int keyId = 0; keyId < animNodes.rotationKeys[nodeId].size() - 1; keyId++){
                if(tick < animNodes.rotationKeys[nodeId][keyId + 1].tick){
                    auto currentKeyFrame = animNodes.rotationKeys[nodeId][keyId];
                    auto nextKeyFrame = animNodes.rotationKeys[nodeId][keyId + 1];

                    auto length = nextKeyFrame.tick - currentKeyFrame.tick;
                    auto position = tick - currentKeyFrame.tick;
                    float t = position/length;
                    assert(t >= 0.f && t <= 1.f);
                    auto qRotate = slerp(currentKeyFrame.value, nextKeyFrame.value, t);
                    transforms[nodeId].rotation = glm::normalize(qRotate);
                    break;
                }
            }
        }
    }

    for(auto nodeId = 0; nodeId < size; nodeId++){
        if(animNodes.hasBone[nodeId]){
            auto translation = glm::translate(glm::mat4(1), animNodes.transforms[nodeId].position);
            auto rotation = glm::mat4(animNodes.transforms[nodeId].rotation);
            auto scale = glm::scale(glm::mat4(1), animNodes.transforms[nodeId].scale);
            animNodes.nodeTransforms[nodeId] = translation * rotation * scale;
        }
    }

    for(auto nodeId = 0; nodeId < size; nodeId++){
        glm::mat4 parentTransform = animNodes.parentIds[nodeId] != -1 ? animNodes.globalTransforms[animNodes.parentIds[nodeId]] : glm::mat4(1);
        animNodes.globalTransforms[nodeId] = parentTransform * animNodes.nodeTransforms[nodeId];
    }

    auto globalInverseXform = model->globalInverseTransform;
    for(auto nodeId = 0; nodeId < size; nodeId++){
        if(animNodes.hasBone[nodeId]){
            animNodes.finalTransforms[nodeId] = globalInverseXform * animNodes.globalTransforms[nodeId] * animNodes.offsetMatrix[nodeId];
        }
    }

    int boneId = 0;
    auto boneTransforms = reinterpret_cast<glm::mat4*>(model->buffers.boneTransforms.map());
    for(auto nodeId = 0; nodeId < size; nodeId++){
        if(animNodes.hasBone[nodeId]){
            boneTransforms[boneId] = animNodes.finalTransforms[nodeId];
            boneId++;
        }
    }
    model->buffers.boneTransforms.unmap();
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
