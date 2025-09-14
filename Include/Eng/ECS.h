#ifndef __ECS
#define __ECS

#include "Helpers.h"
#include <type_traits>

namespace Eng {
    class Component;
    class EntitySystem;
    using ECS_id_t = int;
    static ECS_id_t globalId = 0;
    class Entity {
        EntitySystem* system;
        Entity(EntitySystem* _system) : system(_system), id(globalId) {
            globalId++;
        };
        Entity(const Entity& copy) = delete;
        Entity& operator=(const Entity& copy) = delete;
        Entity(Entity&& move) = delete;
        Entity& operator=(Entity&& move) = delete;
        public:
        virtual ~Entity();
        ECS_id_t id;
        template<typename T>
        T& AddComponent(T* component);
        template<typename T>
        void RemoveComponent();
        template<typename T>
        T& GetComponent();
        friend Component;
        friend EntitySystem;
    };

    class Component {
        std::string name;
        virtual void onAdd() {};
    };
    struct TransformComponent : Component {
        vec3 position{0.0f};
        vec3 scale{1.0f};
        vec3 rotation{0.0f};
        mat4 getTransformMat() const;
        mat3 getNormalMat() const;
        TransformComponent() = default;
        TransformComponent(const TransformComponent& copy) = delete;
        TransformComponent& operator=(const TransformComponent& copy) = delete;
        TransformComponent(TransformComponent&& move) = delete;
        TransformComponent& operator=(TransformComponent&& move) = delete;
        virtual ~TransformComponent() = default;
    };

    class StorageI {
        public:
        virtual ~StorageI() = default;
        virtual void RemoveComponent(const ECS_id_t& removedId) = 0;
    };
    template<typename T>
    class Storage : public StorageI {
        std::unordered_map<ECS_id_t, size_t> entityToIndex{};
        std::vector<ECS_id_t> entities{};
        std::vector<OwnedPointer<T>> components{};
        public:
        Storage() = default;
        Storage(const Storage& copy) = delete;
        Storage& operator=(const Storage& copy) = delete;
        Storage(Storage&& move) = delete;
        Storage& operator=(Storage&& move) = delete;
        virtual ~Storage() = default;
        T& InsertComponent(const ECS_id_t& id, T* component) {
            if (entityToIndex.count(id) != 0) throw std::runtime_error("Cannot add more than one of the same component to an entity.");
            size_t index = components.size();
            entityToIndex[id] = index;
            entities.push_back(id);
            components.push_back(component);
            return components[index];
        }
        void RemoveComponent(const ECS_id_t& removedId) {
            if (entityToIndex.count(removedId) == 0) return;
            std::size_t removedIndex = entityToIndex[removedId];
            std::size_t lastIndex = components.size()-1;
            if (removedIndex != lastIndex) {
                ECS_id_t lastEntity = entities[lastIndex];
                entityToIndex.erase(removedId);
                entityToIndex[lastEntity] = removedIndex;
                entities[removedIndex] = lastEntity;
                components[removedIndex] = (OwnedPointer<T>&&)components[lastIndex];
            }
            entities.pop_back();
            components.pop_back();
        }
        T& GetComponent(const ECS_id_t& id) {
            if (entityToIndex.count(id) == 0) throw std::runtime_error("Entity does not have component T.");
            return components[entityToIndex[id]];
        }
        std::vector<ECS_id_t>& GetEntities() {
            return entities;
        }
    };

    class EntitySystem {
        std::unordered_map<ECS_id_t, OwnedPointer<Entity>> entities{};
        std::vector<OwnedPointer<StorageI>> storages{};
        std::unordered_map<std::size_t, std::size_t> registeredComponents{};
        template<typename T>
        Storage<T>* getStorage() {
            size_t typeHash = typeid(T).hash_code();
            if (registeredComponents.count(typeHash) == 0) throw std::runtime_error("Cannot get storage of unregistered type.");
            return (Storage<T>*)storages[registeredComponents[typeHash]].value;
        }
        public:
        EntitySystem() = default;
        EntitySystem(const EntitySystem& copy) = delete;
        EntitySystem& operator=(const EntitySystem& copy) = delete;
        EntitySystem(EntitySystem&& move) = delete;
        EntitySystem& operator=(EntitySystem&& move) = delete;
        virtual ~EntitySystem() {
            entities.clear();
            storages.clear();
        };

        template<typename T>
        void RegisterComponent() {
            static_assert(std::is_base_of<Component, T>::value);
            size_t typeHash = typeid(T).hash_code();
            std::string typeName = typeid(T).name();
            registeredComponents[typeHash] = storages.size();
            storages.push_back(new Storage<T>());
        }
        Entity* CreateEntity() {
            Entity* entity = new Entity(this);
            entities[entity->id] = entity;
            return entity;
        }
        template<typename T>
        T& AddComponent(const ECS_id_t& id, T* component) {
            static_assert(std::is_base_of<Component, T>::value);
            return getStorage<T>()->InsertComponent(id, component);
        }
        template<typename T>
        void RemoveComponent(const ECS_id_t& id) {
            static_assert(std::is_base_of<Component, T>::value);
            getStorage<T>()->RemoveComponent(id);
        }
        void RemoveAll(const ECS_id_t& id) {
            for (size_t i = 0; i < storages.size(); i++) {
                storages[i]->RemoveComponent(id);
            }
        }
        Entity& GetEntity(const ECS_id_t& id) {
            return entities[id];
        }
        template<typename T>
        T& GetComponent(const ECS_id_t& id) {
            static_assert(std::is_base_of<Component, T>::value);
            return getStorage<T>()->GetComponent(id);
        }
        template<typename T>
        std::vector<ECS_id_t>& GetEntitiesWithComponent() {
            static_assert(std::is_base_of<Component, T>::value);
            return getStorage<T>()->GetEntities();
        }
    };
    template<typename T>
    T& Entity::AddComponent(T* component) {
        return system->AddComponent(id, component);
    }
    template<typename T>
    void Entity::RemoveComponent() {
        system->RemoveComponent<T>(id);
    }
    template<typename T>
    T& Entity::GetComponent() {
        return system->GetComponent<T>(id);
    }
}

#endif// _ECS
/*
Adding component N3Eng18TransformComponentE to entity 0
Adding component N3Eng21MeshRendererComponentE to; entity 0
Adding component N3Eng18TransformComponentE to entity 1
Adding component N3Eng21MeshRendererComponentE to entity 1
Adding component N3Eng18TransformComponentE to entity 2
Adding component N3Eng21MeshRendererComponentE to entity 2
Adding component N3Eng18TransformComponentE to entity 3
Adding component N3Eng21MeshRendererComponentE to entity 3
Adding component N3Eng18TransformComponentE to entity 4
Adding component N3Eng19PointLightComponentE to entity 4
Adding component N3Eng18TransformComponentE to entity 5
Adding component N3Eng19PointLightComponentE to entity 5
Adding component N3Eng18TransformComponentE to entity 6
Adding component N3Eng19PointLightComponentE to entity 6
Adding component N3Eng18TransformComponentE to entity 7
Adding component N3Eng19PointLightComponentE to entity 7
Adding component N3Eng18TransformComponentE to entity 8
Adding component N3Eng19PointLightComponentE to entity 8
Adding component N3Eng18TransformComponentE to entity 9
Adding component N3Eng19PointLightComponentE to entity 9

*/