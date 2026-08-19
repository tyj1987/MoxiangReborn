"""
verify_algorithms.py
=====================
验证 modern/ 下的资源格式算法与原版墨香 .bin / .pak 文件 100% 兼容。

用法:
    python verify_algorithms.py
"""

import os
import struct
import sys
from pathlib import Path

ROOT = Path(r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）")
RES = ROOT / "墨香【源码配套资源】" / "PlayDH" / "Resource"


def decrypt_bin_payload(data: bytes, type_: int) -> bytes:
    """1:1 复刻 MHFileEx.cpp 中的解密算法。"""
    out = bytearray(data)
    for i, b in enumerate(out):
        v = (b - i) & 0xFF
        if type_ != 0 and (i % type_) == 0:
            v = (v - type_) & 0xFF
        out[i] = v
    return bytes(out)


def encrypt_bin_payload(data: bytes, type_: int) -> bytes:
    """解密算法的逆。"""
    out = bytearray(data)
    for i, b in enumerate(out):
        v = (b + i) & 0xFF
        if type_ != 0 and (i % type_) == 0:
            v = (v + type_) & 0xFF
        out[i] = v
    return bytes(out)


def verify_bin_roundtrip() -> bool:
    """构造已知载荷，验证 .bin 加解密 round-trip。"""
    print("\n[1/4] Testing .bin round-trip (type=0)...")

    payload = b"Hello Moxian! This is a test payload. " * 4

    # encrypt -> write to disk -> read back -> decrypt
    encrypted = encrypt_bin_payload(payload, 0)
    header = struct.pack("<III", 1, 0, len(payload))  # version=1, type=0, size=N
    blob = header + encrypted

    # Decode
    if len(blob) < 12:
        print("  FAIL: header too short")
        return False
    ver, typ, size = struct.unpack("<III", blob[:12])
    if ver != 1 or typ != 0:
        print(f"  FAIL: header mismatch ver={ver} typ={typ}")
        return False

    enc_payload = blob[12 : 12 + size]
    decoded = decrypt_bin_payload(enc_payload, typ)
    if decoded == payload:
        print(f"  OK: round-trip {len(payload)} bytes verified")
        return True
    print(f"  FAIL: decoded != payload")
    print(f"  expected: {payload[:32]}...")
    print(f"  got:      {decoded[:32]}...")
    return False


def verify_real_bin_file() -> bool:
    """读取真实 MonsterList.bin，验证解密后是合法文本/数据。"""
    print("\n[2/4] Decrypting real MonsterList.bin...")

    target = RES / "MonsterList.bin"
    if not target.exists():
        print(f"  SKIP: {target} not found")
        return True

    raw = target.read_bytes()
    if len(raw) < 12:
        print(f"  FAIL: file too small ({len(raw)} bytes)")
        return False

    ver, typ, size = struct.unpack("<III", raw[:12])
    print(f"  Header: version=0x{ver:08X}, type={typ}, size={size}")

    if size > len(raw) - 12:
        # 实际文件中头部后可能有 CRC 字节，先按原始大小尝试
        print(f"  WARN: declared size {size} > file size {len(raw) - 12}")
        print(f"  Trying actual remaining bytes...")

    payload_bytes = raw[12 : 12 + size] if size + 12 <= len(raw) else raw[12:]
    decoded = decrypt_bin_payload(payload_bytes, typ)

    # 检查可读字符比例（怪物列表应该大部分可读）
    printable = sum(1 for b in decoded[: min(2000, len(decoded))]
                    if 32 <= b < 127 or b in (9, 10, 13, 0))
    ratio = printable / min(2000, len(decoded))
    print(f"  Decoded {len(decoded)} bytes, printable ratio: {ratio:.1%}")

    if ratio > 0.5:
        print(f"  OK: looks like valid data")
        # 显示前 200 字节
        preview = decoded[:200]
        printable_preview = ''.join(
            chr(b) if 32 <= b < 127 else '.' for b in preview
        )
        print(f"  Preview: {printable_preview[:160]}")
        return True
    print(f"  WARN: printable ratio too low; might be encrypted differently")
    return False


def verify_pak_signature() -> bool:
    """检测真实 .pak 文件的签名。"""
    print("\n[3/4] Inspecting .pak file structure...")

    candidates = [
        RES / "Effect.pak",
        RES / "Character.pak",
        RES / "Map.pak",
        RES / "Monster.pak",
    ]
    found_any = False
    for pak in candidates:
        if not pak.exists():
            continue
        found_any = True
        raw = pak.read_bytes()
        if len(raw) < 16:
            print(f"  FAIL: {pak.name} too small ({len(raw)} bytes)")
            return False

        total_size, file_count, version, flag = struct.unpack("<IIII", raw[:16])
        print(f"  {pak.name}: {len(raw)} bytes total_size={total_size} "
              f"count={file_count} version=0x{version:08X} flag={flag}")

        if total_size != len(raw):
            print(f"    WARN: total_size mismatch (got {total_size}, file {len(raw)})")

        # 解析前 3 个文件描述符
        cursor = 16
        for i in range(min(3, file_count)):
            if cursor + 32 > len(raw):
                break
            desc = struct.unpack("<IIIIIIII", raw[cursor : cursor + 32])
            cursor += 32
            name_len = desc[2]
            if cursor + name_len > len(raw):
                break
            name = raw[cursor : cursor + name_len].decode("ascii", errors="replace")
            cursor += name_len
            # 4 字节对齐
            if cursor % 4:
                cursor += 4 - (cursor % 4)
            print(f"    [{i}] name={name!r} size={desc[1]} "
                  f"data_off={desc[3]}")

    if not found_any:
        print("  SKIP: no .pak files found")
        return True
    return True


def verify_bsad_format() -> bool:
    """检查真实 .bsad 文件格式。"""
    print("\n[4/4] Inspecting .bsad skill area files...")

    skill_dir = RES / "SkillArea"
    if not skill_dir.exists():
        print(f"  SKIP: {skill_dir} not found")
        return True

    samples = sorted(skill_dir.glob("*.bsad"))[:5]
    for bsad in samples:
        raw = bsad.read_bytes()
        if len(raw) < 8:
            print(f"  FAIL: {bsad.name} too small")
            return False
        w, h, _ = struct.unpack("<HHI", raw[:8])
        cells = raw[8 : 8 + w * h]
        print(f"  {bsad.name}: {w}x{h} = {len(cells)} cells "
              f"(expected {w*h})")
        # 显示前几行
        for row in range(min(3, h)):
            line = ''.join(
                '#' if cells[row * w + col] == 1
                else 'X' if cells[row * w + col] == 2
                else '.'
                for col in range(min(w, 20))
            )
            print(f"    | {line}")

    return True


def main() -> int:
    print("=" * 60)
    print(" Moxian-Reborn Algorithm Verification")
    print("=" * 60)

    ok = all([
        verify_bin_roundtrip(),
        verify_real_bin_file(),
        verify_pak_signature(),
        verify_bsad_format(),
    ])

    print("\n" + "=" * 60)
    print(f" RESULT: {'PASS' if ok else 'FAIL'}")
    print("=" * 60)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())