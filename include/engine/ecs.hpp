#pragma once

// TODO: Review this ECS implementation, it's basic but it worked in previous versions.
// However, it's old and needs a lot of improvements, especially on the memory management side.

#include <engine/api.hpp>
#include <engine/uuid.hpp>
#include <engine/macros.hpp>
#include <engine/event.hpp>
#include <engine/guard.hpp>
#include <engine/time.hpp>
#include <engine/memory.hpp>
#include <engine/profiler.hpp>
#include <engine/traits.hpp>
#include <engine/type.hpp>
#include <engine/list.hpp>
#include <engine/hash_map.hpp>
#include <engine/pool.hpp>
#include <engine/function.hpp>

#include <bitset>
#include <algorithm>

#ifndef BX_ECS_POOL_SIZE
#define BX_ECS_POOL_SIZE 1000
#endif

#ifndef BX_ECS_MAX_COMPONENTS
#define BX_ECS_MAX_COMPONENTS 64
#endif

using EntityId = UUID;
constexpr EntityId INVALID_ENTITY_ID = 0;

class Entity;
class ComponentBase;

class IComponentRef;

template <typename TCmp>
class ComponentRef;

using ComponentMask = std::bitset<BX_ECS_MAX_COMPONENTS>;

/// <summary>
/// Event struct for when an entity is created.
/// </summary>
struct BX_API EntityCreated
{
    EntityCreated(const Entity& e)
        : entity(e)
    {}

    const Entity& entity;
};

/// <summary>
/// Event struct for when an entity is destroyed.
/// </summary>
struct BX_API EntityDestroyed
{
    EntityDestroyed(const Entity& e)
        : entity(e)
    {}

    const Entity& entity;
};

/// <summary>
/// Event struct for when any component is added to an entity.
/// </summary>
struct BX_API AnyComponentAdded
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
struct BX_API AnyComponentRemoved
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
struct BX_API ComponentAdded
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
struct BX_API ComponentRemoved
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
class BX_API Entity
{
    BX_TYPE(Entity)

public:
    Entity() {}

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
    BX_TYPE_REGISTRATION_FRIEND;

    Entity(EntityId id, EntityManager* em)
        : m_id(id)
        , m_pEntityMgr(em)
    {}

    template <typename TCmp>
    inline TCmp* GetComponentPtr() const;

    EntityId m_id = INVALID_ENTITY_ID;
    EntityManager* m_pEntityMgr{ nullptr };
};

class BX_API ComponentBase
{
    BX_TYPE(ComponentBase)

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
class BX_API Component : public ComponentBase
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

class BX_API IComponentId
{
public:
    static EntityId NextId()
    {
        static EntityId s_nextCmpId = 0;
        return s_nextCmpId++;
    }
};

template <typename TCmp>
class BX_API ComponentId : public IComponentId
{
public:
    static const ComponentMask& Mask()
    {
        static const EntityId s_id = NextId();
        BX_ENSURE(s_id < BX_ECS_MAX_COMPONENTS);

        static const ComponentMask s_mask((EntityId)1 << s_id);
        return s_mask;
    }
};

struct BX_API ComponentHandle
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
class BX_API EntityManager
{
    BX_TYPE(EntityManager)
    BX_NOCOPY(EntityManager)

public:
    EntityManager()
    {
        // Reserve the default number of entities for each registries
        m_cmpMaskMap.reserve(BX_ECS_POOL_SIZE);
        m_cmpHandles.reserve(BX_ECS_POOL_SIZE);

        // Reserve the max number of components for each pool entry
        m_cmpPoolMap.reserve(BX_ECS_MAX_COMPONENTS);
    }

