#pragma once
#include "SharpProductKinds.hpp"

class PC1500Machine;
class PC1600Machine;

// The loader memory access (C1600-2 AC-14, design §3.1) over the core's
// machine classes: the objects the interface machines list as their
// `c1600.loader-memory` kind, and the ones the format-1 preset runner makes
// over its core until M-46. Stateless wrappers; this header names the
// classes only by declaration.

namespace sharp {

class PC1500LoaderMemory final : public LoaderMemory {
public:
    explicit PC1500LoaderMemory(PC1500Machine& m) : m_(m) {}
    std::uint8_t peek(std::uint16_t address, Processor) const override;
    Mode mode() const override { return Mode::None; }
    int programArea() const override { return 0; }
    Write writeBuiltIn(std::uint16_t address, const std::uint8_t* data, std::size_t size) override;
    Write writeSlot(int, std::uint16_t address, const std::uint8_t*, std::size_t) override { return {false, address}; }
    bool slotRam(int, std::uint16_t, std::size_t) const override { return false; }
    Module module(int) const override { return {}; }
    std::vector<std::uint8_t> internalRam() const override { return {}; }

private:
    PC1500Machine& m_;
};

class PC1600LoaderMemory final : public LoaderMemory {
public:
    explicit PC1600LoaderMemory(PC1600Machine& m) : m_(m) {}
    std::uint8_t peek(std::uint16_t address, Processor) const override;
    Mode mode() const override;
    int programArea() const override;
    Write writeBuiltIn(std::uint16_t address, const std::uint8_t* data, std::size_t size) override;
    Write writeSlot(int bank, std::uint16_t address, const std::uint8_t* data, std::size_t size) override;
    bool slotRam(int bank, std::uint16_t address, std::size_t size) const override;
    Module module(int slot) const override;
    std::vector<std::uint8_t> internalRam() const override;

private:
    PC1600Machine& m_;
};

} // namespace sharp
