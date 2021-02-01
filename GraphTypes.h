#pragma once

#include <vector>
#include <queue>
#include <tuple>
#include <memory>

/**
 * Base class for an element (or subelement) to indicate that it supports receiving data of this type.
 * Can inherit from it directly, or have member variables that inherit from it with callbacks to the parent class.
 */
template<typename T>
struct IInput { virtual void accept_input(T) = 0; };

/**
 * Base class to indicate that the object inheriting from it can produce output of the given type.
 * Can inherit from it directly, or have member variables that inherit from it with callbacks to the parent class.
 */
template<typename T>
struct IOutput { virtual T return_output(void) = 0; };

/**
 * Constructed with only pointer to parent object. Member function to call is compile-time template parameter.
 * Very useful when you want multiple different inputs of the same type to get pre-processed during the "distribute" step.
 * Calls a function in the parent class, which can do whatever.
 */
template<typename DATA_T, typename PARENT_T, void (PARENT_T::*POINTER_TO_MEMBER)(DATA_T)>
struct MemberInput : IInput<DATA_T>
{
    MemberInput(PARENT_T* pParent)
        : m_pParent(pParent)
    {
        assert(m_pParent != nullptr);
    }

    void accept_input(DATA_T t) override final
    {
        (m_pParent->*POINTER_TO_MEMBER)(std::move(t));
    }
    PARENT_T* m_pParent;
};

/**
 * Simply dumb storage for the input. The class that this BufferedMemberInput is a member variable of needs to reach in and grab the buffer when it's time to process it.
 * Useful for elements that don't need any kind of pre-processing during the distribute step.
 */
template<typename DATA_T, typename PARENT_T, void (PARENT_T::*POINTER_TO_MEMBER)(DATA_T)>
struct BufferedMemberInput : IInput<DATA_T>
{
    void accept_input(DATA_T t) override final
    {
        m_buffer = std::move(t);
    }
    DATA_T m_buffer;
};

/**
 * Note, totally ignorant of it's parent object. Exists only to have the return_output() function, and storage for one buffer of type DATA_T.
 */
template<typename DATA_T>
struct MemberOutput : IOutput<DATA_T>
{
    DATA_T return_output() override final
    {
        // Use std::move because the buffer is consumed upon this function call.
        // If it's not a movable type, the value remains the same.
        // Upto user of this class to make it the right type for what they want it to do.
        // e.g. maybe std::optional<int> is what they want. Maybe just int is what they want.
        // not this class's problem to decide.
        return std::move(m_buffer);
    }
    DATA_T m_buffer;
};

/**
 * Base class of connection objects. The distribute function accepts no arguments. Connections need to know what they are connecting.
 */
struct IConnection { virtual void distribute() = 0; };

#if 0
struct MainComputer : IInput<EjectButton> // you could totally inherit from IInput multiple times, if you wanted to. Different input types each time though. And you can inherit from IOutput multiple times as well.
{
    void accept_missle_ammo_input(AmmoInput);
    void accept_laser_ammo_input(AmmoInput);
    void accept_rifle_ammo_input(AmmoInput);

    MemberInput<AmmoBuffer, MainComputer, &MainComputer::accept_missle_ammo_input> m_weaponOneAmmoInput;
    MemberInput<AmmoBuffer, MainComputer, &MainComputer::accept_laser_ammo_input> m_weaponTwoAmmoInput;
    MemberInput<AmmoBuffer, MainComputer, &MainComputer::accept_rifle_ammo_input> m_weaponThreeAmmoInput;

    MemberOutput<WeaponsCommandBuffer> m_weaponOneOutput;
    MemberOutput<WeaponsCommandBuffer> m_weaponTwoOutput;
    MemberOutput<WeaponsCommandBuffer> m_weaponThreeOutput;

    void accept_input(EjectButtonBuffer) override final; // No need for the MemberInput member variable. Only one of this type.
};
#endif

struct ORGate : IOutput<bool>
{
    void accept_pin1_input(bool pin) { m_pinOne.accept_input(pin); }
    void accept_pin2_input(bool pin) { m_pinTwo.accept_input(pin); }

    BufferedMemberInput<bool, ORGate, &ORGate::accept_pin1_input> m_pinOne;
    BufferedMemberInput<bool, ORGate, &ORGate::accept_pin2_input> m_pinTwo;
    bool m_output;

    void process() { m_output = (m_pinOne.m_buffer || m_pinTwo.m_buffer); }
    bool return_output() override final { return m_output; }
};

struct ANDGate : IOutput<bool>
{
    void accept_pin1_input(bool pin) { m_pinOne.accept_input(pin); }
    void accept_pin2_input(bool pin) { m_pinTwo.accept_input(pin); }

