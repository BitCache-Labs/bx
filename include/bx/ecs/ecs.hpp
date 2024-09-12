#pragma once

// TODO: Review this ECS implementation, it's basic but it worked in previous versions.
// However, it's old and needs a lot of improvements, especially on the memory management side.

#include "bx/engine/core/uuid.hpp"
#include "bx/engine/core/macros.hpp"
#include "bx/engine/core/event.hpp"
#include "bx/engine/core/guard.hpp"
#include "bx/engine/core/time.hpp"
#include "bx/engine/core/meta.hpp"
#include "bx/engine/core/type.hpp"
#include "bx/engine/core/memory.hpp"
#include "bx/engine/core/profiler.hpp"
#include "bx/engine/containers/list.hpp"
#include "bx/engine/containers/hash_map.hpp"
#include "bx/engine/containers/pool.hpp"

#include <bitset>
#include <functional>
#include <memory>
#include <algorithm>

#ifndef ECS_POOL_SIZE
#define ECS_POOL_SIZE 1000
#endif

#ifndef ECS_MAX_COMPONENTS
#define ECS_MAX_COMPONENTS 64
#endif

using EntityId = UUID;
constexpr EntityId INVALID_ENTITY_ID = 0;

class Entity;
class ComponentBase;

class IComponentRef;

template <typename TCmp>
class ComponentRef;

using ComponentMask = std::bitset<ECS_MAX_COMPONENTS>;

/// <summary>
/// Event struct for when an entity is created.
/// </summary>
struct EntityCreated
{
    EntityCreated(const Entity& e)
        : entity(e)
    {}

    const Entity& entity;
};

/// <summary>
/// Event struct for when an entity is destroyed.
/// </summary>
struct EntityDestroyed
{
    EntityDestroyed(const Entity& e)
        : entity(e)
    {}

    const Entity& entity;
};

/// <summary>
/// Event struct for when any component is added to an entity.
/// </summary>
struct AnyComponentAdded
{
    AnyComponentAdded(const Entity& e, const ComponentMask& cmpMask)
        : entity(e)
        , cmpMask(cmpMask)
    {}

    const Entity& entity;
    const ComponentMask& cmpMask;
};

/// <summary>
/// Event struct for when any component is removed from an entity.
/// </summary>
struct AnyComponentRemoved
{
    AnyComponentRemoved(const Entity& e, const ComponentMask& cmpMask)
        : entity(e)
        , cmpMask(cmpMask)
    {}

    const Entity& entity;
    const ComponentMask& cmpMask;
};

/// <summary>
/// Event struct for when an component is added to an entity.
/// </summary>
/// <typeparam name="TCmp">Component type.</typeparam>
template <typename TCmp>
struct ComponentAdded
{
    ComponentAdded(const Entity& e, const TCmp& c)
        : entity(e)
        , cmp(c)
    {}

    const Entity& entity;
    const TCmp& cmp;
};

/// <summary>
/// Event struct for when an component is removed from an entity.
/// </summary>
/// <typeparam name="TCmp">Component type.</typeparam>
template <typename TCmp>
struct ComponentRemoved
{
    ComponentRemoved(const Entity& e, const TCmp& c)
        : entity(e)
        , cmp(c)
    {}

    const Entity& entity;
    const TCmp& cmp;
};

/// <summary>
/// The entity struct is a fancy wrapper for an ID.
/// </summary>
class Entity
{
public:
    Entity() : m_id(INVALID_ENTITY_ID) {}
    Entity(EntityId id)
        : m_id(id)
    {}

    inline bool operator <(const Entity& rhs) const { return GetId() < rhs.GetId(); }
    inline bool operator >(const Entity& rhs) const { return GetId() > rhs.GetId(); }
    inline bool operator ==(const Entity& rhs) const { return GetId() == rhs.GetId(); }
    inline bool operator !=(const Entity& rhs) const { return GetId() != rhs.GetId(); }

    /// <summary>
    /// Returns an invalid entity with no proper ID.
    /// </summary>
    /// <returns>Invalid Entity.</returns>
    static Entity Invalid() { return Entity(); }

    /// <summary>
    /// Returns the ID of this entity.
    /// </summary>
    /// <returns>Entity ID.</returns>
    inline EntityId GetId() const { return m_id; }

