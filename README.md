# Network configuration model

This C++20 project defines a network interface configuration API for a Linux gateway daemon. `main.cpp` demonstrates building a configuration, updating it, rejecting invalid changes, and tearing it down.

## Build and run

Requires CMake 3.16 or newer, a C++20 compiler, and Linux headers for `inet_pton`.

```sh
./build.sh
./run.sh
```

`build.sh` configures and builds in `build/`. `run.sh` runs the existing `build/demo` executable; it does not build it.

## Design

`Interface` holds the common name, admin state, and IP addresses. Its `std::variant` holds the properties specific to Ethernet, VLAN, or Bridge. This keeps each interface as a value rather than requiring a class hierarchy and virtual methods. The configuration container owns interfaces by name and exposes `add`, `update`, and `remove`; each operation returns a result with an error code and message. An update supplies a complete replacement interface description.

I considered type-specific methods such as `addEthernet()` and `addVlan()`, but chose `add(Interface)` and `update(Interface)` to keep the container API simple. A future JSON parser could construct an `Interface` value before passing it to the container; parsing itself is outside this project.

For `add` and `update`, the container changes a temporary copy, validates the resulting configuration, then commits it with a swap. A rejected change leaves the stored configuration untouched. `remove` checks for dependents before erasing directly. The temporary copy is simpler to reason about than separate validation paths for each kind of update.

## Adding another interface type

To add Bond/LAG, define a `Bond` value type in the public header and add it to `InterfaceKind`. Add checks for bond properties and members to `validate()`. Add a check to `remove()` that prevents removing an interface while a bond uses it as a member. Add a Bond scenario to `main.cpp`. The `add`, `update`, and `remove` method signatures remain unchanged.
