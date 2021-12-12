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

anim::Animation anim::transition(anim::Animation &from, anim::Animation &to, float durationInSeconds) {
    Animation animation;
    animation.name = fmt::format("{}_{}", from.name, to.name);
    animation.loop = false;
    animation.ticksPerSecond = from.ticksPerSecond;
    animation.nodes = from.nodes;
    animation.duration = durationInSeconds * from.ticksPerSecond + 1;
    animation.model = from.model;

    for(const auto& [name, channel] : from.channels){
        BoneAnimation boneAnim;
        boneAnim.name = channel.name;

        auto tick = glm::clamp(from.elapsedTime * from.ticksPerSecond, 0.f, from.duration - 1);
        auto index = channel.translationKey(tick);
        auto tKey = channel.translationKeys[index];
        tKey.tick = 0;
        boneAnim.translationKeys.push_back(tKey);


        index = channel.scaleKey(tick);
        auto sKey = channel.scaleKeys[index];
        sKey.tick = 0;
        boneAnim.scaleKeys.push_back(sKey);


        index = channel.rotationKey(tick);
        auto rKey = channel.rotationKeys[index];
        rKey.tick = 0;
        boneAnim.rotationKeys.push_back(rKey);



        tKey = to.channels[name].translationKeys[0];
        sKey = to.channels[name].scaleKeys[0];
        rKey = to.channels[name].rotationKeys[0];
        for(int i = 1; i < int(animation.duration - 1); i++){
            tKey.tick = sKey.tick = rKey.tick = float(i);
            boneAnim.translationKeys.push_back(tKey);
            boneAnim.scaleKeys.push_back(sKey);
            boneAnim.rotationKeys.push_back(rKey);
        }

        animation.channels[name] = boneAnim;


    }
    return animation;
}

void anim::Animation::update(float time) {
    assert(model);
    elapsedTime += time;

    static std::vector<glm::mat4> hTransforms;
    hTransforms.resize(model->bones.size());

    auto tick = elapsedTime * ticksPerSecond;
    tick = loop ? glm::mod(tick, duration) : glm::clamp(tick, 0.f, duration - 1);
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

bool anim::Animation::finished() const {
    return !loop && elapsedTime * ticksPerSecond > duration;
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

void anim::Character::update(float time) {
    animations[currentState].update(time);
}