    /// <summary>
    /// Check whether this entity is valid.
    /// </summary>
    /// <returns>True if valid.</returns>
    inline bool IsValid() const;

    /// <summary>
    /// Gets the component mask associated with this entity.
    /// </summary>
    /// <returns>Entity component mask.</returns>
    inline ComponentMask GetComponentMask();

    /// <summary>
    /// Checks whether this entity's mask contains desired mask.
    /// </summary>
    /// <returns>True if entity contains mask.</returns>
    template<typename ... TCmps>
    inline bool HasComponents();

    /// <summary>
    /// Checks whether an entity has a given component.
    /// </summary>
    /// <typeparam name="TCmp">Component type.</typeparam>
    /// <returns>True if entity has component.</returns>
    template <typename TCmp>
    inline bool HasComponent() const;

    /// <summary>
    /// Adds a component to a given entity.
    /// </summary>
    /// <typeparam name="TCmp">Component type.</typeparam>
    /// <returns>A reference of the added component.</returns>
    template <typename TCmp>
    inline TCmp& AddComponent() const;

    /// <summary>
    /// Return the component reference from the entity.
    /// </summary>
    /// <typeparam name="TCmp">Type of component.</typeparam>
    /// <returns>Component reference.</returns>
    template <typename TCmp>
    inline TCmp& GetComponent() const;

    /// <summary>
    /// Removes a component from an entity.
    /// </summary>
    /// <typeparam name="TCmp">Component type.</typeparam>
    template <typename TCmp>
    inline void RemoveComponent() const;

    /// <summary>
    /// Destroys and invalidates an entity.
    /// </summary>
    inline void Destroy();

    /// <summary>
    /// Returns list of components as their base pointer
    /// </summary>
    /// <returns></returns>
    inline std::vector<ComponentBase*> GetComponents() const;

private:
    friend class EntityManager;
    friend class IComponentRef;

    template <typename T>
    friend class Serial;

    template <typename TCmp>
    inline TCmp* GetComponentPtr() const;

    EntityId m_id = INVALID_ENTITY_ID;
};

class ComponentBase
{
public:
    virtual ~ComponentBase() = 0;

    virtual void Copy(const ComponentBase& cmp) = 0;
    virtual void OnPostCopy() = 0;
    virtual void OnRemoved() = 0;
    virtual TypeId GetTypeId() = 0;

    inline bool GetEnabled() const { return m_enabled; }
    inline void SetEnabled(bool enabled) { m_enabled = enabled; }

    inline const Entity& GetEntity() const { return m_entity; }

private:
    friend class EntityManager;

    template <typename T>
    friend class Component;

    template <typename T>
    friend class Inspector;

    bool m_enabled = true;

    Entity m_entity = Entity::Invalid();
};

template <typename TCmp>
class Component : public ComponentBase
{
public:
    inline void Copy(const ComponentBase& cmp)
    {
        auto entity = m_entity;
        *reinterpret_cast<TCmp*>(this) = (const TCmp&)cmp;
        m_entity = entity;

        OnPostCopy();
    }

    virtual void OnPostCopy() override {}
    virtual void OnRemoved() override {}

    virtual TypeId GetTypeId() override { return Type<TCmp>::Id(); }
};

//class IComponentRef
//{
//public:
//    virtual ~IComponentRef() {}
//
//    virtual bool operator==(const IComponentRef& cmpRef) const = 0;
//
//    virtual TypeId GetTypeId() const = 0;
//    virtual ComponentBase* GetCmpBasePtr(const Entity& entity) const = 0;
//
//protected:
//    friend class Entity;
//    friend class EntityManager;
//    
//    template <typename TCmp>
//    inline TCmp* GetCmpPtr(const Entity& entity) const { return entity.GetComponentPtr<TCmp>(); }
//
//    Entity m_entity;
//};
//
//template <typename TCmp>
//class ComponentRef : public IComponentRef
//{
//public:
//    ComponentRef(const Entity& entity)
//        : m_entity(entity)
//    {}
//    ~ComponentRef() {}
//
//    inline TCmp* Get() const
//    {
//        if (!m_entity.IsValid() || !m_entity.HasComponent<TCmp>())
//            return nullptr;
//
//        return GetCmpPtr<TCmp>(m_entity);
//    }
//
//    inline TCmp* operator->() const { return Get(); }
//
//    inline void Remove() const { m_entity.RemoveComponent<TCmp>(); }
//
//    bool operator==(const IComponentRef& other) const { return GetTypeId() == other.GetTypeId() && m_entity == other.m_entity; }
//
//    TypeId GetTypeId() const { return Type<TCmp>::Id(); }
//    ComponentBase* GetCmpBasePtr(const Entity& entity) const override { return Get(); }
//};

