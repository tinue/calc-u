// What tests/mac_attach_check.sh does to the Mac app's window (C59-5 S6):
//   swift tests/mac_window.swift cover <seconds>   a window over every screen
//                                                  for that long (the app's
//                                                  window is covered, AC-9)
//   swift tests/mac_window.swift hide <pid>        hides the app (Cmd-H)
//   swift tests/mac_window.swift unhide <pid>      shows it again
import AppKit

let args = CommandLine.arguments.dropFirst()
switch args.first {
case "cover":
    let seconds = Double(args.dropFirst().first ?? "30") ?? 30
    let app = NSApplication.shared
    app.setActivationPolicy(.accessory)
    var windows: [NSWindow] = []
    for screen in NSScreen.screens {
        let w = NSWindow(contentRect: screen.frame, styleMask: .borderless, backing: .buffered, defer: false)
        w.level = .floating
        w.backgroundColor = .darkGray
        w.isOpaque = true
        w.ignoresMouseEvents = true
        w.orderFrontRegardless()
        windows.append(w)
    }
    app.activate(ignoringOtherApps: true)
    DispatchQueue.main.asyncAfter(deadline: .now() + seconds) { app.terminate(nil) }
    app.run()
case "hide", "unhide":
    guard let pid = Int32(args.dropFirst().first ?? ""),
          let app = NSRunningApplication(processIdentifier: pid) else { exit(2) }
    let ok = args.first == "hide" ? app.hide() : app.unhide()
    exit(ok ? 0 : 1)
default:
    FileHandle.standardError.write("usage: mac_window.swift cover <s> | hide <pid> | unhide <pid>\n".data(using: .utf8)!)
    exit(2)
}
