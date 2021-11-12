
#include <string>
#include <vulkan/vulkan_core.h>
#include <Plugin.hpp>
#include <BulletPhysicsPlugin.hpp>
#include <glm_bullet_interpreter.hpp>

BulletPhysicsPlugin::BulletPhysicsPlugin(const BulletPhysicsPluginInfo& info)
 : _timeStep{ info.timeStep }
 , _maxSubSteps{ info.maxSubSteps}
 , _fixedTimeStep{ info.fixedTimeStep }
 , _gravity{ info.gravity }
 , _debugMode{ info.debugMode }{

}


void BulletPhysicsPlugin::init() {
    _collisionConfiguration = std::make_unique<btDefaultCollisionConfiguration>();
    _dispatcher = std::make_unique<btCollisionDispatcher>(_collisionConfiguration.get());
    _overlappingPairCache = std::make_unique<btDbvtBroadphase>();
    _solver = std::make_unique<btSequentialImpulseConstraintSolver>();
    _dynamicsWorld = std::unique_ptr<btDynamicsWorld, DynamicsWorldDeleter>(
            new btDiscreteDynamicsWorld{_dispatcher.get(), _overlappingPairCache.get(), _solver.get(), _collisionConfiguration.get()});
    _dynamicsWorld->setGravity(btVector3(_gravity.x, _gravity.y, _gravity.z));

    if(_debugMode != btIDebugDraw::DebugDrawModes::DBG_NoDebug) {
        _dynamicsWorld->setDebugDrawer(new DebugDrawer{data, _debugMode});
    }
}

std::string BulletPhysicsPlugin::name() const {
    return BULLET_PHYSICS_PLUGIN;
}

void BulletPhysicsPlugin::draw(VkCommandBuffer commandBuffer) {
    if(getDebugDrawer()) {
        assert(_camera);
        getDebugDrawer()->setCamera(_camera);
        getDebugDrawer()->draw(commandBuffer);
    }
}

std::vector <VkCommandBuffer> BulletPhysicsPlugin::draw(VkCommandBufferInheritanceInfo inheritanceInfo) {
    return {};
}

void BulletPhysicsPlugin::update(float time) {
}

void BulletPhysicsPlugin::newFrame() {
    _dynamicsWorld->stepSimulation(_timeStep, _maxSubSteps, _fixedTimeStep);
    _dynamicsWorld->debugDrawWorld();
}

void BulletPhysicsPlugin::endFrame() {
    if(_dynamicsWorld->getDebugDrawer()) {
        _dynamicsWorld->getDebugDrawer()->clearLines();
    }
}

RigidBody BulletPhysicsPlugin::addRigidBody(RigidBody body) {
    auto shape = body.shape;
    btVector3 localInertia{0, 0, 0};
    btScalar mass{body.mass};

    if(body.mass > 0){
        shape->calculateLocalInertia(mass, localInertia);
    }
    btTransform xform{};
    xform.setBasis(to_btMatrix3x3(body.xform.basis));
    xform.setOrigin(to_btVector3(body.xform.origin));

    auto motionState = new btDefaultMotionState{ xform };
    btRigidBody::btRigidBodyConstructionInfo info{ mass, motionState, shape, localInertia};
    auto bt_body = new btRigidBody{ info };
    _dynamicsWorld->addRigidBody(bt_body);

    body.id =  RigidBodyId{ _dynamicsWorld->getNumCollisionObjects() - 1};
    return body;
}

Transform BulletPhysicsPlugin::getTransform(RigidBodyId id) const {
    assert(id < _dynamicsWorld->getNumCollisionObjects());
    auto obj = _dynamicsWorld->getCollisionObjectArray()[id];
    auto body = btRigidBody::upcast(obj);

    btTransform xform;
    if(body && body->getMotionState()){
        body->getMotionState()->getWorldTransform(xform);
    }else{
        xform = obj->getWorldTransform();
    }

    return {
        to_mat3(xform.getBasis()),
        to_vec3(xform.getOrigin())
    };
}

DebugDrawer *BulletPhysicsPlugin::getDebugDrawer() const {
    return dynamic_cast<DebugDrawer*>(_dynamicsWorld->getDebugDrawer());
}

const btDynamicsWorld &BulletPhysicsPlugin::dynamicsWorld() const {
    return *_dynamicsWorld;
}

void BulletPhysicsPlugin::set(const Camera &camera) {
    _camera = const_cast<Camera*>(&camera);
}

void DynamicsWorldDeleter::operator()(btDynamicsWorld *dynamicsWorld) {
    for(auto i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--){
        auto obj = dynamicsWorld->getCollisionObjectArray()[i];
        auto body = btRigidBody::upcast(obj);
        if(body && body->getMotionState()){
            delete body->getMotionState();
        }
        dynamicsWorld->removeCollisionObject(obj);
        delete obj;
    }
    if(dynamicsWorld->getDebugDrawer()){
        delete dynamicsWorld->getDebugDrawer();
    }
    delete dynamicsWorld;
}
