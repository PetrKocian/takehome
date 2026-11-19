#include "network_config.h"

#include <arpa/inet.h>

#include <algorithm>
#include <utility>

namespace netcfg {
namespace {

using InterfaceMap = std::map<std::string, Interface, std::less<>>;

Result success()
{
    return {ErrorCode::None, {}};
}

Result failure(ErrorCode code, std::string message)
{
    return {code, std::move(message)};
}

Result validate(const InterfaceMap& interfaces)
{
    for (const auto& [name, iface] : interfaces)
    {
        if (name.empty())
        {
            return failure(ErrorCode::InvalidValue, "interface name must be nonempty");
        }

        // Check each IP address and its prefix length.
        for (const auto& address : iface.addresses)
        {
            in_addr ipv4{};
            in6_addr ipv6{};
            const bool is_ipv4 = address.text.find('\0') == std::string::npos &&
                                 inet_pton(AF_INET, address.text.c_str(), &ipv4) == 1;
            const bool is_ipv6 = !is_ipv4 && address.text.find('\0') == std::string::npos &&
                                 inet_pton(AF_INET6, address.text.c_str(), &ipv6) == 1;
            if ((!is_ipv4 && !is_ipv6) ||
                address.prefix_length > (is_ipv4 ? 32 : 128))
            {
                return failure(ErrorCode::InvalidValue,
                               "invalid IP address or prefix on " + name);
            }
        }

        // A VLAN needs a valid ID and an Ethernet parent.
        if (const auto* vlan = std::get_if<Vlan>(&iface.kind))
        {
            if (vlan->id < 1 || vlan->id > 4094)
            {
                return failure(ErrorCode::InvalidValue,
                               "VLAN ID must be between 1 and 4094 on " + name);
            }
            const auto parent = interfaces.find(vlan->parent);
            if (parent == interfaces.end() ||
                !std::holds_alternative<Ethernet>(parent->second.kind))
            {
                return failure(ErrorCode::InvalidReference,
                               "VLAN parent must be an Ethernet interface on " + name);
            }
        }

        // Bridge members must exist and have no IP addresses.
        if (const auto* bridge = std::get_if<Bridge>(&iface.kind))
        {
            for (const auto& member_name : bridge->members)
            {
                const auto member = interfaces.find(member_name);
                if (member == interfaces.end())
                {
                    return failure(ErrorCode::InvalidReference,
                                   "bridge member does not exist: " + member_name);
                }
                if (!member->second.addresses.empty())
                {
                    return failure(ErrorCode::InvalidReference,
                                   "bridge member has IP addresses: " + member_name);
                }
            }
        }
    }
    return success();
}

}  // namespace

Result NetworkConfig::add(Interface iface)
{
    if (_interfaces.contains(iface.name))
    {
        return failure(ErrorCode::AlreadyExists,
                       "interface already exists: " + iface.name);
    }

    // Stage the change, validate it, then commit it.
    InterfaceMap candidate = _interfaces;
    std::string name = iface.name;
    candidate.emplace(std::move(name), std::move(iface));
    if (Result result = validate(candidate); !result.ok())
    {
        return result;
    }
    _interfaces.swap(candidate);
    return success();
}

Result NetworkConfig::update(Interface iface)
{
    if (!_interfaces.contains(iface.name))
    {
        return failure(ErrorCode::NotFound,
                       "interface does not exist: " + iface.name);
    }

    // Stage the change, validate it, then commit it.
    InterfaceMap candidate = _interfaces;
    candidate.find(iface.name)->second = std::move(iface);
    if (Result result = validate(candidate); !result.ok())
    {
        return result;
    }
    _interfaces.swap(candidate);
    return success();
}

Result NetworkConfig::remove(std::string_view name)
{
    const auto target = _interfaces.find(name);
    if (target == _interfaces.end())
    {
        return failure(ErrorCode::NotFound,
                       "interface does not exist: " + std::string{name});
    }

    // Another interface cannot reference the one being removed.
    for (const auto& entry : _interfaces)
    {
        if (entry.first == name)
        {
            continue;
        }
        const Interface& iface = entry.second;
        if (const auto* vlan = std::get_if<Vlan>(&iface.kind);
            vlan && vlan->parent == name)
        {
            return failure(ErrorCode::InUse,
                           "interface is a VLAN parent: " + std::string{name});
        }
        if (const auto* bridge = std::get_if<Bridge>(&iface.kind);
            bridge && std::find(bridge->members.begin(), bridge->members.end(), name) !=
                          bridge->members.end())
        {
            return failure(ErrorCode::InUse,
                           "interface is a bridge member: " + std::string{name});
        }
    }

    _interfaces.erase(target);
    return success();
}

}  // namespace netcfg