class IComponentId
{
public:
    static EntityId NextId()
    {
        static EntityId s_nextCmpId = 0;
        return s_nextCmpId++;
    }
};

template <typename TCmp>
class ComponentId : public IComponentId
{
public:
    static const ComponentMask& Mask()
    {
        static const EntityId s_id = NextId();
        BX_ENSURE(s_id < ECS_MAX_COMPONENTS);

        static const ComponentMask s_mask((EntityId)1 << s_id);
        return s_mask;
    }
};

struct ComponentHandle
{
    bool operator ==(const ComponentHandle& rhs) const { return mask == rhs.mask && idx == rhs.idx; }
    bool operator !=(const ComponentHandle& rhs) const { return mask != rhs.mask && idx != rhs.idx; }

    ComponentMask mask;
    SizeType idx = 0;
};

template <typename TCmp>
static void SetComponentMask(ComponentMask& cmpMask)
{
    cmpMask |= ComponentId<TCmp>::Mask();
}

template <typename TCmpA, typename TCmpB, typename ... TCmps>
static void SetComponentMask(ComponentMask& cmpMask)
{
    cmpMask |= ComponentId<TCmpA>::Mask();
    SetComponentMask<TCmpB, TCmps...>(cmpMask);
}

static bool HasMask(const ComponentMask& a, const ComponentMask& b)
{
    return (a & b) == b;
}

/// <summary>
/// Entity manager handles all entity registries and memory allocations of components.
/// </summary>
class EntityManager : NoCopy
{
public:
    static void Initialize()
    {
        // Reserve the default number of entities for each registries
        GetCmpMaskMap().reserve(ECS_POOL_SIZE);
        GetCmpHandles().reserve(ECS_POOL_SIZE);

        // Reserve the max number of components for each pool entry
        GetCmpPoolMap().reserve(ECS_MAX_COMPONENTS);
    }

    static void Shutdown()
    {
        for (SizeType i = 0; i < GetEntities().GetSize(); i++)
        {
            if (!GetEntities().IsUsed(i))
                continue;

            Destroy(GetEntities().Get(i));
        }

        //for (auto& entry : GetCmpHandles())
        //{
        //    entry.second.Clear();
        //}
        //
        //for (auto& entry : GetCmpPoolMap())
        //{
        //    delete entry.second;
        //}
        //
        //GetEntities().Clear();
        //GetCmpMaskMap().clear();
        //GetCmpHandles().clear();
        //GetCmpPoolMap().clear();
    }

    /// <summary>
    /// Creates a new entity with valid ID.
    /// </summary>
    /// <returns>New valid entity.</returns>
    static Entity CreateEntity()
    {
        return CreateEntityWithId(GenUUID::MakeUUID());
    }

    /// <summary>
    /// Creates an entity with a specified ID.
    /// The ID must not be already in use!
    /// </summary>
    /// <param name="id">Specific ID to use.</param>
    /// <returns>New valid entity.</returns>
    static Entity CreateEntityWithId(EntityId id)
    {
        BX_ENSURE(!IsValid(Entity(id)));

        // Setup entity
        Entity& entity = GetEntities().New();

        BX_ENSURE(entity.m_id == INVALID_ENTITY_ID);
        entity.m_id = id;

        // Update registries
        if (GetCmpMaskMap().find(entity.GetId()) == GetCmpMaskMap().end())
            GetCmpMaskMap().insert(std::make_pair(entity.GetId(), ComponentMask()));

        if (GetCmpHandles().find(entity.GetId()) == GetCmpHandles().end())
            GetCmpHandles().insert(std::make_pair(entity.GetId(), Pool<ComponentHandle>(ComponentHandle(), ECS_MAX_COMPONENTS)));

        // Broadcast event
        Event::Broadcast<EntityCreated>(entity);
        return entity;
    }

