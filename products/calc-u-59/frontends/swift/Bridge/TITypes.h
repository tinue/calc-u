#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// Plain C mirrors of the structs Swift reads: the display as drawn (Shared
// LedFrame) and a CPU trace frame (TICpuFrame matches CpuFrame field for field; the layout
// is checked in BridgeConvert.h).

/// The display as drawn (Shared LedFrame): each position's last glyph and its
/// brightness as the eye sees it (DisplayPov in app/FrameSampler).
typedef struct {
    uint8_t  segments[12];    ///< last driven glyph per position: A–G as bits 0–6 (the machine's display output)
    uint8_t  digitLevel[12];  ///< brightness 0 (dark) … 255 (lit) per position
    uint8_t  dpLevel[12];     ///< same for the decimal point of each position
} TIDisplaySnapshot;

/// One instruction's CPU state (Core CpuFrame).
typedef struct {
    // Identity (always captured)
    uint32_t seqno;
    uint16_t pc;
    uint16_t opcode;
    uint8_t  digit;
    uint8_t  postDigit;
    uint8_t  cycleWeight;
    // Light registers
    uint16_t KR, SR, fA, fB, cpuFlags;
    uint8_t  R5;
    // Full snapshot
    uint8_t  A[16], B[16], C[16], D[16], E[16];
    uint8_t  SCOM[16][16];
    uint8_t  Sout[16];
    uint16_t EXT, PREG, flags, m_libAddr;
    uint8_t  REG_ADDR, RAM_ADDR, RAM_OP, m_libAddrReadPos;
    uint8_t  displayOn;
    uint8_t  maxDigitDecay;
} TICpuFrame;

NS_ASSUME_NONNULL_END
