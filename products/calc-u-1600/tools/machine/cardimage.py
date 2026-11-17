"""Reads the byte image of a saved card and the RAM disk on it.

A card comes from the step `saveas: live slot-N:file:NAME.card.yaml`. The format is
docs/Memory-Card-Definition-Format.md of Calc-U-1600, chapter 6. The RAM disk layout follows
the PC-1600 file system (header, FAT, directory). Project neutral, no PC-1600 is needed to use this module.
"""

import re
from dataclasses import dataclass
from pathlib import Path

PAGE_SIZE = 0x4000
HEADER_SIZE = 0x20
DIR_ENTRY_SIZE = 32
FILE_MODULE_ID = 0x55


class CardFormatError(ValueError):
    """The card file does not have the expected form."""


def read_card(path):
    """Returns the bytes of all banks of a saved card, in bank order."""
    text = Path(path).read_text()
    if text.count("initial-content:") != 1:
        raise CardFormatError(f"{path}: expected exactly one region with content")
    size = re.search(r"^\s*bank-size:\s*(0x[0-9A-Fa-f]+|\d+)", text, re.M)
    if not size:
        raise CardFormatError(f"{path}: no bank-size, only banked cards are supported")
    bank_size = int(size.group(1), 0)
    content = text[text.index("initial-content:"):]
    banks = re.split(r"^\s*- bank:\s*", content, flags=re.M)[1:]
    if not banks:
        raise CardFormatError(f"{path}: no banks")
    image = bytearray()
    for expected, block in enumerate(banks):
        number = int(block.split()[0], 0)
        if number != expected:
            raise CardFormatError(f"{path}: bank {expected} expected, found {number}")
        image += _read_bank(block, bank_size)
    return bytes(image)


def _read_bank(block, bank_size):
    """Decodes one addressed-hex block. The lines must cover the bank without a gap."""
    if "encoding: addressed-hex" not in block:
        raise CardFormatError("only addressed-hex is supported")
    lines = re.findall(r"^\s*\$([0-9A-Fa-f]+):\s*(.+?)\s*$", block, re.M)
    bank = bytearray()
    pending_run = None
    for address, data in lines:
        address = int(address, 16)
        if pending_run is not None:
            bank += bytes([pending_run]) * (address - len(bank))
            pending_run = None
        if address != len(bank):
            raise CardFormatError(f"line ${address:04X} does not follow ${len(bank):04X}")
        if data.endswith("..."):
            pending_run = int(data[:-3], 16)
        else:
            bank += bytes.fromhex(data)
    if pending_run is not None:
        bank += bytes([pending_run]) * (bank_size - len(bank))
    if len(bank) != bank_size:
        raise CardFormatError(f"bank has {len(bank)} bytes, expected {bank_size}")
    return bytes(bank)


@dataclass(frozen=True)
class DirEntry:
    name: str
    extension: str
    attribute: int
    first_cluster: int
    size: int


class RamDisk:
    """The RAM disk in a card image. base_page is the 16 KB page of the header."""

    def __init__(self, image, base_page=0):
        self.image = image
        self.base = base_page * PAGE_SIZE
        header = image[self.base:self.base + HEADER_SIZE]
        self.header = header
        self.module_id = header[0x00]
        self.write_flag = header[0x07]
        self.media_id = header[0x08]
        self.sector_size = _word(header, 0x09)
        self.sectors_per_cluster = header[0x0D] + 1
        self.first_fat_sector = _word(header, 0x0F)
        self.fat_count = header[0x11]
        self.max_dir = header[0x12]
        self.first_data_sector = _word(header, 0x13)
        self.max_cluster = _word(header, 0x15)
        self.fat_size = header[0x17]
        self.first_dir_sector = _word(header, 0x18)
        self.sectors_per_track = header[0x1A]

    @property
    def format_row(self):
        """The 19 bytes that SET ID takes, as in init/design.md."""
        return bytes(self.header[0x08:0x1B])

    @property
    def checksum_ok(self):
        return sum(self.header) & 0xFF == 0

    @property
    def cluster_size(self):
        return self.sectors_per_cluster * self.sector_size

    @property
    def data_offset(self):
        """Offset of the data area, relative to the header."""
        return self.first_data_sector * self.sector_size

    @property
    def size(self):
        """Size of the RAM disk in bytes, relative to the header."""
        return self.data_offset + (self.max_cluster - 1) * self.cluster_size

    def sectors(self, first, count=1):
        start = self.base + first * self.sector_size
        return self.image[start:start + count * self.sector_size]

    def fat(self, number=0):
        """FAT number as bytes: media ID, then one byte per cluster."""
        return self.sectors(self.first_fat_sector + number * self.fat_size, self.fat_size)

    def directory(self):
        """The bytes of all directory sectors."""
        return self.sectors(self.first_dir_sector, self.first_data_sector - self.first_dir_sector)

    def files(self):
        """All entries in use. A first byte of 0 is a free entry, &E5 a deleted one."""
        data = self.directory()[:self.max_dir * DIR_ENTRY_SIZE]
        entries = (data[i:i + DIR_ENTRY_SIZE] for i in range(0, len(data), DIR_ENTRY_SIZE))
        return [_entry(raw) for raw in entries if raw[0] not in (0x00, 0xE5)]

    def clusters(self, entry):
        """The cluster chain of a file."""
        fat = self.fat()
        chain = []
        cluster = entry.first_cluster
        while cluster != 0xFF:
            if not 1 <= cluster < self.max_cluster or cluster in chain:
                raise CardFormatError(f"bad cluster {cluster} after {chain}")
            chain.append(cluster)
            cluster = fat[cluster]
        return chain

    def read(self, entry):
        """The content of a file."""
        data = bytearray()
        for cluster in self.clusters(entry):
            first = self.first_data_sector + (cluster - 1) * self.sectors_per_cluster
            data += self.sectors(first, self.sectors_per_cluster)
        return bytes(data[:entry.size])


def _word(data, offset):
    return data[offset] | data[offset + 1] << 8


def _entry(raw):
    return DirEntry(
        name=raw[:8].decode("latin-1").rstrip(),
        extension=raw[8:11].decode("latin-1").rstrip(),
        attribute=raw[0x0B],
        first_cluster=raw[0x1A],
        size=int.from_bytes(raw[0x1C:0x20], "little"),
    )