    ~EntityManager()
    {
        for (SizeType i = 0; i < m_entities.GetSize(); i++)
        {
            if (!m_entities.IsUsed(i))
                continue;

            Destroy(m_entities.Get(i));
        }

        //for (auto& entry : m_cmpHandles)
        //{
        //    entry.second.Clear();
        //}
        //
        //for (auto& entry : m_cmpPoolMap)
        //{
        //    delete entry.second;
        //}
        //
        //m_entities.Clear();
        //m_cmpMaskMap.clear();
        //m_cmpHandles.clear();
        //m_cmpPoolMap.clear();
    }

    /// <summary>
    /// Creates a new entity with valid ID.
    /// </summary>
    /// <returns>New valid entity.</returns>
    Entity CreateEntity()
    {
        return CreateEntityWithId(GenUUID::MakeUUID());
    }

    /// <summary>
    /// Creates an entity with a specified ID.
    /// The ID must not be already in use!
    /// </summary>
    /// <param name="id">Specific ID to use.</param>
    /// <returns>New valid entity.</returns>
    Entity CreateEntityWithId(EntityId id)
    {
        BX_ENSURE(!IsValid(Entity(id, this)));

        // Setup entity
        Entity& entity = m_entities.New();

        BX_ENSURE(entity.m_id == INVALID_ENTITY_ID);
        entity.m_id = id;
        entity.m_pEntityMgr = this;

        // Update registries
        if (m_cmpMaskMap.find(entity.GetId()) == m_cmpMaskMap.end())
            m_cmpMaskMap.insert(std::make_pair(entity.GetId(), ComponentMask()));

        if (m_cmpHandles.find(entity.GetId()) == m_cmpHandles.end())
            m_cmpHandles.insert(std::make_pair(entity.GetId(), Pool<ComponentHandle>(ComponentHandle(), BX_ECS_MAX_COMPONENTS)));

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
    void ForAll(const ForAllCallback& callback)
    {
        for (SizeType i = 0; i < m_entities.GetSize(); i++)
        {
            if (!m_entities.IsUsed(i))
                continue;

            auto& entity = m_entities.Get(i);
            callback(entity);
        }
    }

    /// <summary>
    /// Iterates through all entities with given component and invokes a callback for each one.
    /// </summary>
    /// <typeparam name="...TCmps">Components to check for.</typeparam>
    /// <param name="callback">Callback to invoke.</param>
    template <typename ... TCmps>
    void ForEach(typename meta::identity<ForEachCallback<TCmps...>>::type callback)
    {
        const ComponentMask& cmpMask = GetComponentMask<TCmps...>();

        for (SizeType i = 0; i < m_entities.GetSize(); i++)
        {
            if (!m_entities.IsUsed(i))
                continue;

            auto& entity = m_entities.Get(i);
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
    bool IsValid(const Entity& entity)
    {
        return m_entities.IsUsed(entity);
    }

    /// <summary>
    /// Checks whether an entity has a given component.
    /// </summary>
    /// <typeparam name="TCmp">Component type.</typeparam>
    /// <param name="entity">Entity to check for component.</param>
    /// <returns>True if entity has component.</returns>
    template <typename TCmp>
    bool HasComponent(const Entity& entity)
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
    void AddComponent(const Entity& entity)
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
    TCmp& GetComponent(const Entity& entity)
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
    std::vector<ComponentBase*> GetComponents(const Entity& entity)
    {
        BX_ENSURE(IsValid(entity));

        std::vector<ComponentBase*> cmps;
        auto& cmpHandles = GetComponentHandles(entity);
        for (SizeType i = 0; i < cmpHandles.GetSize(); i++)
        {
            if (!cmpHandles.IsUsed(i))
                continue;

            auto& cmpHandle = cmpHandles.Get(i);
            auto cmp = (ComponentBase*)m_cmpPoolMap[cmpHandle.mask]->GetPtr(cmpHandle.idx);

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
    void RemoveComponent(const Entity& entity)
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
    void Destroy(Entity& entity)
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

            auto pCmp = static_cast<ComponentBase*>(m_cmpPoolMap[cmpHandle.mask]->GetPtr(cmpHandle.idx));
            pCmp->OnRemoved();

            m_cmpPoolMap[cmpHandle.mask]->Remove(cmpHandle.idx);
            cmpHandle.idx = 0;
            cmpHandle.mask.reset();
        }

        cmpHandles.Clear();
        entityMask.reset();

        // Update registries
        m_cmpHandles.erase(entity.GetId());
        m_cmpMaskMap.erase(entity.GetId());
        m_entities.Remove(entity);

        entity.m_id = INVALID_ENTITY_ID;
        entity.m_pEntityMgr = nullptr;
    }

    /// <summary>
    /// Calculates the component mask for a give set of component types.
    /// </summary>
    /// <typeparam name="...TCmps">Set of component types.</typeparam>
    /// <returns>Component mask from component set.</returns>
    template <typename ... TCmps>
    ComponentMask GetComponentMask()
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
    ComponentMask& GetComponentMask(const Entity& entity)
    {
        BX_ENSURE(IsValid(entity));
        auto it = m_cmpMaskMap.find(entity.GetId());
        BX_ENSURE(it != m_cmpMaskMap.end());

        return it->second;
    }

    Pool<ComponentHandle>& GetComponentHandles(const Entity& entity)
    {
        BX_ENSURE(IsValid(entity));
        auto it = m_cmpHandles.find(entity.GetId());
        BX_ENSURE(it != m_cmpHandles.end());

        return it->second;
    }

    /// <summary>
    /// End of recursion unpack impl.
    /// </summary>
    template<typename TCmp>
    TCmp& UnpackImpl(const Entity& entity)
    {
        return GetComponent<TCmp>(entity);
    }

    /// <summary>
    /// Recursion unpack impl.
    /// </summary>
    template<typename TCmpA, typename TCmpB, typename ... TCmps>
    TCmpA& UnpackImpl(const Entity& entity)
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
    void Unpack(const Entity& entity, typename meta::identity<ForEachCallback<TCmps...>>::type callback)
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
    Pool<TCmp>& GetPool()
    {
        const ComponentMask& cmpMask = ComponentId<TCmp>::Mask();
        Pool<TCmp>* pPool;

        if (m_cmpPoolMap.find(cmpMask) != m_cmpPoolMap.end())
        {
            pPool = static_cast<Pool<TCmp>*>(m_cmpPoolMap[cmpMask]);
            return *pPool;
        }

        pPool = new Pool<TCmp>(TCmp(), BX_ECS_POOL_SIZE);
        m_cmpPoolMap[cmpMask] = pPool;
        return *pPool;
    }

private:
    Pool<Entity> m_entities{};
    HashMap<EntityId, ComponentMask> m_cmpMaskMap{};
    HashMap<EntityId, Pool<ComponentHandle>> m_cmpHandles{};
    HashMap<ComponentMask, IPool*> m_cmpPoolMap{};
};

/// <summary>
/// Implementation of Entity::IsValid
/// </summary>
inline bool Entity::IsValid() const
{
    if (!m_pEntityMgr) return false;
    return m_pEntityMgr->IsValid(*this);
}

/// <summary>
/// Implementation of Entity::HasComponent
/// </summary>
template <typename TCmp>
inline bool Entity::HasComponent() const
{
    if (!m_pEntityMgr) return false;
    return m_pEntityMgr->HasComponent<TCmp>(*this);
}

/// <summary>
/// Implementation of Entity::AddComponent
/// </summary>
template <typename TCmp>
inline TCmp& Entity::AddComponent() const
{
    BX_ENSURE(m_pEntityMgr != nullptr);
    m_pEntityMgr->AddComponent<TCmp>(*this);
    return m_pEntityMgr->GetComponent<TCmp>(*this);
}

/// <summary>
/// Implementation of Entity::GetComponent
/// </summary>
template <typename TCmp>
inline TCmp& Entity::GetComponent() const
{
    BX_ENSURE(m_pEntityMgr != nullptr);
    return m_pEntityMgr->GetComponent<TCmp>(*this);
}

/// <summary>
/// Implementation of Entity::RemoveComponent
/// </summary>
template <typename TCmp>
inline void Entity::RemoveComponent() const
{
    BX_ENSURE(m_pEntityMgr != nullptr);
    m_pEntityMgr->RemoveComponent<TCmp>(*this);
}

/// <summary>
/// Implementation of Entity::Destroy
/// </summary>
inline void Entity::Destroy()
{
    BX_ENSURE(m_pEntityMgr != nullptr);
    m_pEntityMgr->Destroy(*this);
}

/// <summary>
/// Implementation of Entity::GetComponentMask
/// </summary>
inline ComponentMask Entity::GetComponentMask()
{
    BX_ENSURE(m_pEntityMgr != nullptr);
    return m_pEntityMgr->GetComponentMask(*this);
}

template<typename ... TCmps>
inline bool Entity::HasComponents()
{
    BX_ENSURE(m_pEntityMgr != nullptr);
    auto mask = m_pEntityMgr->GetComponentMask<TCmps...>();
    return HasMask(GetComponentMask(), mask);
}

/// <summary>
/// Implementation of Entity::GetComponentPtr
/// </summary>
template <typename TCmp>
inline TCmp* Entity::GetComponentPtr() const
{
    BX_ENSURE(m_pEntityMgr != nullptr);
    return &m_pEntityMgr->GetComponent<TCmp>(*this);
}

inline std::vector<ComponentBase*> Entity::GetComponents() const
{
    BX_ENSURE(m_pEntityMgr != nullptr);
    return m_pEntityMgr->GetComponents(*this);
}

class BX_API SystemBase
{
    BX_TYPE(SystemBase)

private:
    friend class SystemManager;

    virtual void Initialize(EntityManager& e) = 0;
    virtual void Shutdown(EntityManager& e) = 0;

    virtual void Update(EntityManager& e) = 0;
    virtual void Render(EntityManager& e) = 0;
};

template <typename TSys>
class BX_API System : public SystemBase
{
public:
};

class BX_API SystemManager
{
    BX_TYPE(SystemManager)
    BX_NOCOPY(SystemManager)

public:
    SystemManager()
    {
    }

    ~SystemManager()
    {
        m_systems.clear();
        m_systemsMap.clear();
    }

    void Initialize(EntityManager& entityMgr)
    {
        for (auto& sys : m_systems)
        {
            sys->Initialize(entityMgr);
        }
    }

    void Shutdown(EntityManager& entityMgr)
    {
        for (auto& sys : m_systems)
        {
            sys->Shutdown(entityMgr);
        }
    }

    void Update(EntityManager& entityMgr)
    {
        PROFILE_FUNCTION();

        for (auto& sys : m_systems)
        {
            sys->Update(entityMgr);
        }
    }

    void Render(EntityManager& entityMgr)
    {
        PROFILE_FUNCTION();

        for (auto& sys : m_systems)
        {
            sys->Render(entityMgr);
        }
    }

    template <typename TSys, typename... TArgs>
    void AddSystem(TArgs&&... args)
    {
        auto sys = new TSys(std::forward<TArgs>(args)...);
        m_systems.emplace_back(sys);
        m_systemsMap.insert(std::make_pair(Type<TSys>::Id(), sys));
    }

    template <typename TSys>
    TSys& GetSystem()
    {
        SizeType id = Type<TSys>::Id();
        BX_ENSURE(m_systemsMap.find(id) != m_systemsMap.end());

        return *static_cast<TSys*>(m_systemsMap[id]);
    }

    template <typename TSys>
    void RemoveSystem()
    {
        // TODO
    }

private:
    List<SystemBase*> m_systems{};
    HashMap<SizeType, SystemBase*> m_systemsMap{};
};