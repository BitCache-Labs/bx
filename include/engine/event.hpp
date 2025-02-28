#pragma once

#include <engine/api.hpp>
#include <engine/memory.hpp>
#include <engine/guard.hpp>
#include <engine/list.hpp>
#include <engine/hash_map.hpp>
#include <engine/function.hpp>

using SignalId = SizeType;
using SigSPtr = SharedPtr<struct ISignal>;
using SigSlots = HashMap<SignalId, SigSPtr>;

struct BX_API SigHandle
{
    SigHandle() {}
    SigHandle(SignalId id1, SignalId id2)
        : id1(id1), id2(id2) {}

    SignalId id1 = 0;
    SignalId id2 = 0;
};

struct BX_API SignalData
{
    SignalData() {}
    SignalData(SignalId id, SigSlots slots)
        : id(id), slots(slots) {}

    SignalId id = 0;
    SigSlots slots;
};

// TODO: Use TypeId
struct BX_API ISignal
{
    virtual ~ISignal() {}

    virtual void operator()(const void* p) = 0;

protected:
    static SignalId RegisterId()
    {
        static SignalId s_counter = 0;
        return s_counter++;
    }
};

template <typename TEvent>
struct BX_API Signal : public ISignal
{
    using EventFn = Function<void(const TEvent&)>;

    explicit Signal(EventFn sigFn)
        : m_sigFn(sigFn)
    {}

    virtual void operator()(const void* ptr)
    {
        m_sigFn(*(static_cast<const TEvent*>(ptr)));
    }

    static SignalId Id()
    {
        static const SignalId s_id = RegisterId();
        return s_id;
    }

private:
    EventFn m_sigFn;
};

class BX_API Receiver
{
public:
    ~Receiver()
    {
        if (m_sigHandles.size() > 0)
        {
            m_clearSigFn(m_sigHandles);
        }
    }

private:
    friend class Event;
    Function<void(List<SigHandle>&)> m_clearSigFn;
    List<SigHandle> m_sigHandles;
};

// TODO: Rename to EventManager? EventBus?
class BX_API Event
{
    BX_TYPE(Event)
    BX_NOCOPY(Event)

public:
    static void Initialize() {}
    static void Shutdown() {} // TODO, clean memory

    template <typename TEvent, typename TReceiver>
    static void Subscribe(TReceiver& receiver)
    {
        if (receiver.m_sigHandles.size() == 0)
        {
            receiver.m_clearSigFn = std::bind(&Event::ClearSignals, std::placeholders::_1);
        }

        void (TReceiver:: * receive)(const TEvent&) = &TReceiver::Receive;
        auto signal = new Signal<TEvent>(std::bind(receive, &receiver, std::placeholders::_1));

        auto& sigSlots = GetSlotsFor(Signal<TEvent>::Id());
        sigSlots.slots[sigSlots.id] = SigSPtr(signal);

        receiver.m_sigHandles.emplace_back(SigHandle{ Signal<TEvent>::Id(), sigSlots.id });

        sigSlots.id++;
    }

    template <typename TEvent, typename TReceiver>
    static void Unsubscribe(TReceiver& receiver)
    {
        auto& sigSlots = GetSlotsFor(Signal<TEvent>::Id());
        for (auto handle : receiver.m_sigHandles)
        {
            if (handle.id1 == Signal<TEvent>::Id())
            {
                sigSlots.slots.erase(handle.id2);
            }
        }
    }

    template <typename TEvent, typename... TArgs>
    static void Broadcast(TArgs&&... args)
    {
        Broadcast(TEvent(std::forward<TArgs>(args)...));
    }

    template <typename TEvent>
    static void Broadcast(const TEvent& event)
    {
        auto& sigSlots = GetSlotsFor(Signal<TEvent>::Id());
        for (auto sig : sigSlots.slots)
        {
            (*sig.second)(static_cast<const void*>(&event));
        }
    }

private:
    static SignalData& GetSlotsFor(SignalId eId)
    {
        if (eId >= GetBus().size())
        {
            GetBus().resize(eId + 1);
        }
        return GetBus()[eId];
    }

    static void ClearSignals(const List<SigHandle>& sigHandles)
    {
        for (const auto& handle : sigHandles)
        {
            auto& sigSlots = GetSlotsFor(handle.id1);
            sigSlots.slots.erase(handle.id2);
        }
    }

    static List<SignalData>& GetBus()
    {
        static List<SignalData> s_bus;
        return s_bus;
    }
};