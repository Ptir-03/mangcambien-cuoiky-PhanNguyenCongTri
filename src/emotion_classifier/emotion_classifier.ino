/**
 * ================================================================
 * NHẬN DIỆN CẢM XÚC – ESP32-S3 OV3660 + Edge Impulse (GRAYSCALE)
 * ================================================================
 */

#include <Face_Emotion_Recognition_inferencing.h>
#include "esp_camera.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================================================================
// PIN
// ================================================================
#define BTN_PIN         0

// ================================================================
// CAMERA PIN (OV3660 - ESP32-S3)
// ================================================================
#define CAM_PWDN        -1
#define CAM_RESET       -1
#define CAM_XCLK        15
#define CAM_SIOD         4
#define CAM_SIOC         5

#define CAM_Y9          16
#define CAM_Y8          17
#define CAM_Y7          18
#define CAM_Y6          12
#define CAM_Y5          10
#define CAM_Y4           8
#define CAM_Y3           9
#define CAM_Y2          11

#define CAM_VSYNC        6
#define CAM_HREF         7
#define CAM_PCLK        13

// ================================================================
// LCD I2C
// ================================================================
#define LCD_SDA         41
#define LCD_SCL         42
#define LCD_COLS        20
#define LCD_ROWS         4

// ================================================================
// KÍCH THƯỚC ẢNH & CENTER CROP
// ================================================================
#define SRC_W   160
#define SRC_H   120
#define DST_W   EI_CLASSIFIER_INPUT_WIDTH   // Tự động nhận 96 từ Edge Impulse
#define DST_H   EI_CLASSIFIER_INPUT_HEIGHT  // Tự động nhận 96 từ Edge Impulse
#define CROP_X  ((SRC_W - DST_W) / 2)       // Cắt vào giữa
#define CROP_Y  ((SRC_H - DST_H) / 2)       // Cắt vào giữa

// ================================================================
// BIẾN TOÀN CỤC
// ================================================================
LiquidCrystal_I2C *lcd = nullptr;
static camera_fb_t *fb_ptr = nullptr;
uint32_t lastBtnMs = 0;
#define DEBOUNCE_MS 250

// ================================================================
// SCAN I2C
// ================================================================
uint8_t scan_i2c_addr() {
    uint8_t found = 0x27;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("[I2C] Tim thay: 0x%02X\n", addr);
            found = addr;
            if (addr == 0x27 || addr == 0x3F) break;
        }
    }
    return found;
}

// ================================================================
// KHỞI TẠO CAMERA (CHUYỂN SANG ẢNH XÁM - GRAYSCALE)
// ================================================================
bool init_camera() {
    camera_config_t cfg;
    cfg.ledc_channel = LEDC_CHANNEL_0;
    cfg.ledc_timer   = LEDC_TIMER_0;
    cfg.pin_d0 = CAM_Y2; cfg.pin_d1 = CAM_Y3;
    cfg.pin_d2 = CAM_Y4; cfg.pin_d3 = CAM_Y5;
    cfg.pin_d4 = CAM_Y6; cfg.pin_d5 = CAM_Y7;
    cfg.pin_d6 = CAM_Y8; cfg.pin_d7 = CAM_Y9;
    cfg.pin_xclk  = CAM_XCLK; cfg.pin_pclk  = CAM_PCLK;
    cfg.pin_vsync = CAM_VSYNC; cfg.pin_href  = CAM_HREF;
    cfg.pin_sscb_sda = CAM_SIOD; cfg.pin_sscb_scl = CAM_SIOC;
    cfg.pin_pwdn  = CAM_PWDN;  cfg.pin_reset = CAM_RESET;
    cfg.xclk_freq_hz = 10000000;
    
    // Đã đổi sang ảnh xám để khớp với Edge Impulse
    cfg.pixel_format = PIXFORMAT_GRAYSCALE; 
    
    cfg.frame_size   = FRAMESIZE_QQVGA; // 160x120
    cfg.jpeg_quality = 10;
    cfg.fb_count     = 1;
    cfg.grab_mode    = CAMERA_GRAB_LATEST;
    cfg.fb_location  = CAMERA_FB_IN_PSRAM;

    esp_err_t err = esp_camera_init(&cfg);
    if (err != ESP_OK) {
        Serial.printf("[LOI] Camera init that bai: 0x%x\n", err);
        return false;
    }

    sensor_t *s = esp_camera_sensor_get();
    if (s) {
        s->set_brightness(s, 1);
        s->set_contrast(s, 1);
        s->set_saturation(s, -1);
        s->set_whitebal(s, 1);
        s->set_gain_ctrl(s, 1);
        s->set_exposure_ctrl(s, 1);
    }
    Serial.println("[OK] Camera san sang.");
    return true;
}

