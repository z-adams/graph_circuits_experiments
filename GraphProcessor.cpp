#include <iostream>
#include <memory>

#include "Nodes.h"

int main()
{
    CircuitData data;
    data.m_nodes.reserve(5);

    auto printer = data.get<Printer<uint32_t>>(data.add<Printer<uint32_t>>());

    std::array<uint32_t, 16> romData{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    auto rom = data.get<ROM<16>>(data.add<ROM<16>>(std::move(romData)));

    SysCircuit::connect(data, rom->m_output, printer->m_input);

    for (size_t clk = 0; clk < 36; clk++)
    {
        SysCircuit::process_all(data);
        SysCircuit::propagate_all(data);
    }

    return 0;
}

#if 0
int main()
{
    std::shared_ptr<MainComputer> comp;
    std::shared_ptr<Missles> miss;
    std::shared_ptr<Laser> las;
    std::shared_ptr<Rifle> gun;
    std::shared_ptr<EjectButton> button;

    std::vector elements{comp, miss, las, gun, button};
    std::vector connections
    {
        UniDirectionalConnection(comp->m_weaponOneOutput, miss),
        UniDirectionalConnection(comp->m_weaponTwoOutput, las),
        UniDirectionalConnection(comp->m_weaponThreeOutput, gun),
        UniDirectionalConnection(miss->m_ammoOutput, comp->m_weaponOneInput),
        UniDirectionalConnection(las->m_ammoOutput, comp->m_weaponTwoInput),
        UniDirectionalConnection(gun->m_ammoOutput, comp->m_weaponThreeInput),
        UniDirectionalConnection(button, comp) // note no member object. Just the accept_input function itself)
    };

    while (true)
    {
        for (auto & elem : elements) { elem->process(); }
        for (auto & conn : connections) { conn->distribute(); }
    }
}
#endif