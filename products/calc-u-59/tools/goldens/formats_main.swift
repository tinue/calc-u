// Golden generator: dumps the Swift originals of the
// register-line helpers and BCD encoders so the C++ ports in app/ can be
// checked against them. See tools/goldens/README.md.
import Foundation

func num(_ d: Double) -> String { String(format: "%.17g", d) }

func hex(_ b: [UInt8]) -> String { b.map { String(format: "%X", $0) }.joined() }

var out = ""
// No CARD records of 2.3.0's card files (magnetic cards are card format 3
// now) and no STATE records of its state files: neither file is read any more.

// BCD encoders over literals (text path) and doubles (Double path).
let literals = ["0", "-0", "00", "1", "-1", "7", "10", "99", "100", "3641003032", "1234567890123", "12345678901234567",
                "-98765", "0.5", "-1.5e-3", "7.77E22", "1e-99", "9.999999999999e99", "1e100", "1e-100", "3.14159265358979",
                "2.5", "  42  ", "+5", "1.", ".5", "inf", "-inf", "nan", "abc", "", "1e5", "-0.0", "0001", "1E+05",
                "123456789012.5", "0.1", "0.3", "1.0000000000005", "9.9999999999995e8", "5e-324", "1.7976931348623157e308"]
for l in literals { out += "BCDT [\(l)] \(hex(encodeTI59BCD(text: l)))\n" }
var seed: UInt64 = 0x59
func next() -> UInt64 { seed = seed &* 6364136223846793005 &+ 1442695040888963407; return seed >> 11 }
for _ in 0..<300 {
    let mant = Double(next() % 10_000_000_000_000) / 1e12
    let exp = Int(next() % 199) - 99
    let v = (next() % 2 == 0 ? -1.0 : 1.0) * mant * pow(10.0, Double(exp))
    out += "BCDD \(num(v)) \(hex(encodeTI59BCD(v)))\n"
}

// Register-line helpers.
for _ in 0..<50 {
    let nib = (0..<16).map { _ in UInt8(next() % 16) }
    let enc = encodeRegisterLine(nib)
    out += "REG \(hex(nib)) \(enc.map { String(format: "%02X", $0) }.joined()) \(hex(decodeRegisterLine(enc)))\n"
}

// Math tokens.
for s in ["\\lambda\\Lambda", "^{-1}^{1}^{-}", "_{-1}_{1}", "\\leftrightarrows\\leftrightarrow\\leftarrow",
          "x^{2}+y_{i}", "\\blank", "\\pibar", "\\sqrt\\cbrt", "plain"] {
    out += "MATH \(s) \(expandMathTokens(s))\n"
}

FileHandle.standardOutput.write(out.data(using: .utf8)!)
