#pragma once

/// The model inside the core (TI-58/59 family); above the core it is a ModelId.
enum class MachineVariant { TI59, TI58, TI58C };

/// TI-58C only: external RAM chip (TMC0599) provides constant memory that
/// survives power-off.  Also governs MEMRD/MEMWR instruction decoding, ROM
/// loading, and printer-detection digit (KP.D10 vs KP.D0).
inline bool hasConstantMemory(MachineVariant v) { return v == MachineVariant::TI58C; }

/// TI-59 only: full 120-register RAM, magnetic card reader.  Governs data-
/// register addressing, accessible memory size, and preset handling.
/// TI-58 and TI-58C both use 60 registers and lack a card reader.
inline bool hasLargeMemory(MachineVariant v) { return v == MachineVariant::TI59; }

// The tick rates (455,000 / 384,000 / 394,000 per second) are descriptor
// data: TICatalogue.cpp. Core-internal: nothing above the core names a
// MachineVariant (C59-1 design §4).
