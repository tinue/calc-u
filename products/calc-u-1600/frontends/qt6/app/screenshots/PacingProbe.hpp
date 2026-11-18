#pragma once
#include <cstdint>

class QApplication;
class MainWindow;

// --pacing-probe <s>: an attach client in
// the app's own process, as the shot runner is. Once the start is over (the
// bare `wait`), `ticks` through attach, `s` seconds of wall time at regular
// speed, `ticks` again; printed with the wall time. Exit 0 when the two grew
// alike within the pacing bound (one host call of 16.7 ms plus the last
// instruction; 25 ms here), else 1.
int runPacingProbe(QApplication& app, MainWindow& window, std::uint16_t attachPort, double seconds);
