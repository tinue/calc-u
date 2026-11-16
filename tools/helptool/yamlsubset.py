"""A YAML reader for the subset the help manifests use (LIB-16 design §3.1).

Block mappings and block sequences by indentation, flow sequences
`[a, b]` and flow mappings `{ k: v }` on one line, plain and quoted
scalars, `#` comments. Anything else is refused with its line.
"""


class YamlError(Exception):
    def __init__(self, line, message):
        super().__init__(f"line {line}: {message}")
        self.line = line


def _strip_comment(text):
    out, quote = [], None
    for i, c in enumerate(text):
        if quote:
            if c == quote:
                quote = None
        elif c in "\"'":
            quote = c
        elif c == "#" and (i == 0 or text[i - 1] in " \t"):
            break
        out.append(c)
    return "".join(out).rstrip()


def _scalar(text, line):
    text = text.strip()
    if not text:
        return None
    if text[0] in "\"'":
        if len(text) < 2 or text[-1] != text[0]:
            raise YamlError(line, f"unterminated quote: {text}")
        return text[1:-1]
    if text[0] in "[{":
        value, rest = _flow(text, 0, line)
        if text[rest:].strip():
            raise YamlError(line, f"text after a flow value: {text[rest:].strip()}")
        return value
    if text[0] in "|>&*!%@`":
        raise YamlError(line, f"not in the manifest subset: {text}")
    if text.lstrip("-").isdigit():
        return int(text)
    if text in ("true", "false"):
        return text == "true"
    return text


def _flow(text, i, line):
    """Parses a flow sequence or mapping starting at text[i]; returns (value, next index)."""
    opening = text[i]
    closing = "]" if opening == "[" else "}"
    items = [] if opening == "[" else {}
    i += 1
    while True:
        while i < len(text) and text[i] == " ":
            i += 1
        if i >= len(text):
            raise YamlError(line, f"unterminated {opening}")
        if text[i] == closing:
            return items, i + 1
        if text[i] in "[{":
            value, i = _flow(text, i, line)
            if opening == "{":
                raise YamlError(line, "a flow value as a key")
            items.append(value)
        else:
            j, quote = i, None
            while j < len(text):
                c = text[j]
                if quote:
                    if c == quote:
                        quote = None
                elif c in "\"'":
                    quote = c
                elif c in ",]}" or (opening == "{" and c == ":" and (j + 1 >= len(text) or text[j + 1] == " ")):
                    break
                j += 1
            token = text[i:j]
            if opening == "{":
                if j >= len(text) or text[j] != ":":
                    raise YamlError(line, f"a flow mapping entry without a colon: {token}")
                key = _scalar(token, line)
                j += 1
                while j < len(text) and text[j] == " ":
                    j += 1
                if j < len(text) and text[j] in "[{":
                    value, j = _flow(text, j, line)
                else:
                    k, quote = j, None
                    while k < len(text):
                        c = text[k]
                        if quote:
                            if c == quote:
                                quote = None
                        elif c in "\"'":
                            quote = c
                        elif c in ",}":
                            break
                        k += 1
                    value = _scalar(text[j:k], line)
                    j = k
                items[key] = value
            else:
                items.append(_scalar(token, line))
            i = j
        while i < len(text) and text[i] == " ":
            i += 1
        if i < len(text) and text[i] == ",":
            i += 1


def loads(text):
    lines = []
    for number, raw in enumerate(text.splitlines(), 1):
        if "\t" in raw[: len(raw) - len(raw.lstrip())]:
            raise YamlError(number, "a tab in the indentation")
        stripped = _strip_comment(raw)
        if stripped.strip():
            lines.append((number, len(stripped) - len(stripped.lstrip()), stripped.strip()))
    if not lines:
        return {}
    value, pos = _block(lines, 0, lines[0][1])
    if pos != len(lines):
        raise YamlError(lines[pos][0], "unexpected indentation")
    return value


def _split_key(content, line):
    if content[0] in "\"'":
        end = content.find(content[0], 1)
        if end < 0:
            raise YamlError(line, "unterminated quote")
        key, rest = content[1:end], content[end + 1:]
        if not rest.startswith(":"):
            raise YamlError(line, "a quoted key without a colon")
        return key, rest[1:]
    idx = content.find(": ")
    if content.endswith(":") and (idx < 0 or idx > len(content) - 2):
        idx = len(content) - 1
    if idx < 0:
        return None, None
    key = content[:idx]
    if key.lstrip("-").isdigit():
        key = int(key)
    return key, content[idx + 1:]


def _block(lines, pos, indent):
    number, ind, content = lines[pos]
    if content.startswith("- ") or content == "-":
        seq = []
        while pos < len(lines) and lines[pos][1] == indent and (lines[pos][2].startswith("- ") or lines[pos][2] == "-"):
            number, ind, content = lines[pos]
            item = content[1:].strip()
            pos += 1
            if not item:
                if pos < len(lines) and lines[pos][1] > indent:
                    value, pos = _block(lines, pos, lines[pos][1])
                else:
                    value = None
                seq.append(value)
                continue
            key, rest = _split_key(item, number) if item[0] not in "[{" else (None, None)
            if key is not None:
                # A mapping that starts on the dash's line.
                sub = [(number, indent + 2, item)]
                while pos < len(lines) and lines[pos][1] > indent:
                    sub.append(lines[pos])
                    pos += 1
                value, used = _block(sub, 0, indent + 2)
                if used != len(sub):
                    raise YamlError(sub[used][0], "unexpected indentation")
                seq.append(value)
            else:
                seq.append(_scalar(item, number))
        return seq, pos
    mapping = {}
    while pos < len(lines) and lines[pos][1] == indent:
        number, ind, content = lines[pos]
        if content.startswith("- "):
            raise YamlError(number, "a sequence item inside a mapping")
        key, rest = _split_key(content, number)
        if key is None:
            raise YamlError(number, f"not a key: {content}")
        if key in mapping:
            raise YamlError(number, f"key {key} given twice")
        pos += 1
        if rest.strip():
            mapping[key] = _scalar(rest, number)
        elif pos < len(lines) and lines[pos][1] > indent:
            mapping[key], pos = _block(lines, pos, lines[pos][1])
        elif pos < len(lines) and lines[pos][1] == indent and lines[pos][2].startswith("- "):
            mapping[key], pos = _block(lines, pos, indent)
        else:
            mapping[key] = None
    if pos < len(lines) and lines[pos][1] > indent:
        raise YamlError(lines[pos][0], "unexpected indentation")
    return mapping, pos
