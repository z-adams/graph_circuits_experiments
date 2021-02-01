#pragma once
#include <iostream>
#include <cstdint>
#include <vector>
#include <memory>
#include <array>

using nodeID_t = uint32_t;
using edgeID_t = uint32_t;

constexpr nodeID_t nullNode_t = uint32_t(0);
constexpr edgeID_t nullEdge_t = uint32_t(0);

// SYSTEM

struct Connection;
struct Node;

template <typename T>
struct NodeTerminal;

struct CircuitData
{
    std::vector<std::shared_ptr<Connection>> m_edges{std::shared_ptr<Connection>{}};
    std::vector<std::shared_ptr<Node>> m_nodes{std::shared_ptr<Node>{}};

    template <typename NODE_T, typename ... ARGS_T>
    nodeID_t add(ARGS_T&& ...args)
    {
        std::shared_ptr<NODE_T> ptr = std::make_shared<NODE_T>(std::forward<ARGS_T>(args)...);
        nodeID_t id = m_nodes.size();
        m_nodes.push_back(ptr);
        return id;
    }

    template <typename NODE_T>
    constexpr std::shared_ptr<NODE_T> get(nodeID_t id)
    {
        return std::static_pointer_cast<NODE_T>(m_nodes.at(id));
    }
};

namespace SysCircuit
{
void process_all(CircuitData& rData);
void propagate_all(CircuitData& rData);

template <typename TYPE_T>
void connect(CircuitData& rData, NodeTerminal<TYPE_T>& a, NodeTerminal<TYPE_T>& b)
{
    edgeID_t id = rData.m_edges.size();
    std::shared_ptr<TYPE_T> connection = std::static_pointer_cast<TYPE_T>(
        rData.m_edges.emplace_back(std::make_shared<TYPE_T>()));

    a.m_id = id;
    b.m_id = id;

    connection->m_in = a.m_parentID;
    connection->m_out = b.m_parentID;
}

}

// GRAPH COMPS

template <typename CONNECTION_T>
struct NodeTerminal
{
    nodeID_t m_parentID{nullNode_t};
    edgeID_t m_id{nullEdge_t};

    CONNECTION_T& get(CircuitData& rData)
    {
        return std::static_pointer_cast<CONNECTION_T>(rData.m_edges.at(m_id));
    }
};

struct Node
{
    virtual void process(CircuitData& rData) = 0;
    virtual void propagate(CircuitData& rData) = 0;
};

struct Connection
{
    typedef void value_type;
};

template <typename DATA_T>
struct WireNode : Connection
{
    typedef DATA_T value_type;

    DATA_T m_value{};
    nodeID_t m_in{nullNode_t};
    nodeID_t m_out{nullNode_t};
};


// Example nodes

struct Constant : public Node
{
    void process(CircuitData&) {}
    void propagate(CircuitData& rData) { m_output.get(rData).m_value = m_state; }

    bool m_state{false};
    NodeTerminal<WireNode<bool>> m_output;
};

struct ANDGate : public Node
{
    void process(CircuitData& rData) override
    {
        m_outVal = m_inA.get(rData).m_value && m_inB.get(rData).m_value;
    }

    void propagate(CircuitData& rData) override
    {
        m_output.get(rData).m_value = m_outVal;
    }

    NodeTerminal<WireNode<bool>> m_inA;
    NodeTerminal<WireNode<bool>> m_inB;
    NodeTerminal<WireNode<bool>> m_output;

    bool m_outVal{false};
};

template <size_t SIZE>
struct ROM : public Node
{
    ROM(std::array<uint32_t, SIZE>&& data) : m_data(std::move(data)) {}

    void process(CircuitData& rData) override
    {

    }

    void propagate(CircuitData& rData) override
    {
        m_output.get(rData).m_value = m_data[m_pc];
        (++m_pc) %= SIZE;
    }

    void jmp(uint32_t addr) { m_pc = addr; }

    std::array<uint32_t, SIZE> m_data;
    uint32_t m_pc{0};

    NodeTerminal<WireNode<uint32_t>> m_output;
};

template <typename DATA_T>
struct Printer : public Node
{
    void process(CircuitData& rData) override
    {
        std::cout << m_input.get(rData).m_value << "\n";
    }

    void propagate(CircuitData&) override {}

    NodeTerminal<WireNode<DATA_T>> m_input;
};
