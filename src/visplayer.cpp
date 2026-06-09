// visplayer.cpp
#include "visplayer.h"

#define VIS_HEADER_SIZE 7
#define INITIAL_FRAME_SIZE (TILES_X * TILES_Y * TILE_BYTES)

#define TILE_W 8
#define TILE_H 8
#define TILES_X 16
#define TILES_Y 8
#define TILE_BYTES 8  // 8x8 / 8 bits

static File visFile;
static uint8_t visFps = 24;
static bool playing = false;
static unsigned long lastFrameTime = 0;
static bool isFirstFrame = true;

// buffer tile hiện tại của màn hình
static uint8_t screenBuf[TILES_Y][TILES_X][TILE_BYTES];

static void drawTile(uint8_t tx, uint8_t ty, uint8_t* data) {
  int ox = tx * TILE_W;
  int oy = ty * TILE_H;
  for (int row = 0; row < TILE_H; row++) {
    uint8_t byte = data[row];
    for (int col = 0; col < TILE_W; col++) {
      if (byte & (0x80 >> col))
        u8g2.drawPixel(ox + col, oy + row);
    }
  }
}

bool visLoad(const char* path) {
  if (visFile) visFile.close();
  visFile = LittleFS.open(path, "r");
  if (!visFile) return false;

  uint8_t magic[2];
  visFile.read(magic, 2);
  if (magic[0] != 'V' || magic[1] != 'S') { visFile.close(); return false; }

  uint8_t hdr[5];
  visFile.read(hdr, 5);
  visFps = hdr[0];
  // hdr[1..4] bỏ qua, dùng constant

  // đọc initial frame vào screenBuf
  for (int ty = 0; ty < TILES_Y; ty++)
    for (int tx = 0; tx < TILES_X; tx++)
      visFile.read(screenBuf[ty][tx], TILE_BYTES);

  // visFile position bây giờ đúng ở delta frame đầu tiên
  isFirstFrame = true;
  playing = true;
  return true;
}

void visPlay() {
  if (!playing || !visFile) return;

  unsigned long now = millis();
  unsigned long interval = 1000 / visFps;
  if (now - lastFrameTime < interval) return;
  lastFrameTime = now;

  u8g2.clearBuffer();

  if (isFirstFrame) {
    // render toàn bộ initial frame
    for (int ty = 0; ty < TILES_Y; ty++)
      for (int tx = 0; tx < TILES_X; tx++)
        drawTile(tx, ty, screenBuf[ty][tx]);
    isFirstFrame = false;
  } else {
    // đọc delta frame
    uint8_t tileCount = 0;
    if (visFile.read(&tileCount, 1) != 1) {
      // hết file → loop lại
      visFile.seek(VIS_HEADER_SIZE + INITIAL_FRAME_SIZE);
      isFirstFrame = false;
      return;
    }

    for (int i = 0; i < tileCount; i++) {
      uint8_t tx, ty;
      visFile.read(&tx, 1);
      visFile.read(&ty, 1);
      visFile.read(screenBuf[ty][tx], TILE_BYTES);
    }

    // render toàn bộ buffer
    for (int ty = 0; ty < TILES_Y; ty++)
      for (int tx = 0; tx < TILES_X; tx++)
        drawTile(tx, ty, screenBuf[ty][tx]);
  }

  u8g2.sendBuffer();
}

void visStop() {
  playing = false;
  if (visFile) visFile.close();
}

bool visIsPlaying() { return playing; }