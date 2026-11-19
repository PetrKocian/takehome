#include "network_config.h"

#include <iostream>
#include <string_view>

namespace
{

bool apply(netcfg::Result result, std::string_view action)
{
    if (!result.ok())
    {
        std::cerr << action << " failed: " << result.message << '\n';
        return false;
    }

    std::cout << action << '\n';
    return true;
}

}  // namespace

int main()
{
    using namespace netcfg;

    NetworkConfig config;

    Interface eth0{"eth0", Ethernet{1000, Duplex::Full, 1500}, AdminState::Up, {}};
    Interface eth1{"eth1", Ethernet{1000, Duplex::Full, 1500}, AdminState::Up, {}};
    Interface vlan{"eth0.100", Vlan{"eth0", 100}, AdminState::Up, {}};
    Interface bridge{"br0", Bridge{{"eth1", "eth0.100"}, false}, AdminState::Up,
                     {{"192.0.2.1", 24}}};

    if (!apply(config.add(eth0), "Added eth0") ||
        !apply(config.add(eth1), "Added eth1") ||
        !apply(config.add(vlan), "Added eth0.100") ||
        !apply(config.add(bridge), "Added br0"))
    {
        return 1;
    }

    // Update the bridge to enable STP and add an IPv6 address.
    bridge.kind = Bridge{{"eth1", "eth0.100"}, true};
    bridge.addresses.push_back({"2001:db8::1", 64});
    if (!apply(config.update(bridge), "Updated br0: STP on, IPv6 added"))
    {
        return 1;
    }

    // A member of br0 cannot have its own IP address.
    eth1.addresses.push_back({"192.0.2.2", 24});
    const Result address_result = config.update(eth1);
    if (address_result.code != ErrorCode::InvalidReference)
    {
        std::cerr << "Expected InvalidReference for eth1 update, got error code "
                  << static_cast<int>(address_result.code) << ": "
                  << address_result.message << '\n';
        return 1;
    }
    std::cout << "Rejected eth1 update: " << address_result.message << '\n';

    // eth0 cannot be removed while eth0.100 depends on it.
    const Result removal_result = config.remove("eth0");
    if (removal_result.code != ErrorCode::InUse)
    {
        std::cerr << "Expected InUse for eth0 removal, got error code "
                  << static_cast<int>(removal_result.code) << ": "
                  << removal_result.message << '\n';
        return 1;
    }
    std::cout << "Rejected eth0 removal: " << removal_result.message << '\n';

    if (!apply(config.remove("br0"), "Removed br0") ||
        !apply(config.remove("eth0.100"), "Removed eth0.100") ||
        !apply(config.remove("eth1"), "Removed eth1") ||
        !apply(config.remove("eth0"), "Removed eth0"))
    {
        return 1;
    }
}