    using ForAllCallback = std::function<void(const Entity& e)>;

    template <typename ... TCmps>
    using ForEachCallback = std::function<void(const Entity& e, TCmps& ...cmps)>;

    /// <summary>
    /// Iterates through all valid entities and invokes a callback for each one.
    /// </summary>
    /// <param name="callback">Callback to invoke.</param>
    static void ForAll(const ForAllCallback& callback)
    {
        for (SizeType i = 0; i < GetEntities().GetSize(); i++)
        {
            if (!GetEntities().IsUsed(i))
                continue;

            auto& entity = GetEntities().Get(i);
            callback(entity);
        }
    }

    /// <summary>
    /// Iterates through all entities with given component and invokes a callback for each one.
    /// </summary>
    /// <typeparam name="...TCmps">Components to check for.</typeparam>
    /// <param name="callback">Callback to invoke.</param>
    template <typename ... TCmps>
    static void ForEach(typename meta::identity<ForEachCallback<TCmps...>>::type callback)
    {
        const ComponentMask& cmpMask = GetComponentMask<TCmps...>();

        auto& entities = GetEntities();
        for (SizeType i = 0; i < entities.GetSize(); i++)
        {
            if (!entities.IsUsed(i))
                continue;

            auto& entity = entities.Get(i);
            if (HasMask(GetComponentMask(entity), cmpMask))
                Unpack<TCmps...>(entity, callback);
        }
    }

private:
    friend class Entity;

    /// <summary>
    /// Check whether a given entity is valid.
    /// </summary>
    /// <param name="entity">Entity to check.</param>
    /// <returns>True if entity is valid.</returns>
    static bool IsValid(const Entity& entity)
    {
        return GetEntities().IsUsed(entity);
    }

    /// <summary>
    /// Checks whether an entity has a given component.
    /// </summary>
    /// <typeparam name="TCmp">Component type.</typeparam>
    /// <param name="entity">Entity to check for component.</param>
    /// <returns>True if entity has component.</returns>
    template <typename TCmp>
    static bool HasComponent(const Entity& entity)
    {
        BX_ENSURE(IsValid(entity));

        // Get component masks
        const ComponentMask& cmpMask = ComponentId<TCmp>::Mask();
        const ComponentMask& entityCmpMask = GetComponentMask(entity);

        // True if its subset
        return HasMask(entityCmpMask, cmpMask);
    }

    /// <summary>
    /// Adds a component to a given entity.
    /// </summary>
    /// <typeparam name="TCmp">Component type.</typeparam>
    /// <param name="entity">Entity to add component to.</param>
    /// <returns>A reference of the added component.</returns>
    template <typename TCmp>
    static void AddComponent(const Entity& entity)
    {
        BX_ENSURE(IsValid(entity));
        BX_ENSURE(!HasComponent<TCmp>(entity));

        // Update entity component mask
        const ComponentMask& cmpMask = ComponentId<TCmp>::Mask();
        ComponentMask& entityCmpMask = GetComponentMask(entity);
        entityCmpMask |= cmpMask;

        // Get available component index
        Pool<TCmp>& cmpPool = GetPool<TCmp>();
        SizeType cmpIdx = cmpPool.GetFreeIndex();
        TCmp& cmp = cmpPool.New(cmpIdx);
        new (&cmp) TCmp();
        cmp.m_entity = entity;

        // Update component handle
        bool found = false;
        auto& cmpHandles = GetComponentHandles(entity);
        for (SizeType i = 0; i < cmpHandles.GetSize(); i++)
        {
            if (!cmpHandles.IsUsed(i))
                continue;

            auto& cmpHandle = cmpHandles.Get(i);
            if (HasMask(cmpHandle.mask, cmpMask))
            {
                cmpHandle.idx = cmpIdx;
                found = true;
                break;
            }
        }

        // If component handle is not present we need to add it.
        if (!found)
        {
            auto& cmpHandle = cmpHandles.New();
            cmpHandle.mask = cmpMask;
            cmpHandle.idx = cmpIdx;
        }

        // Broadcast events
        Event::Broadcast<ComponentAdded<TCmp>>(entity, GetComponent<TCmp>(entity));
        Event::Broadcast<AnyComponentAdded>(entity, cmpMask);
    }

