#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace netcfg {

enum class AdminState
{
    Down,
    Up
};

enum class Duplex
{
    Half,
    Full
};

// `text` contains an IPv4 or IPv6 address; the prefix is stored separately.
struct IpAddress
{
    std::string text;
    std::uint8_t prefix_length;
};

struct Ethernet
{
    std::uint32_t speed_mbps;
    Duplex duplex;
    std::uint32_t mtu;
};

struct Vlan
{
    std::string parent;
    std::uint16_t id;
};

struct Bridge
{
    std::vector<std::string> members;
    bool stp_enabled;
};

using InterfaceKind = std::variant<Ethernet, Vlan, Bridge>;

struct Interface
{
    std::string name;
    InterfaceKind kind;
    AdminState admin_state;
    std::vector<IpAddress> addresses;
};

enum class ErrorCode
{
    None,
    AlreadyExists,
    NotFound,
    InvalidValue,
    InvalidReference,
    InUse,
};

struct Result
{
    ErrorCode code;
    std::string message;

    [[nodiscard]] bool ok() const noexcept
    {
        return code == ErrorCode::None;
    }
};

class NetworkConfig
{
public:
    [[nodiscard]] Result add(Interface iface);
    [[nodiscard]] Result update(Interface iface);
    [[nodiscard]] Result remove(std::string_view name);

private:
    std::map<std::string, Interface, std::less<>> _interfaces;
};

}  // namespace netcfg
