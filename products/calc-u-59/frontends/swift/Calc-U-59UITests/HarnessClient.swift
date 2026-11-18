import Foundation

/// A line client for harness attach (docs/formats/harness.md): the test runner runs in the same simulator as the app and reaches
/// its session on 127.0.0.1:<port>, the port the test hands the app with
/// `--attach-port` (one port per simulator).
final class HarnessClient {
    private var fd: Int32 = -1
    private var buffer = Data()

    /// Connects, retrying while the app starts. False after `timeout`.
    func connect(port: UInt16, timeout: TimeInterval = 30) -> Bool {
        let end = Date().addingTimeInterval(timeout)
        while Date() < end {
            let s = socket(AF_INET, SOCK_STREAM, 0)
            var addr = sockaddr_in()
            addr.sin_family = sa_family_t(AF_INET)
            addr.sin_port = port.bigEndian
            addr.sin_addr.s_addr = inet_addr("127.0.0.1")
            let ok = withUnsafePointer(to: &addr) {
                $0.withMemoryRebound(to: sockaddr.self, capacity: 1) {
                    Darwin.connect(s, $0, socklen_t(MemoryLayout<sockaddr_in>.size)) == 0
                }
            }
            if ok {
                fd = s
                return true
            }
            close(s)
            Thread.sleep(forTimeInterval: 0.25)
        }
        return false
    }

    deinit { if fd >= 0 { close(fd) } }

    func send(_ line: String) {
        let data = Array((line + "\n").utf8)
        _ = data.withUnsafeBytes { Darwin.send(fd, $0.baseAddress, data.count, 0) }
    }

    /// The next line, or nil after `timeout` or at the end of the connection.
    func line(timeout: TimeInterval = 10) -> String? {
        let end = Date().addingTimeInterval(timeout)
        while true {
            if let nl = buffer.firstIndex(of: 0x0A) {
                let line = String(decoding: buffer[buffer.startIndex..<nl], as: UTF8.self)
                buffer.removeSubrange(buffer.startIndex...nl)
                return line
            }
            let left = end.timeIntervalSinceNow
            if left <= 0 || fd < 0 { return nil }
            var p = pollfd(fd: fd, events: Int16(POLLIN), revents: 0)
            if poll(&p, 1, Int32(left * 1000)) <= 0 { return nil }
            var chunk = [UInt8](repeating: 0, count: 4096)
            let n = recv(fd, &chunk, chunk.count, 0)
            if n <= 0 { return nil }
            buffer.append(contentsOf: chunk[0..<n])
        }
    }

    /// The reply to `command` (message lines skipped): its first line and its result lines.
    func command(_ command: String, timeout: TimeInterval = 10) -> [String] {
        send(command)
        while let l = line(timeout: timeout) {
            if l.hasPrefix("message ") { continue }
            var out = [l]
            if l.hasPrefix("done "), let n = Int(l.dropFirst(5)) {
                for _ in 0..<n { if let x = line(timeout: timeout) { out.append(x) } }
            }
            return out
        }
        return []
    }

    /// Reads message lines until one matches, false after `timeout`.
    func waitForMessage(timeout: TimeInterval, _ matches: (String) -> Bool) -> Bool {
        let end = Date().addingTimeInterval(timeout)
        while Date() < end {
            guard let l = line(timeout: end.timeIntervalSinceNow) else { return false }
            if l.hasPrefix("message ") && matches(l) { return true }
        }
        return false
    }

    /// Drops what has come so far (the messages of the past).
    func drain(for seconds: TimeInterval = 0.3) {
        let end = Date().addingTimeInterval(seconds)
        while Date() < end, line(timeout: max(0.01, end.timeIntervalSinceNow)) != nil {}
    }

    /// True once the app has closed the connection.
    func closedByPeer(timeout: TimeInterval = 5) -> Bool {
        while let _ = line(timeout: timeout) {}
        var p = pollfd(fd: fd, events: Int16(POLLIN), revents: 0)
        if poll(&p, 1, 100) > 0 {
            var c = [UInt8](repeating: 0, count: 16)
            return recv(fd, &c, c.count, 0) == 0
        }
        return false
    }
}