    /// <summary>
    /// Return the component reference from the entity.
    /// </summary>
    /// <typeparam name="TCmp">Type of component.</typeparam>
    /// <param name="entity">Entity to get component from.</param>
    /// <returns>Component reference.</returns>
    template <typename TCmp>
    static TCmp& GetComponent(const Entity& entity)
    {
        BX_ENSURE(IsValid(entity));
        BX_ENSURE(HasComponent<TCmp>(entity));

        // Get component masks
        const ComponentMask& cmpMask = ComponentId<TCmp>::Mask();

        // Get component pool
        Pool<TCmp>& cmpPool = GetPool<TCmp>();

        // Get component
        auto& cmpHandles = GetComponentHandles(entity);
        for (SizeType i = 0; i < cmpHandles.GetSize(); i++)
        {
            if (!cmpHandles.IsUsed(i))
                continue;

            auto& cmpHandle = cmpHandles.Get(i);
            if (HasMask(cmpHandle.mask, cmpMask))
            {
                return cmpPool.Get(cmpHandle.idx);
            }
        }

        BX_ASSERT(false, "Critical error!");
        throw;
    }

    /// <summary>
    /// 
    /// </summary>
    /// <param name="entity"></param>
    /// <returns></returns>
    static std::vector<ComponentBase*> GetComponents(const Entity& entity)
    {
        BX_ENSURE(IsValid(entity));

        std::vector<ComponentBase*> cmps;
        auto& cmpHandles = GetComponentHandles(entity);
        for (SizeType i = 0; i < cmpHandles.GetSize(); i++)
        {
            if (!cmpHandles.IsUsed(i))
                continue;

            auto& cmpHandle = cmpHandles.Get(i);
            auto cmp = (ComponentBase*)GetCmpPoolMap()[cmpHandle.mask]->GetPtr(cmpHandle.idx);

            cmps.emplace_back(cmp);
        }

        return cmps;
    }

    /// <summary>
    /// Removes a component from an entity.
    /// </summary>
    /// <typeparam name="TCmp">Component type.</typeparam>
    /// <param name="entity">Entity to remove component from.</param>
    template <typename TCmp>
    static void RemoveComponent(const Entity& entity)
    {
        BX_ENSURE(IsValid(entity));
        BX_ENSURE(HasComponent<TCmp>(entity));

        const ComponentMask& cmpMask = ComponentId<TCmp>::Mask();

        // Broadcast component removed event
        Event::Broadcast<ComponentRemoved<TCmp>>(entity, GetComponent<TCmp>(entity));
        Event::Broadcast<AnyComponentRemoved>(entity, cmpMask);

        // Update entity component mask
        ComponentMask& entityCmpMask = GetComponentMask(entity);
        entityCmpMask ^= cmpMask;

        // Get component pool
        Pool<TCmp>& cmpPool = GetPool<TCmp>();

        // Update registries
        auto& cmpHandles = GetComponentHandles(entity);
        for (SizeType i = 0; i < cmpHandles.GetSize(); i++)
        {
            if (!cmpHandles.IsUsed(i))
                continue;

            auto& cmpHandle = cmpHandles.Get(i);
            if (HasMask(cmpHandle.mask, cmpMask))
            {
                cmpPool.Remove(cmpHandle.idx);
                cmpHandles.Remove(cmpHandle);
                return;
            }
        }

        BX_ASSERT(false, "Critical error!");
    }

