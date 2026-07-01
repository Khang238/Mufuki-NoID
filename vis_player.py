import argparse
import pygame
import struct
import time
import numpy as np

FRAME_W, FRAME_H = 128, 64
TILE_W, TILE_H = 8, 8

def unpack_monochrome(data, size):
    """Unpack 1-bit per pixel array into 0/255 grayscale"""
    bits = np.unpackbits(np.frombuffer(data, dtype=np.uint8))
    return (bits[:size] * 255).astype(np.uint8)

def vis_player(filename, debug=False):
    with open(filename, "rb") as f:
        magic = f.read(2)
        assert magic == b"VS", "Invalid file"
        fps, tiles_x, tiles_y, tile_w, tile_h = struct.unpack("BBBBB", f.read(5))
        FRAME_W = tiles_x * tile_w
        FRAME_H = tiles_y * tile_h
        TILE_W, TILE_H = tile_w, tile_h

        # Init pygame
        pygame.init()
        screen = pygame.display.set_mode((FRAME_W, FRAME_H))
        infolay = pygame.Surface((FRAME_W, FRAME_H), pygame.SRCALPHA)
        dislay = pygame.Surface((FRAME_W, FRAME_H), pygame.SRCALPHA)
        clock = pygame.time.Clock()
        font = pygame.font.SysFont("monospace", 14)

        # Load initial frame
        # Monochrome
        size = FRAME_W * FRAME_H
        packed = f.read(size // 8)
        frame = unpack_monochrome(packed, size).reshape(FRAME_H, FRAME_W)
#//        surf = pygame.surfarray.make_surface(np.stack([frame]*3, axis=-1))
        surf = pygame.surfarray.make_surface(np.stack([frame]*3, axis=-1).swapaxes(0,1))
        dislay.blit(surf, (0, 0))

        running = True
        frame_idx = 0
        start_time = time.time()
        mbitrate = 0
        mboxes = 0
        sumbytes = 0
        sumboxes = 0
        total_bytes = f.tell()

        while running:
            infolay.fill((0, 0, 0, 0))
            screen.fill((0, 0, 0))
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False

            # Read number of tiles
            num_bytes = f.read(1)
            if not num_bytes:
                break
            num_tiles = struct.unpack("B", num_bytes)[0]

            updates = []
            for _ in range(num_tiles):
                tx = struct.unpack("B", f.read(1))[0]
                ty = struct.unpack("B", f.read(1))[0]
                packed = f.read((TILE_W*TILE_H)//8)
                tile = unpack_monochrome(packed, TILE_W*TILE_H).reshape(TILE_H, TILE_W)
                tile_rgb = np.stack([tile]*3, axis=-1)
                updates.append((tx, ty, tile_rgb))

            # Apply updates
            tcnt = 0
            for tx,ty,tile_rgb in updates:
                surf_tile = pygame.surfarray.make_surface(tile_rgb.swapaxes(0,1))
                dislay.blit(surf_tile, (tx*TILE_W, ty*TILE_H))
                tcnt += 1
                if debug:
                    pygame.draw.rect(infolay, (255,0,0), (tx*TILE_W, ty*TILE_H, TILE_W, TILE_H), 1)

            if debug:
                elapsed = time.time() - start_time
                fps_actual = (frame_idx+1) / elapsed if elapsed > 0 else 0
                bitrate = (f.tell() - total_bytes) * 8 / elapsed / 1000  # kbps
                pout = False
                if mbitrate < bitrate:
                    mbitrate = bitrate
                    pout = True
                if mboxes < tcnt:
                    mboxes = tcnt
                    pout = True
                print(f"max bitrate up to: {round(mbitrate)}kbps with {mboxes} tile(s)      ", end = "\r")
                text = f"FPS: {fps_actual:.1f}/{fps}, Bitrate: {bitrate:.1f} kbps"
                overlay = font.render(text, True, (255, 0, 0))
                infolay.blit(overlay, (5,5))
                text = f"Boxes: {tcnt}"
                overlay = font.render(text, True, (255, 0, 0))
                infolay.blit(overlay, (5,20))
                sumboxes += tcnt
                sumbytes += round(bitrate)
            screen.blit(dislay, (0, 0))
            screen.blit(infolay, (0, 0))
            pygame.display.flip()
            clock.tick(fps)
            frame_idx += 1

        pygame.quit()
        if debug:
            print("\n\n\n")
            print("====== RESULT ======")
            print(f"{frame_idx + 1} frames passed")
            print(f"{sumboxes} boxes drawed, average {sumboxes // (frame_idx + 1)}, max {mboxes}")
            print(f"max bitrate {mbitrate}, average {sumbytes // frame_idx}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("file", help="Input .vis file")
    parser.add_argument("--debug", action="store_true", help="Show tile outlines and bitrate/FPS")
    args = parser.parse_args()

    vis_player(args.file, args.debug)
