# genAni.py
import cv2
import numpy as np
import argparse
import struct

TILE_W, TILE_H = 8, 8
FRAME_W, FRAME_H = 128, 64

def ordered_dither(gray):
    bayer8 = (np.array([
        [0, 48, 12, 60,  3, 51, 15, 63],
        [32, 16, 44, 28, 35, 19, 47, 31],
        [8, 56,  4, 52, 11, 59,  7, 55],
        [40, 24, 36, 20, 43, 27, 39, 23],
        [2, 50, 14, 62,  1, 49, 13, 61],
        [34, 18, 46, 30, 33, 17, 45, 29],
        [10, 58,  6, 54,  9, 57,  5, 53],
        [42, 26, 38, 22, 41, 25, 37, 21],
    ]) * 4).astype(np.uint8)
    h, w = gray.shape
    threshold_map = np.tile(bayer8, (h // 8 + 1, w // 8 + 1))[:h, :w]
    return np.where(gray > threshold_map, 255, 0).astype(np.uint8)

def floyd_steinberg(gray):
    h, w = gray.shape
    out = gray.astype(np.float32).copy()
    for y in range(h):
        for x in range(w):
            old = out[y, x]
            new = 0 if old < 128 else 255
            out[y, x] = new
            err = old - new
            if x+1 < w:           out[y,   x+1] += err * 7/16
            if y+1 < h and x > 0: out[y+1, x-1] += err * 3/16
            if y+1 < h:           out[y+1, x  ] += err * 5/16
            if y+1 < h and x+1<w: out[y+1, x+1] += err * 1/16
    return out.clip(0, 255).astype(np.uint8)

def process_frame(frame, dither):
    frame = cv2.resize(frame, (FRAME_W, FRAME_H))
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    if dither == "ordered":
        return ordered_dither(gray)
    elif dither == "fs":
        return floyd_steinberg(gray)
    else:
        _, bw = cv2.threshold(gray, 127, 255, cv2.THRESH_BINARY)
        return bw

def pack_tile(tile):
    # 8x8 mono → 8 bytes (1 bit/pixel, MSB first)
    bits = (tile.flatten() // 255).astype(np.uint8)
    return np.packbits(bits).tobytes()

def extract_tiles(curr, prev):
    tiles = []
    for ty in range(FRAME_H // TILE_H):
        for tx in range(FRAME_W // TILE_W):
            y, x = ty * TILE_H, tx * TILE_W
            c = curr[y:y+TILE_H, x:x+TILE_W]
            p = prev[y:y+TILE_H, x:x+TILE_W]
            if not np.array_equal(c, p):
                tiles.append((tx, ty, pack_tile(c)))
    return tiles

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--input",  required=True)
    parser.add_argument("-o", "--output", default=None)
    parser.add_argument("-f", "--fps",    type=int, default=24)
    parser.add_argument("-d", "--dither", choices=["none","ordered","fs"],
                        default="ordered")
    args = parser.parse_args()

    out_path = args.output or args.input.rsplit(".", 1)[0] + ".vis"
    cap = cv2.VideoCapture(args.input)

    ret, frame = cap.read()
    if not ret:
        print("Cannot read input"); return

    initial = process_frame(frame, args.dither)
    prev = initial.copy()

    TILES_X = FRAME_W // TILE_W  # 16
    TILES_Y = FRAME_H // TILE_H  # 8

    with open(out_path, "wb") as f:
        # Header: magic(2) + fps(1) + tilesX(1) + tilesY(1) + tileW(1) + tileH(1)
        f.write(b"VS")
        f.write(struct.pack("BBBBB", args.fps, TILES_X, TILES_Y, TILE_W, TILE_H))

        # Initial frame — toàn bộ 128 tiles
        for ty in range(TILES_Y):
            for tx in range(TILES_X):
                y, x = ty * TILE_H, tx * TILE_W
                f.write(pack_tile(initial[y:y+TILE_H, x:x+TILE_W]))

        # Delta frames
        frame_count = 1
        while True:
            ret, frame = cap.read()
            if not ret: break
            curr = process_frame(frame, args.dither)
            tiles = extract_tiles(curr, prev)

            # 1 byte số tile thay đổi, rồi từng tile: tx(1) ty(1) data(8)
            f.write(struct.pack("B", len(tiles)))
            for tx, ty, data in tiles:
                f.write(struct.pack("BB", tx, ty))
                f.write(data)

            prev = curr.copy()
            frame_count += 1
            print(f"Frame {frame_count}    ", end="\r")

    cap.release()
    print(f"\nDone: {frame_count} frames → {out_path}")

if __name__ == "__main__":
    main()