    BufferedMemberInput<bool, ORGate, &ORGate::accept_pin1_input> m_pinOne;
    BufferedMemberInput<bool, ORGate, &ORGate::accept_pin2_input> m_pinTwo;
    bool m_output;

    void process() { m_output = (m_pinOne.m_buffer && m_pinTwo.m_buffer); }
    bool return_output() override final { return m_output; }
};

template<typename SOURCE_ONE_T, typename SOURCE_TWO_T, typename SINK_T>
struct ORGateImm : IConnection
{
    ORGateImm(std::shared_ptr<SOURCE_ONE_T> src1,
        std::shared_ptr<SOURCE_TWO_T> src2, std::shared_ptr<SINK_T> sink)
        : sourceOne(src1), sourceTwo(src2), sink(sink)
    {}

    void distribute() override final
    {
        sink.accept_input(sourceOne.return_output() || sourceTwo.return_output());
    }

    std::shared_ptr<SOURCE_ONE_T> sourceOne;
    std::shared_ptr<SOURCE_TWO_T> sourceTwo;
    std::shared_ptr<SINK_T> sink;
};

template<typename SOURCE_ONE_T, typename SOURCE_TWO_T, typename SINK_T>
struct ANDGateImm : IConnection
{
    ANDGateImm(std::shared_ptr<SOURCE_ONE_T> src1,
        std::shared_ptr<SOURCE_TWO_T> src2, std::shared_ptr<SINK_T> sink)
        : sourceOne(src1), sourceTwo(src2), sink(sink)
    {}

    void distribute() override final
    {
        sink.accept_input(sourceOne.return_output() && sourceTwo.return_output());
    }

    std::shared_ptr<SOURCE_ONE_T> sourceOne;
    std::shared_ptr<SOURCE_TWO_T> sourceTwo;
    std::shared_ptr<SINK_T> sink;
};

template<typename SOURCE, typename SINK>
struct UniDirectionalConnection : IConnection
{
    UniDirectionalConnection(std::shared_ptr<SOURCE> src, std::shared_ptr<SINK> sink)
        : source(src), sink(sink)
    {}

    void distribute() override final
    {
        sink.accept_input(source.return_output());
    }

    std::shared_ptr<SOURCE> source;
    std::shared_ptr<SINK> sink;
};

template<typename A, typename B>
struct BiDirectionalConnection : IConnection
{
    BiDirectionalConnection(std::shared_ptr<A> a, std::shared_ptr<B> b)
        : a(a), b(b)
    {}

    void distribute() override final
    {
        a.accept_input(b.return_output());
        b.accept_input(a.return_output());
    }

    std::shared_ptr<A> a;
    std::shared_ptr<B> b;
};

template<typename A>
struct ReflectionConnection : IConnection
{
    ReflectionConnection(std::shared_ptr<A> a) : a(a) {}

    void distribute() override final
    {
        a.accept_input(a.return_output());
    }

    std::shared_ptr<A> a;
};

/*template<typename SOURCE_T, typename SINK_T, size_t DELAY>
struct PropagationDelayConnection : IConnection
{
    PropagationDelayConnection(std::shared_ptr<SOURCE_T> src, std::shared_ptr<SINK_T> sink)
        : source(src), sink(sink) {}

    void distribute() override final
    {
        m_queue.push_back(source.return_output());
        if (m_queue.size() > DELAY)
        {
            sink.accept_input(m_queue.pop_front());
        }
    }
    std::queue<SOURCE_T::output_type> m_queue;

    std::shared_ptr<SOURCE_T> source;
    std::shared_ptr<SINK_T> sink;
};*/

/*template<typename SOURCE_T, typename ... SINKS_T>
struct FanoutConnection : IConnection
{
    // TODO: One would need a constructor template deducation guide. Which I forget how to write with variadic templates, sorry.
    // Or make the constructor, itself, a template, and cross your fingers that the types match exactly.
    // Or a very different way to handle it is to make the constructor take a std::tuple of the appropriate types already.
    // that's probably what i would do. Making the call-e call it with std::make_tuple() as the second argument isn't really a big deal.
    BiDirectionalConnection<SOURCE_T, SINKS_T>(std::shared_ptr<SOURCE_T> src, std::shared_ptr<SINKS_T>... sinks)
        : m_src(src), m_sinks(std::move(sinks)...)
    {}

    // Store the sink elements in a tuple for easy access later.
    std::tuple<std::shared_ptr<SINKS>...> m_sinks;

    void distribute() override final
    {
        // Std::apply simply calls the function one time for each item in the tuple.
        std::apply([buffer = src.return_output()](auto& sink){ sink.accept_input(buffer); }, m_sinks);
    }
};*/