    /// <summary>
    /// Destroys and invalidates an entity.
    /// </summary>
    /// <param name="entity">Entity to destroy.</param>
    static void Destroy(Entity& entity)
    {
        BX_ENSURE(IsValid(entity));

        ComponentMask& entityMask = GetComponentMask(entity);

        // Broadcast entity destroyed event
        Event::Broadcast<EntityDestroyed>(entity);
        Event::Broadcast<AnyComponentRemoved>(entity, entityMask);

        // Remove components
        auto& cmpHandles = GetComponentHandles(entity);
        for (SizeType i = 0; i < cmpHandles.GetSize(); i++)
        {
            if (!cmpHandles.IsUsed(i))
                continue;

            auto& cmpHandle = cmpHandles.Get(i);

            auto pCmp = static_cast<ComponentBase*>(GetCmpPoolMap()[cmpHandle.mask]->GetPtr(cmpHandle.idx));
            pCmp->OnRemoved();

            GetCmpPoolMap()[cmpHandle.mask]->Remove(cmpHandle.idx);
            cmpHandle.idx = 0;
            cmpHandle.mask.reset();
        }

        cmpHandles.Clear();
        entityMask.reset();

        // Update registries
        GetCmpHandles().erase(entity.GetId());
        GetCmpMaskMap().erase(entity.GetId());
        GetEntities().Remove(entity);

        entity.m_id = INVALID_ENTITY_ID;
    }

    /// <summary>
    /// Calculates the component mask for a give set of component types.
    /// </summary>
    /// <typeparam name="...TCmps">Set of component types.</typeparam>
    /// <returns>Component mask from component set.</returns>
    template <typename ... TCmps>
    static ComponentMask GetComponentMask()
    {
        ComponentMask cmpMask;
        SetComponentMask<TCmps...>(cmpMask);
        return cmpMask;
    }

    /// <summary>
    /// Return the non-const reference to the entity component mask.
    /// </summary>
    /// <param name="entity">The entity to get mask.</param>
    /// <returns>Entity mask reference.</returns>
    static ComponentMask& GetComponentMask(const Entity& entity)
    {
        BX_ENSURE(IsValid(entity));
        auto it = GetCmpMaskMap().find(entity.GetId());
        BX_ENSURE(it != GetCmpMaskMap().end());

        return it->second;
    }

    static Pool<ComponentHandle>& GetComponentHandles(const Entity& entity)
    {
        BX_ENSURE(IsValid(entity));
        auto it = GetCmpHandles().find(entity.GetId());
        BX_ENSURE(it != GetCmpHandles().end());

        return it->second;
    }

    /// <summary>
    /// End of recursion unpack impl.
    /// </summary>
    template<typename TCmp>
    static TCmp& UnpackImpl(const Entity& entity)
    {
        return GetComponent<TCmp>(entity);
    }

    /// <summary>
    /// Recursion unpack impl.
    /// </summary>
    template<typename TCmpA, typename TCmpB, typename ... TCmps>
    static TCmpA& UnpackImpl(const Entity& entity)
    {
        return GetComponent<TCmpA>(entity);
    }

    /// <summary>
    /// Unpacks (gets reference of) each component in the entity, and forwards it to a callback.
    /// </summary>
    /// <typeparam name="...TCmps">The set of components to get.</typeparam>
    /// <param name="entity">The entity to get components from.</param>
    /// <param name="callback">The callback to forward components.</param>
    template <typename ... TCmps>
    static void Unpack(const Entity& entity, typename meta::identity<ForEachCallback<TCmps...>>::type callback)
    {
        return callback(entity, UnpackImpl<TCmps>(entity)...);
    }

    /// <summary>
    /// Returns the component pool for a given type.
    /// If there is no pool available for that type then a new one is created.
    /// </summary>
    /// <typeparam name="TCmp">The component type.</typeparam>
    /// <returns>A pool reference casted to TCmp.</returns>
    template <typename TCmp>
    static Pool<TCmp>& GetPool()
    {
        const ComponentMask& cmpMask = ComponentId<TCmp>::Mask();
        Pool<TCmp>* pPool;

        if (GetCmpPoolMap().find(cmpMask) != GetCmpPoolMap().end())
        {
            pPool = static_cast<Pool<TCmp>*>(GetCmpPoolMap()[cmpMask]);
            return *pPool;
        }

        pPool = new Pool<TCmp>(TCmp(), ECS_POOL_SIZE);
        GetCmpPoolMap()[cmpMask] = pPool;
        return *pPool;
    }

private:
    static Pool<Entity>& GetEntities()
    {
        static Pool<Entity> s_entities(Entity::Invalid(), ECS_POOL_SIZE);
        return s_entities;
    }

    static HashMap<EntityId, ComponentMask>& GetCmpMaskMap()
    {
        static HashMap<EntityId, ComponentMask> s_cmpMaskMap;
        return s_cmpMaskMap;
    }

