// Requires M5Unified and M5GFX. Keep the device still when comparing views.
// Reference: docs/api/display.md#orientation-and-dimensions
#include <M5Unified.h>

static uint8_t rotation = 0;
static bool ready = false;

static void drawView(uint8_t value)
{
  auto& display = M5.Display;
  display.waitDisplay();
  display.setRotation(value);
  rotation = display.getRotation();

  const int w = display.width();
  const int h = display.height();
  const int shortest = w < h ? w : h;
  const int u = shortest / 9 > 0 ? shortest / 9 : 1;
  const int x = (w - 3 * u) / 2;
  const int y = (h - 5 * u) / 2;
  const int marker = u / 2 > 1 ? u / 2 : 2;
  const int a = marker + 3;
  const int length = 2 * u;

  display.startWrite();
  display.fillScreen(TFT_WHITE);

  // Match the diagram's logical corner colors. Corners may be outside a round screen.
  display.fillRect(0, 0, marker, marker, display.color888(245, 158, 11));
  display.fillRect(w - marker, 0, marker, marker, display.color888(34, 197, 94));
  display.fillRect(0, h - marker, marker, marker, display.color888(56, 189, 248));
  display.fillRect(w - marker, h - marker, marker, marker, display.color888(244, 114, 182));

  // The same asymmetric F used in the eight-view diagram.
  display.fillRect(x, y, u, 5 * u, TFT_BLACK);
  display.fillRect(x, y, 3 * u, u, TFT_BLACK);
  display.fillRect(x, y + 2 * u, 2 * u, u, TFT_BLACK);

  display.drawLine(a, a, a + length, a, TFT_BLACK);
  display.drawLine(a + length, a, a + length - 2, a - 2, TFT_BLACK);
  display.drawLine(a + length, a, a + length - 2, a + 2, TFT_BLACK);
  display.drawLine(a, a, a, a + length, TFT_BLACK);
  display.drawLine(a, a + length, a - 2, a + length - 2, TFT_BLACK);
  display.drawLine(a, a + length, a + 2, a + length - 2, TFT_BLACK);
  display.setFont(&fonts::Font0);
  display.setTextSize(1);
  display.setTextColor(TFT_BLACK, TFT_WHITE);
  display.setTextWrap(false);
  display.setCursor(a + length + 3, a - 2);
  display.print("X");
  display.setCursor(a - 2, a + length + 3);
  display.print("Y");

  // Keep the value near the center line so it remains visible on circular screens.
  char label[32];
  snprintf(label, sizeof(label), "r=%u", static_cast<unsigned>(rotation));
  display.drawString(label, (w - display.textWidth(label)) / 2, y + 5 * u + 2);
  if (h >= 80)
  {
    snprintf(label, sizeof(label), "%dx%d", w, h);
    display.drawString(label, (w - display.textWidth(label)) / 2, y + 5 * u + 12);
  }
  display.endWrite();
  display.display();

  Serial.printf("requested=%u rotation=%u width=%d height=%d\n",
                static_cast<unsigned>(value), static_cast<unsigned>(rotation), w, h);
}

void setup()
{
  M5.begin();
  Serial.begin(115200);
  Serial.printf("Startup after M5.begin(): rotation=%u width=%d height=%d\n",
                static_cast<unsigned>(M5.Display.getRotation()),
                static_cast<int>(M5.Display.width()), static_cast<int>(M5.Display.height()));
  if (M5.Display.width() <= 0 || M5.Display.height() <= 0)
  {
    Serial.println("No display available.");
    return;
  }
  Serial.println("Button A: next rotation (0..7). Serial: n for next, or 0..7 to select.");
  ready = true;
  drawView(0);
}

void loop()
{
  M5.update();
  if (ready)
  {
    int next = -1;
    if (M5.BtnA.wasPressed()) next = (rotation + 1) & 7;
    if (Serial.available())
    {
      const int key = Serial.read();
      if (key >= '0' && key <= '7') next = key - '0';
      else if (key == 'n' || key == 'N') next = (rotation + 1) & 7;
    }
    if (next >= 0) drawView(static_cast<uint8_t>(next));
  }
  delay(1);
}