// ================================================================
// GRAYSCALE → FLOAT DÀNH CHO EDGE IMPULSE
// ================================================================
static int camera_get_data(size_t offset, size_t length, float *out_ptr) {
    const uint8_t *buf = fb_ptr->buf;
    for (size_t i = 0; i < length; i++) {
        size_t dst_x = (offset + i) % DST_W;
        size_t dst_y = (offset + i) / DST_W;
        
        // Tọa độ trên ảnh gốc (đã cộng offset để lấy chính giữa 96x96)
        size_t src_x = dst_x + CROP_X;
        size_t src_y = dst_y + CROP_Y;
        
        // Ảnh xám chỉ tốn 1 byte cho mỗi pixel
        size_t idx   = (src_y * SRC_W + src_x);
        uint8_t pixel = buf[idx];

        // Edge Impulse yêu cầu float dạng 0xRRGGBB.
        // Vì là ảnh xám nên R = G = B = giá trị pixel.
        out_ptr[i] = (float)((pixel << 16) | (pixel << 8) | pixel);
    }
    return 0;
}

// ================================================================
// HIỂN THỊ KẾT QUẢ TRÊN LCD
// ================================================================
void display_result(const char *emotion, float confidence) {
    lcd->clear();
    lcd->setCursor(0, 0); lcd->print("=== KET QUA ===");
    lcd->setCursor(0, 1); lcd->print("Cam xuc: "); lcd->print(emotion);
    lcd->setCursor(0, 2); lcd->print("Tin cay: ");
    lcd->print((int)(confidence * 100)); lcd->print("%");
    lcd->setCursor(0, 3); lcd->print("Nhan nut de chup");

    Serial.printf("[KQ] Cam xuc: '%s' | Tin cay: %.1f%%\n", emotion, confidence * 100.0f);
}

// ================================================================
// CHỤP + PHÂN LOẠI CẢM XÚC
// ================================================================
void capture_and_classify() {
    lcd->clear();
    lcd->setCursor(3, 1); lcd->print("Dang chup...");
    lcd->setCursor(1, 2); lcd->print("Vui long doi...");
    Serial.println("[INFO] Bat dau chup anh...");

    fb_ptr = esp_camera_fb_get();
    if (!fb_ptr) {
        Serial.println("[LOI] Khong lay duoc frame!");
        lcd->clear(); lcd->print("LOI CAMERA!");
        return;
    }

    Serial.printf("[DEBUG] Frame: %d x %d\n", fb_ptr->width, fb_ptr->height);

    if (fb_ptr->width < DST_W || fb_ptr->height < DST_H) {
        Serial.println("[LOI] Frame nho hon model!");
        esp_camera_fb_return(fb_ptr);
        fb_ptr = nullptr;
        return;
    }

    // Nạp dữ liệu vào thư viện AI
    ei::signal_t signal;
    signal.total_length = DST_W * DST_H;
    signal.get_data     = &camera_get_data;

    ei_impulse_result_t result = {0};
    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);

    if (err != EI_IMPULSE_OK) {
        Serial.printf("[LOI] run_classifier: %d\n", err);
        esp_camera_fb_return(fb_ptr);
        fb_ptr = nullptr;
        return;
    }

    Serial.println("[INFO] Ket qua phan loai:");
    const char *best_label = result.classification[0].label;
    float best_score       = result.classification[0].value;

    // Quét qua các classes (Angry, Happy, Sad, Surprise)
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        Serial.printf("  [%d] '%-12s' : %.1f%%\n",
                      i,
                      result.classification[i].label,
                      result.classification[i].value * 100);

        if (result.classification[i].value > best_score) {
            best_score = result.classification[i].value;
            best_label = result.classification[i].label;
        }
    }

    display_result(best_label, best_score);

    esp_camera_fb_return(fb_ptr);
    fb_ptr = nullptr;
    Serial.println("[OK] Xong.\n");
}

// ================================================================
// SETUP
// ================================================================
void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\n================================");
    Serial.println(" EMOTION CAM - GRAYSCALE");
    Serial.println("================================");
    Serial.printf("[INFO] PSRAM: %u bytes\n", ESP.getPsramSize());
    Serial.printf("[INFO] Heap:  %u bytes\n", ESP.getFreeHeap());

    pinMode(BTN_PIN, INPUT_PULLUP);

    Wire.begin(LCD_SDA, LCD_SCL);
    uint8_t addr = scan_i2c_addr();
    Serial.printf("[LCD] Dia chi: 0x%02X\n", addr);
    lcd = new LiquidCrystal_I2C(addr, LCD_COLS, LCD_ROWS);
    lcd->init(); lcd->backlight(); lcd->clear();
    lcd->setCursor(0, 0); lcd->print("  EMOTION CAM   ");
    lcd->setCursor(0, 2); lcd->print("Dang khoi dong..");

    if (!init_camera()) {
        lcd->clear();
        lcd->setCursor(0, 1); lcd->print("LOI CAMERA!");
        while (true) { delay(1000); }
    }

    lcd->clear();
    lcd->setCursor(0, 0); lcd->print("=== SAN SANG ===");
    lcd->setCursor(0, 1); lcd->print("Nhan nut GPIO0");
    lcd->setCursor(0, 2); lcd->print("de chup & AI");
    Serial.println("[OK] San sang! Nhan nut de bat dau.");
}

// ================================================================
// LOOP
// ================================================================
void loop() {
    if (digitalRead(BTN_PIN) == LOW) {
        uint32_t now = millis();
        if (now - lastBtnMs > DEBOUNCE_MS) {
            lastBtnMs = now;
            capture_and_classify();
        }
    }
    delay(10);
}