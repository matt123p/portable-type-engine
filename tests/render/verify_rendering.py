"""Render real-world scenes and compare every pixel with committed PNG goldens."""
import pathlib
import struct
import subprocess
import sys
import tempfile
import zlib


def read_pgm(path):
    magic, dimensions, maximum, pixels = path.read_bytes().split(b"\n", 3)
    if magic != b"P5" or maximum != b"255":
        raise ValueError(f"Unsupported PGM: {path}")
    width, height = map(int, dimensions.split())
    if len(pixels) != width * height:
        raise ValueError(f"Truncated PGM: {path}")
    return width, height, pixels


def read_png(path):
    data = path.read_bytes()
    if not data.startswith(b"\x89PNG\r\n\x1a\n"):
        raise ValueError(f"Unsupported PNG: {path}")
    position = 8
    compressed = bytearray()
    width = height = None
    while position < len(data):
        length = struct.unpack(">I", data[position:position + 4])[0]
        kind = data[position + 4:position + 8]
        payload = data[position + 8:position + 8 + length]
        position += 12 + length
        if kind == b"IHDR":
            width, height, depth, colour, compression, filtering, interlace = struct.unpack(">IIBBBBB", payload)
            if (depth, colour, compression, filtering, interlace) != (8, 0, 0, 0, 0):
                raise ValueError(f"Golden must be 8-bit non-interlaced grayscale: {path}")
        elif kind == b"IDAT":
            compressed.extend(payload)
        elif kind == b"IEND":
            break
    raw = zlib.decompress(compressed)
    stride = width
    previous = bytearray(stride)
    pixels = bytearray()
    offset = 0
    for _ in range(height):
        filter_type = raw[offset]
        current = bytearray(raw[offset + 1:offset + 1 + stride])
        offset += stride + 1
        for x in range(stride):
            left = current[x - 1] if x else 0
            up = previous[x]
            upper_left = previous[x - 1] if x else 0
            if filter_type == 1:
                current[x] = (current[x] + left) & 255
            elif filter_type == 2:
                current[x] = (current[x] + up) & 255
            elif filter_type == 3:
                current[x] = (current[x] + ((left + up) // 2)) & 255
            elif filter_type == 4:
                estimate = left + up - upper_left
                pa = abs(estimate - left)
                pb = abs(estimate - up)
                pc = abs(estimate - upper_left)
                predictor = left if pa <= pb and pa <= pc else up if pb <= pc else upper_left
                current[x] = (current[x] + predictor) & 255
            elif filter_type != 0:
                raise ValueError(f"Unsupported PNG filter {filter_type}: {path}")
        pixels.extend(current)
        previous = current
    return width, height, bytes(pixels)


executable = pathlib.Path(sys.argv[1])
golden_directory = pathlib.Path(sys.argv[2])
with tempfile.TemporaryDirectory(prefix="pte-golden-") as temporary:
    output_directory = pathlib.Path(temporary)
    subprocess.run([executable, output_directory], check=True)
    expected = sorted(golden_directory.glob("*.png"))
    actual = sorted(output_directory.glob("*.pgm"))
    if [p.stem for p in actual] != [p.stem for p in expected]:
        raise SystemExit("Generated scenes do not match the committed golden-image set")
    for rendered, golden in zip(actual, expected):
        current = read_pgm(rendered)
        reference = read_png(golden)
        if current != reference:
            if current[:2] != reference[:2]:
                detail = f"dimensions {current[:2]} != {reference[:2]}"
            else:
                differences = sum(a != b for a, b in zip(current[2], reference[2]))
                detail = f"{differences} pixels differ"
            raise SystemExit(f"Golden mismatch for {golden.name}: {detail}")
print(f"All {len(expected)} rendered scenes match their golden images pixel-for-pixel.")
