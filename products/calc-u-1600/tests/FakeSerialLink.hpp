#pragma once
#include <cstdint>
#include <deque>
#include <vector>

#include "../core/Serial/SerialLink.hpp"

// The serial-link test double of the core tests (TC8576F, CE-158, the
// PC-1600's COM1:): a queue of bytes the peer delivers, a log of the bytes
// sent, settable modem lines, and what the UART last set.
struct FakeSerialLink : SerialLink {
    std::deque<uint8_t> rx;      // peer -> UART
    std::vector<uint8_t> tx;     // UART -> peer
    Lines lines;                 // reported by getStatus()
    bool armed = true;           // false: the peer stays silent
    bool lastDtr = false, lastRts = false;
    int controlCalls = 0;
    int baudCalls = 0;
    uint32_t lastBaud = 0;

    bool poll(uint8_t& out) override {
        if (!armed || rx.empty()) return false;
        out = rx.front();
        rx.pop_front();
        return true;
    }
    void send(uint8_t b) override { tx.push_back(b); }
    void setControl(bool dtr, bool rts) override {
        lastDtr = dtr;
        lastRts = rts;
        controlCalls++;
    }
    void getStatus(Lines& in) override { in = lines; }
    void onBaud(uint32_t baud, int) override {
        baudCalls++;
        lastBaud = baud;
    }
};
