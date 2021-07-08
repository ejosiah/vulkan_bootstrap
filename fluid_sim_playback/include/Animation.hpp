#pragma once

#include "common.h"
#include "VulkanBuffer.h"
#include "ThreadPool.hpp"

struct Frame{
    uint32_t id;
    uint64_t duration;
    std::vector<glm::vec4> positions;
};

struct Animation{
    std::vector<Frame> frames{};
    bool loop = true;
    uint32_t frameIndex = 0;
    uint64_t duration = 0;

    std::vector<glm::vec4> next(uint64_t elapsedTime) {
        auto size = frames.size();
        if(elapsedTime > duration){
            duration = elapsedTime + frames[frameIndex].duration;

            if(loop){
                frameIndex++;
                frameIndex %= size;
            }else if(frameIndex < size - 1){
                frameIndex++;
            }
            return frames[frameIndex].positions;
        }
        return {};
    }
};

struct Animation1{
    bool loop = true;
    uint32_t frameIndex = 0;
    uint32_t duration = 0;
    uint32_t frameDuration = 0;
    uint32_t numFrames = 0;
    uint32_t numPoints = 0;

   void next(uint64_t elapsedTime, VulkanBuffer& commandBuffer) {
       auto size = numFrames;
       if(elapsedTime > duration){
           duration = elapsedTime + frameDuration;

           if(loop){
               frameIndex++;
               frameIndex %= size;
           }else if(frameIndex < size - 1){
               frameIndex++;
           }
           commandBuffer.map<VkDrawIndirectCommand>([&](auto cmd){
               cmd->vertexCount = numPoints;
               cmd->instanceCount = 1;
               cmd->firstVertex = frameIndex * numPoints;
               cmd->firstInstance = 0;
           });
       }
   }

};

Animation buildAnimation(const std::string& dir, uint32_t & numFrames, uint32_t& numPoints, uint32_t& fps){

    std::ifstream fin(dir + "meta.txt");
    if(!fin.good()) throw std::runtime_error{"unable to load animation metadata"};
    fin >> numPoints >> numFrames >> fps;
    fin.close();

    std::string path_prefix = "frame_";
    std::string ext = ".xyz";
    auto duration = uint64_t (1.0f/float(fps) * 1000);
    Animation animation;
    animation.frames.resize(numFrames);
    std::vector<std::future<int>> futures;
    futures.reserve(numFrames);
    for(auto i = 0; i < numFrames; i++){
        auto future = par::ThreadPool::global().async([&, id=i]{
            auto path = dir + path_prefix + (id < 10 ? "00000" : "0000") + std::to_string(id) + ext;
            std::ifstream fin(path);
            if(!fin.good()) throw std::runtime_error{"Failed to open: " + path};

            Frame frame{uint32_t (id), duration};
            for(int j = 0; j < numPoints; j++){
                glm::vec4 pos{1};
                fin >> pos.x >> pos.y >> pos.z;
                frame.positions.push_back(pos);
            }
            animation.frames[id] = frame;
            return 0;
        });
        futures.push_back(std::move(future));
    }


    par::sequence(futures).get();

    return animation;
}

inline std::tuple<Animation1, VulkanBuffer> buildAnimation1(const std::string& dir, int numFrames, int numPoints, int fps);