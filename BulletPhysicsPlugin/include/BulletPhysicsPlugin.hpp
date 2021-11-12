#pragma once

#include "common.h"
#include "Plugin.hpp"
#include <btBulletDynamicsCommon.h>
#include <DebugDrawer.hpp>

#define BULLET_PHYSICS_PLUGIN "Bullet Physics Engine"
#define INV_60_HERTZ 0.01666666666666666666666666666667
#define INV_120_HERTZ 0.00833333333333333333333333333333
#define EARTH_GRAVITY glm::vec3(0, -9.807, 0)
#define MOON_GRAVITY glm::vec3(0, -1.62, 0)

struct Transform{
    glm::mat3 basis{1 };
    glm::vec3 origin{0 };
};
using RigidBodyId = int;

struct RigidBody{
    RigidBodyId id{0};
    Transform xform{};
    glm::vec3 localInertia{0 };
    btCollisionShape* shape{nullptr };
    float mass{0 };
};


struct CollisionShapes{

    ~CollisionShapes(){
        for(auto i = 0; i < _shapes.size(); i++){
            auto shape = _shapes[i];
            delete shape;
            _shapes[i] = nullptr;
        }
        _shapes.clear();
    }

    void add(btCollisionShape* shape){
        _shapes.push_back(shape);
    }

    btAlignedObjectArray<btCollisionShape*> _shapes;
};

struct DynamicsWorldDeleter{

    void operator()(btDynamicsWorld* dynamicsWorld);
};

struct BulletPhysicsPluginInfo{
    float timeStep{ INV_60_HERTZ };
    int maxSubSteps{ 1 };
    float fixedTimeStep{ INV_60_HERTZ };
    glm::vec3 gravity{EARTH_GRAVITY};
    int debugMode = btIDebugDraw::DebugDrawModes::DBG_NoDebug;
};


class BulletPhysicsPlugin : public Plugin{
public:
    explicit BulletPhysicsPlugin(const BulletPhysicsPluginInfo& info = {});

    void init() override;

    [[nodiscard]]
    std::string name() const override;

    void draw(VkCommandBuffer commandBuffer) override;

    std::vector<VkCommandBuffer> draw(VkCommandBufferInheritanceInfo inheritanceInfo) override;

    void update(float time) override;

    void newFrame() override;

    void endFrame() override;

    void set(const Camera& camera);

    RigidBody addRigidBody(RigidBody body);

    [[nodiscard]]
    Transform getTransform(RigidBodyId id) const;

    [[nodiscard]]
    DebugDrawer* getDebugDrawer() const;

    [[nodiscard]]
    const btDynamicsWorld& dynamicsWorld() const;

private:
    std::unique_ptr<btCollisionConfiguration> _collisionConfiguration;
    std::unique_ptr<btCollisionDispatcher> _dispatcher;
    std::unique_ptr<btBroadphaseInterface> _overlappingPairCache;
    std::unique_ptr<btConstraintSolver> _solver;
    std::unique_ptr<btDynamicsWorld, DynamicsWorldDeleter> _dynamicsWorld;
    std::unique_ptr<CollisionShapes> _collisionShapes;
    float _timeStep;
    float _fixedTimeStep;
    int _maxSubSteps;
    int _debugMode;
    glm::vec3 _gravity;
};