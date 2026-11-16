#pragma once
#include <string>

#include "sessions/SessionPart.hpp"
#include "typing/TypingPart.hpp"

// ── SharpLoaderPart: the program loaders in the app session ─────────────
//
// C1600-2 (design §3.7; AC-21, AC-24, EC-6): the commands and requests of
// app/SharpTopics.hpp's loader block over sharp::loaders (core/Loaders.hpp,
// core/BasicTyping.hpp): what a file is, Load BASIC Program, typing a
// listing in, the one plan for machine code and its load, and loading with
// a start. Stateless: each call carries the file's bytes (D-3). Needs only
// the session's machine and the typing part, so the harness carries it
// before SharpAppPart joins the harness (M-51). Loading BASIC and machine
// code are carried out at once, typing in and loading with a start are
// operations (AC-24); every one is refused while an operation runs.

namespace shared {

class SharpLoaderPart final : public calcu::sessions::SessionPart {
public:
    explicit SharpLoaderPart(calcu::typing::TypingPart& typing) : m_typing(typing) {}

    // D-5: how long `load-code … run` waits for the machine to be idle, in
    // emulated seconds; tests set it shorter.
    void setIdleBound(double seconds) { m_idleBound = seconds; }

    void attach(calcu::sessions::SessionSetup&) override;

private:
    static calcu::sessions::Task<calcu::sessions::OperationResult>
    typeOperation(SharpLoaderPart* self, calcu::sessions::OperationContext& ctx, std::string listing, std::string name);
    static calcu::sessions::Task<calcu::sessions::OperationResult>
    startOperation(SharpLoaderPart* self, calcu::sessions::OperationContext& ctx, calcu::sessions::PayloadPtr loaded,
                   std::string command);

    calcu::typing::TypingPart& m_typing;
    double m_idleBound = 3600;
};

} // namespace shared