    static HashMap<EntityId, Pool<ComponentHandle>>& GetCmpHandles()
    {
        static HashMap<EntityId, Pool<ComponentHandle>> s_cmpHandles;
        return s_cmpHandles;
    }

    static HashMap<ComponentMask, IPool*>& GetCmpPoolMap()
    {
        static HashMap<ComponentMask, IPool*> s_cmpPoolMap;
        return s_cmpPoolMap;
    }
};

/// <summary>
/// Implementation of Entity::IsValid
/// </summary>
inline bool Entity::IsValid() const
{
    return EntityManager::IsValid(*this);
}

/// <summary>
/// Implementation of Entity::HasComponent
/// </summary>
template <typename TCmp>
inline bool Entity::HasComponent() const
{
    return EntityManager::HasComponent<TCmp>(*this);
}

/// <summary>
/// Implementation of Entity::AddComponent
/// </summary>
template <typename TCmp>
inline TCmp& Entity::AddComponent() const
{
    EntityManager::AddComponent<TCmp>(*this);
    return EntityManager::GetComponent<TCmp>(*this);
}

/// <summary>
/// Implementation of Entity::GetComponent
/// </summary>
template <typename TCmp>
inline TCmp& Entity::GetComponent() const
{
    return EntityManager::GetComponent<TCmp>(*this);
}

/// <summary>
/// Implementation of Entity::RemoveComponent
/// </summary>
template <typename TCmp>
inline void Entity::RemoveComponent() const
{
    EntityManager::RemoveComponent<TCmp>(*this);
}

/// <summary>
/// Implementation of Entity::Destroy
/// </summary>
inline void Entity::Destroy()
{
    EntityManager::Destroy(*this);
}

/// <summary>
/// Implementation of Entity::GetComponentMask
/// </summary>
inline ComponentMask Entity::GetComponentMask()
{
    return EntityManager::GetComponentMask(*this);
}

template<typename ... TCmps>
inline bool Entity::HasComponents()
{
    auto mask = EntityManager::GetComponentMask<TCmps...>();
    return HasMask(GetComponentMask(), mask);
}

/// <summary>
/// Implementation of Entity::GetComponentPtr
/// </summary>
template <typename TCmp>
inline TCmp* Entity::GetComponentPtr() const
{
    return &EntityManager::GetComponent<TCmp>(*this);
}

inline std::vector<ComponentBase*> Entity::GetComponents() const
{
    return EntityManager::GetComponents(*this);
}

class System
{
private:
    friend class SystemManager;

    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;

    virtual void Update() = 0;
    virtual void Render() = 0;
};

class SystemManager : NoCopy
{
public:
    static void Initialize()
    {
        for (auto& sys : GetSystems())
        {
            sys->Initialize();
        }
    }

    static void Update()
    {
        PROFILE_FUNCTION();

        for (auto& sys : GetSystems())
        {
            sys->Update();
        }
    }

    static void Render()
    {
        PROFILE_FUNCTION();

        for (auto& sys : GetSystems())
        {
            sys->Render();
        }
    }

    static void Shutdown()
    {
        for (auto& sys : GetSystems())
        {
            sys->Shutdown();
        }

        GetSystems().clear();
        GetSystemsMap().clear();
    }

    template <typename TSys, typename... TArgs>
    static void AddSystem(TArgs&&... args)
    {
        auto sys = new TSys(std::forward<TArgs>(args)...);
        GetSystems().emplace_back(sys);
        GetSystemsMap().insert(std::make_pair(Type<TSys>::Id(), sys));
    }

    template <typename TSys>
    static TSys& GetSystem()
    {
        auto& systemsMap = GetSystemsMap();

        SizeType id = Type<TSys>::Id();
        BX_ENSURE(systemsMap.find(id) != systemsMap.end());

        return *static_cast<TSys*>(systemsMap[id]);
    }

    template <typename TSys>
    static void RemoveSystem()
    {
        // TODO
    }
    
private:
    static List<System*>& GetSystems()
    {
        static List<System*> s_systems;
        return s_systems;
    }

    static HashMap<SizeType, System*>& GetSystemsMap()
    {
        static HashMap<SizeType, System*> s_systemsMap;
        return s_systemsMap;
    }
};