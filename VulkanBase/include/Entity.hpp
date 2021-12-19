#pragma once
#include <entt/entt.hpp>

class Entity{
public:
    Entity() = default;

    Entity(entt::registry& registry)
    :m_handle(registry.create())
    ,m_registry(&registry)
    {}

    template<typename T, typename... Args>
    decltype(auto) add(Args&&... args){
        return m_registry->emplace<T>(m_handle, std::forward<Args>(args)...);
    }


    template<typename T>
    T& get() const{
        return m_registry->get<T>(m_handle);
    }

    template<typename T>
    [[nodiscard]] bool has() const {
        return m_registry->try_get<T>(m_handle) != nullptr;
    }

    template<typename T>
    void remove(){
        assert(has<T>());
        m_registry->remove<T>(m_handle);
    }

    operator bool() const {
        return m_handle != entt::null;
    }

    operator entt::entity() const {
        return m_handle;
    }

    bool operator==(const Entity& other) const {
        return m_registry == other.m_registry && m_handle == other.m_handle;
    }

private:
    entt::entity m_handle{entt::null};
    entt::registry* m_registry{nullptr};
};