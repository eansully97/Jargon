import argparse
import hashlib
import os
import struct
import zlib


def png_chunk(chunk_type: bytes, data: bytes) -> bytes:
    crc = zlib.crc32(chunk_type)
    crc = zlib.crc32(data, crc)
    return struct.pack(">I", len(data)) + chunk_type + data + struct.pack(">I", crc & 0xFFFFFFFF)


def write_solid_png(path: str, width: int, height: int, rgb: tuple[int, int, int]) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)

    r, g, b = rgb
    row = bytes([0]) + bytes([r, g, b]) * width
    raw = row * height

    png = b"\x89PNG\r\n\x1a\n"
    png += png_chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
    png += png_chunk(b"IDAT", zlib.compress(raw, level=6))
    png += png_chunk(b"IEND", b"")

    with open(path, "wb") as file:
        file.write(png)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--prompt-file", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--width", type=int, default=720)
    parser.add_argument("--height", type=int, default=1280)
    args = parser.parse_args()

    with open(args.prompt_file, "r", encoding="utf-8") as file:
        prompt = file.read()

    digest = hashlib.sha256(prompt.encode("utf-8")).digest()

    # Deterministic placeholder color based on prompt.
    color = (
        max(40, digest[0]),
        max(40, digest[1]),
        max(40, digest[2]),
    )

    write_solid_png(args.output, args.width, args.height, color)
    print(f"Wrote dummy card art PNG: {args.output}")


if __name__ == "__main__":
    main()