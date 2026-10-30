from __future__ import annotations

import json
import shutil
from pathlib import Path

import numpy as np
from PIL import Image

REPO = Path(r"E:\Workspaces\moxiang\repo")
SOURCE_OUT = REPO / "artsource" / "Textures" / "Environment" / "Wuguan" / "Generated"
UNITY_OUT = REPO / "unity" / "MoxiangClient" / "Assets" / "Moxiang" / "ArtV2" / "Environment" / "Wuguan" / "Textures"
SIZE = 1024

MATERIALS = {
    "WOOD": {
        "base": (67, 31, 17), "metal": 0.00, "rough": 0.50, "normal": 3.0, "seed": 1101,
    },
    "LACQUER": {
        "base": (112, 18, 13), "metal": 0.00, "rough": 0.28, "normal": 1.3, "seed": 1102,
    },
    "STONE": {
        "base": (92, 94, 91), "metal": 0.00, "rough": 0.73, "normal": 5.0, "seed": 1103,
    },
    "PLASTER": {
        "base": (151, 139, 112), "metal": 0.00, "rough": 0.82, "normal": 2.4, "seed": 1104,
    },
    "PAPER": {
        "base": (200, 163, 92), "metal": 0.00, "rough": 0.66, "normal": 1.6, "seed": 1105,
    },
    "BRONZE": {
        "base": (111, 65, 30), "metal": 0.82, "rough": 0.36, "normal": 2.0, "seed": 1106,
    },
}


def periodic_noise(size: int, seed: int, beta: float = 2.0) -> np.ndarray:
    rng = np.random.default_rng(seed)
    phase = rng.normal(size=(size, size)) + 1j * rng.normal(size=(size, size))
    fy = np.fft.fftfreq(size)[:, None]
    fx = np.fft.fftfreq(size)[None, :]
    radius = np.sqrt(fx * fx + fy * fy)
    radius[0, 0] = 1.0
    spectrum = phase / np.power(radius, beta / 2.0)
    spectrum[0, 0] = 0.0
    field = np.fft.ifft2(spectrum).real
    field -= field.min()
    field /= max(field.max(), 1e-8)
    return field.astype(np.float32)


def normalize01(a: np.ndarray) -> np.ndarray:
    a = a.astype(np.float32)
    a -= a.min()
    return a / max(float(a.max()), 1e-8)


def material_height(kind: str, seed: int) -> np.ndarray:
    n1 = periodic_noise(SIZE, seed, 2.2)
    n2 = periodic_noise(SIZE, seed + 91, 1.0)
    yy, xx = np.mgrid[0:SIZE, 0:SIZE].astype(np.float32)
    x = xx / SIZE
    y = yy / SIZE

    if kind == "WOOD":
        warp = (n1 - 0.5) * 0.20
        grain = 0.5 + 0.5 * np.sin(2 * np.pi * (8.0 * x + warp + 0.08 * np.sin(2*np.pi*2*y)))
        fine = 0.5 + 0.5 * np.sin(2 * np.pi * (34.0 * x + (n2 - 0.5) * 0.18))
        h = 0.58 * grain + 0.25 * n1 + 0.17 * fine
    elif kind == "LACQUER":
        brush = 0.5 + 0.5 * np.sin(2 * np.pi * (48.0 * x + (n1 - 0.5) * 0.10))
        h = 0.72 * n1 + 0.18 * n2 + 0.10 * brush
    elif kind == "STONE":
        coarse = periodic_noise(SIZE, seed + 13, 3.0)
        pores = np.power(n2, 3.0)
        h = 0.64 * coarse + 0.26 * n1 - 0.10 * pores
    elif kind == "PLASTER":
        h = 0.76 * n1 + 0.24 * n2
    elif kind == "PAPER":
        fiber_x = 0.5 + 0.5 * np.sin(2*np.pi*(96*x + (n1-.5)*0.08))
        fiber_y = 0.5 + 0.5 * np.sin(2*np.pi*(77*y + (n2-.5)*0.08))
        h = 0.55 * n1 + 0.23 * fiber_x + 0.22 * fiber_y
    elif kind == "BRONZE":
        patina = periodic_noise(SIZE, seed + 31, 3.4)
        pitted = np.power(n2, 4.0)
        h = 0.62 * n1 + 0.30 * patina - 0.08 * pitted
    else:
        h = n1
    return normalize01(h)


