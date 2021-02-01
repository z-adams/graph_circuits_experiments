#include "Nodes.h"

void SysCircuit::process_all(CircuitData& rData)
{
    for (auto& node : rData.m_nodes)
    {
        if (node) { node->process(rData); }
    }
}

void SysCircuit::propagate_all(CircuitData& rData)
{
    for (auto& node : rData.m_nodes)
    {
        if (node) { node->propagate(rData); }
    }
}
