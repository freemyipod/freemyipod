"""Encrypt block-aligned data with the nano5g AES engine."""
import argparse
import os
import tempfile
from pathlib import Path
from .session import Session

CHUNK = 65536

class Engine:
    def __init__(self, em):
        self.em = em
        self.addr = em.memalign(32, CHUNK + 64)
        if not self.addr:
            raise RuntimeError("device AES buffer allocation failed")

    def close(self):
        if self.addr:
            self.em.free(self.addr)
            self.addr = 0

    def encrypt(self, data, keysel=1, key=b"", iv=bytes(16), ecb=False):
        keybits = len(key) * 8 if keysel == 0 else 128
        out = bytearray()
        for off in range(0, len(data), CHUNK):
            block = data[off:off + CHUNK]
            self.em.write(self.addr, key.ljust(32, b"\0") + iv + block)
            rc = self.em.aesencrypt_ext(self.addr, len(block), keysel, keybits, ecb, not ecb)
            if rc:
                raise RuntimeError("device AES status %d" % rc)
            out.extend(self.em.read(self.addr + 48, len(block)))
            if not ecb:
                iv = self.em.read(self.addr + 32, 16)
        return bytes(out), iv

def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("operation", choices=("encrypt",))
    p.add_argument("input", type=Path)
    p.add_argument("-o", "--output", type=Path, required=True)
    p.add_argument("--key", type=bytes.fromhex, help="hex user key")
    p.add_argument("--keysel", type=int, default=1, help="hardware selector (default 1)")
    p.add_argument("--iv", type=bytes.fromhex, default=bytes(16))
    p.add_argument("--ecb", action="store_true")
    a = p.parse_args(argv)
    session = Session()
    engine = None
    temporary = None
    try:
        engine = Engine(session.em)
        with a.input.open("rb") as src, tempfile.NamedTemporaryFile(
                mode="wb", dir=a.output.parent, prefix=".n5g-aes-", delete=False) as dst:
            temporary = Path(dst.name)
            iv = a.iv
            while True:
                data = src.read(CHUNK)
                if not data:
                    break
                result, iv = engine.encrypt(data, 0 if a.key is not None else a.keysel,
                                             a.key or b"", iv, a.ecb)
                dst.write(result)
        os.replace(temporary, a.output)
        temporary = None
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
        try:
            if engine is not None:
                engine.close()
        finally:
            session.close()