def base_color(kind: str, base: tuple[int, int, int], height: np.ndarray, seed: int) -> np.ndarray:
    n = periodic_noise(SIZE, seed + 211, 1.8)
    variation = (height - 0.5) * 0.30 + (n - 0.5) * 0.12
    rgb = np.zeros((SIZE, SIZE, 3), dtype=np.float32)
    base_arr = np.array(base, dtype=np.float32) / 255.0
    for c in range(3):
        rgb[..., c] = base_arr[c] * (1.0 + variation)

    if kind == "WOOD":
        rgb[..., 0] *= 1.05
        rgb[..., 2] *= 0.86
    elif kind == "STONE":
        speck = periodic_noise(SIZE, seed + 312, 0.6)
        rgb += ((speck - 0.5) * 0.075)[..., None]
    elif kind == "PLASTER":
        stains = periodic_noise(SIZE, seed + 313, 3.8)
        rgb *= (0.88 + 0.18 * stains)[..., None]
    elif kind == "PAPER":
        fibers = periodic_noise(SIZE, seed + 314, 0.8)
        rgb += ((fibers - 0.5) * 0.06)[..., None]
    elif kind == "BRONZE":
        patina = periodic_noise(SIZE, seed + 315, 3.2)
        mask = np.clip((patina - 0.64) * 4.0, 0, 1)[..., None]
        patina_color = np.array([0.10, 0.31, 0.27], dtype=np.float32)
        rgb = rgb * (1-mask*0.65) + patina_color * mask * 0.65

    return np.clip(rgb, 0, 1)


def normal_map(height: np.ndarray, strength: float) -> np.ndarray:
    dx = (np.roll(height, -1, axis=1) - np.roll(height, 1, axis=1)) * strength
    dy = (np.roll(height, -1, axis=0) - np.roll(height, 1, axis=0)) * strength
    nx = -dx
    ny = -dy
    nz = np.ones_like(height)
    length = np.sqrt(nx*nx + ny*ny + nz*nz)
    normal = np.stack((nx/length, ny/length, nz/length), axis=-1)
    return np.clip(normal * 0.5 + 0.5, 0, 1)


def orm_map(kind: str, height: np.ndarray, metal: float, rough: float, seed: int) -> np.ndarray:
    n = periodic_noise(SIZE, seed + 501, 2.0)
    local_rough = np.clip(rough + (n - 0.5) * 0.18, 0.06, 0.96)
    # Height-driven cavity approximation. Low regions receive slightly more occlusion.
    ao = np.clip(0.82 + height * 0.18, 0, 1)
    local_metal = np.full_like(height, metal)
    if kind == "BRONZE":
        # Patina and pits slightly reduce metallic response.
        local_metal = np.clip(metal - np.maximum(0, 0.55 - height) * 0.28, 0, 1)
    smooth = 1.0 - local_rough
    return np.stack((local_metal, ao, local_rough, smooth), axis=-1)


def save_rgb(path: Path, arr: np.ndarray):
    Image.fromarray((np.clip(arr,0,1)*255.0 + 0.5).astype(np.uint8), "RGB").save(path, optimize=True)


def save_rgba(path: Path, arr: np.ndarray):
    Image.fromarray((np.clip(arr,0,1)*255.0 + 0.5).astype(np.uint8), "RGBA").save(path, optimize=True)


def main():
    SOURCE_OUT.mkdir(parents=True, exist_ok=True)
    UNITY_OUT.mkdir(parents=True, exist_ok=True)
    manifest = {
        "schema": "moxiang.art-v2.generated-pbr.v1",
        "resolution": SIZE,
        "generator": Path(__file__).name,
        "materials": {},
    }

    for kind, cfg in MATERIALS.items():
        h = material_height(kind, cfg["seed"])
        color = base_color(kind, cfg["base"], h, cfg["seed"])
        normal = normal_map(h, cfg["normal"])
        orm = orm_map(kind, h, cfg["metal"], cfg["rough"], cfg["seed"])

        stem = f"MX_WUGUAN_{kind}"
        files = {
            "baseColor": f"{stem}_BaseColor.png",
            "normal": f"{stem}_Normal.png",
            "orm": f"{stem}_ORM.png",
        }
        save_rgb(SOURCE_OUT / files["baseColor"], color)
        save_rgb(SOURCE_OUT / files["normal"], normal)
        save_rgba(SOURCE_OUT / files["orm"], orm)
        for filename in files.values():
            shutil.copy2(SOURCE_OUT / filename, UNITY_OUT / filename)
        manifest["materials"][kind] = {**cfg, "files": files}
        print("MXH_PBR_TEXTURE_OK", kind, files)

    (SOURCE_OUT / "MX_WUGUAN_PBR_MANIFEST.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    print("MXH_WUGUAN_PBR_GENERATION_OK", SOURCE_OUT, UNITY_OUT)


if __name__ == "__main__":
    main()
