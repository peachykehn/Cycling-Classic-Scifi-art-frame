#include "SD_MMC.h"
#include "EPD_13in3e.h"     // low-level panel driver, from Waveshare's example
#include "GUI_Paint.h"      // Paint_* drawing functions
#include "GUI_BMPfile.h"    // GUI_ReadBmp()

#define SD_CLK  6
#define SD_CMD  7
#define SD_D0   5
#define SD_D1   4
#define SD_D2   16
#define SD_D3   15

const char* IMAGES_DIR = "/bmp";
const uint32_t SLEEP_MINUTES = 2; //change to 5 or 10 after dont testing

RTC_DATA_ATTR int lastIndex = -1;
UBYTE *Image;

String pickRandomFile(int &countOut) {
  File dir = SD_MMC.open(IMAGES_DIR);
  if (!dir) {
    Serial.println("cant find the sd card.");
    countOut = 0;
    return "";
  }

  int count = 0;
  File f = dir.openNextFile();
  while (f) {
    if (!f.isDirectory()) count++;
    f.close();
    f = dir.openNextFile();
  }
  dir.rewindDirectory();

  if (count == 0) {
    countOut = 0;
    return "";
  }

  int choice = random(count);
  if (count > 1 && choice == lastIndex) {
    choice = (choice + 1) % count;
  }
  lastIndex = choice;
  countOut = count;

  int i = 0;
  String result = "";
  f = dir.openNextFile();
  while (f) {
    if (!f.isDirectory()) {
      if (i == choice) {
        result = String(IMAGES_DIR) + "/" + f.name();
        f.close();
        break;
      }
      i++;
    }
    f.close();
    f = dir.openNextFile();
  }
  return result;
}

void goToSleep() {
  Serial.printf("Sleeping for %u minutes...\n", SLEEP_MINUTES);
  Serial.flush();
  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_MINUTES * 60ULL * 1000000ULL);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  randomSeed(esp_random());

  SD_MMC.setPins(SD_CLK, SD_CMD, SD_D0, SD_D1, SD_D2, SD_D3);
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("sd card failed to mount — will retry next cycle.");
    goToSleep();
    return;
  }

  int fileCount = 0;
  String path = pickRandomFile(fileCount);
  if (path == "") {
    Serial.println("No images found in /bmp.");
    goToSleep();
    return;
  }
  String fullPath = "/sdcard" + path;
  Serial.printf("Showing: %s (%d images available)\n", fullPath.c_str(), fileCount);

  Image = (UBYTE *)ps_malloc((EPD_13IN3E_WIDTH * EPD_13IN3E_HEIGHT) / 2);
  if (!Image) {
    Serial.println("Not enough memory for the buff.");
    goToSleep();
    return;
  }

  DEV_Module_Init();
  EPD_13IN3E_Init();
  Paint_NewImage(Image, EPD_13IN3E_WIDTH, EPD_13IN3E_HEIGHT, 0, EPD_13IN3E_WHITE);
  Paint_SetScale(6);
  Paint_SelectImage(Image);
  Paint_Clear(EPD_13IN3E_WHITE);

  GUI_ReadBmp_RGB_6Color(fullPath.c_str(), 0, 0);
  EPD_13IN3E_Display(Image);
  EPD_13IN3E_Sleep();
  DEV_Module_Exit();   // drop EPD_PWR_PIN so the panel's boost supply isn't left powered through deep sleep

  free(Image);
  goToSleep();
}

void loop() {

}
