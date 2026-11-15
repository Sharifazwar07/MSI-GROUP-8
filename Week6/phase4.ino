/*
 * Phase 4 (Ultimate Fix): Native RGB565 Face Detection + Visual Box + Smooth Servo
 * - Captures in PIXFORMAT_RGB565 for direct, reliable AI face detection
 * - Draws a green box around the detected face on the browser stream
 * - Smoothly pans the servo motor when a face is detected or button is pressed
 */

#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"
#include "img_converters.h"
#include "human_face_detect_msr01.hpp"
#include <list>

// WiFi network credentials
const char* ssid = "Sarepppp";
const char* password = "hello124";

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;
int stream_port_number;

const int servo_pan_pin = 2;   // Horizontal servo on GPIO 2
const int servo_tilt_pin = 14; // Vertical servo on GPIO 14 (Bonus Task 2)
const int button_pin = 13;     // Manual trigger pushbutton on GPIO 13
const int red_led_pin = 33;    // Built-in red LED on back of ESP32-CAM (Active LOW)

// PWM values for servo's min and max positions
const int servo_min_position = 3700;
const int servo_max_position = 7000;
const int servo_speed = 45; // Step size per 20ms in loop()

int pan_position = servo_min_position;
bool pan_direction = true;

int tilt_position = 5000;
bool tilt_direction = true;

// Timestamp to trigger smooth servo motion in loop()
volatile unsigned long last_face_time = 0;
const unsigned long face_trigger_duration = 2500; // Keep moving for 2.5 seconds after detection

// Helper function to draw a green box around the detected face in RGB565 buffer
void draw_face_box(uint16_t *buf, int width, int height, int x1, int y1, int x2, int y2) {
  x1 = max(0, min(width - 1, x1));
  y1 = max(0, min(height - 1, y1));
  x2 = max(0, min(width - 1, x2));
  y2 = max(0, min(height - 1, y2));
  uint16_t green = 0xE007; // Bright green in big-endian RGB565

  for (int x = x1; x <= x2; x++) {
    buf[y1 * width + x] = green;
    buf[y2 * width + x] = green;
    if (y1 + 1 < height) buf[(y1 + 1) * width + x] = green;
    if (y2 - 1 >= 0)     buf[(y2 - 1) * width + x] = green;
  }
  for (int y = y1; y <= y2; y++) {
    buf[y * width + x1] = green;
    buf[y * width + x2] = green;
    if (x1 + 1 < width)  buf[y * width + (x1 + 1)] = green;
    if (x2 - 1 >= 0)     buf[y * width + (x2 - 1)] = green;
  }
}

// stream_handler: handles RGB565 capture, face detection, JPEG conversion, and streaming
static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  // Initialize Face Detection model (0.3F scale for better distance sensitivity)
  HumanFaceDetectMSR01 s1(0.1F, 0.5F, 10, 0.3F);

  res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
  if (res != ESP_OK) {
    return res;
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    } else {
      // Run face detection directly on the native RGB565 buffer
      std::list<dl::detect::result_t> &results = s1.infer((uint16_t *)fb->buf, {(int)fb->height, (int)fb->width, 3});
      if (results.size() > 0) {
        last_face_time = millis();
        Serial.println(">>> FACE DETECTED! Moving Servo... <<<");

        // Draw green bounding box around detected face(s)
        for (std::list<dl::detect::result_t>::iterator it = results.begin(); it != results.end(); ++it) {
          draw_face_box((uint16_t *)fb->buf, fb->width, fb->height, it->box[0], it->box[1], it->box[2], it->box[3]);
        }
      }

      // Convert RGB565 frame buffer to JPEG for browser streaming
      bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
      esp_camera_fb_return(fb);
      fb = NULL;
      if (!jpeg_converted) {
        Serial.println("JPEG compression failed");
        res = ESP_FAIL;
      }
    }

    if (res == ESP_OK) {
      size_t hlen = snprintf((char *)part_buf, 64, STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
    }

    if (fb) {
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if (_jpg_buf) {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }

    if (res != ESP_OK) {
      break;
    }
  }
  return res;
}

// web_handler: builds the web page
static esp_err_t web_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Content-Encoding", "identity");

  char webPage[220] = "";
  strcat(webPage, "<!doctype html><html><head><title id='title'>ESP32-CAM Phase 4</title></head><body><h2>ESP32-CAM Face Detection</h2><img id='stream' src='http://");

  char address[30] = "";
  sprintf(address, "%d.%d.%d.%d:%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3], stream_port_number);
  strcat(webPage, address);
  strcat(webPage, "/stream' style='width:320px;height:240px;'></body></html>");

  int pageLength = strlen(webPage);
  return httpd_resp_send(req, (const char *)webPage, pageLength);
}

// startCameraServer: starts the web and stream servers
void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.stack_size = 32768; // Required stack size for AI model

  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = web_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };

  Serial.printf("Starting web server on port: '%d'\n", config.server_port);
  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
  }

  config.server_port += 1;
  config.ctrl_port += 1;
  Serial.printf("Starting stream server on port: '%d'\n", config.server_port);
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
    stream_port_number = config.server_port;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("====");

  pinMode(button_pin, INPUT_PULLUP);
  pinMode(red_led_pin, OUTPUT);
  digitalWrite(red_led_pin, HIGH); // Turn OFF red LED initially

  // Pin configuration for AI Thinker ESP32-CAM
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = 5;
  config.pin_d1       = 18;
  config.pin_d2       = 19;
  config.pin_d3       = 21;
  config.pin_d4       = 36;
  config.pin_d5       = 39;
  config.pin_d6       = 34;
  config.pin_d7       = 35;
  config.pin_xclk     = 0;
  config.pin_pclk     = 22;
  config.pin_vsync    = 25;
  config.pin_href     = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn     = 32;
  config.pin_reset    = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_RGB565; // Native RGB565 required for reliable Face Detection
  config.frame_size   = FRAMESIZE_QVGA;   // 320x240 resolution
  config.jpeg_quality = 12;
  config.fb_count     = 1;

  // Camera initialization
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Initialize Horizontal Servo on GPIO 2 (Channel 2)
  ledcSetup(2, 50, 16);
  ledcAttachPin(servo_pan_pin, 2);
  ledcWrite(2, pan_position);

  // Initialize Vertical Servo on GPIO 14 (Channel 3)
  ledcSetup(3, 50, 16);
  ledcAttachPin(servo_tilt_pin, 3);
  ledcWrite(3, tilt_position);

  Serial.print("Connecting to WiFi network: ");
  Serial.println(ssid);
  delay(100);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }

  Serial.println("WiFi connected");
  delay(100);
  startCameraServer();

  Serial.print("Camera ready. Access it at: http://");
  Serial.println(WiFi.localIP());
}

// loop: moves servos smoothly when a face is detected OR pushbutton is pressed
void loop() {
  bool face_active = (last_face_time > 0) && (millis() - last_face_time < face_trigger_duration);
  bool button_active = (digitalRead(button_pin) == LOW);

  if (face_active || button_active) {
    digitalWrite(red_led_pin, LOW); // Turn ON red indicator LED

    // 1. Horizontal Servo Panning (GPIO 2)
    ledcWrite(2, pan_position);
    if (pan_direction) {
      pan_position += servo_speed;
      if (pan_position > servo_max_position) {
        pan_direction = false;
      }
    } else {
      pan_position -= servo_speed;
      if (pan_position < servo_min_position) {
        pan_direction = true;
      }
    }

    // 2. Vertical Servo Panning (GPIO 14)
    ledcWrite(3, tilt_position);
    if (tilt_direction) {
      tilt_position += servo_speed;
      if (tilt_position > servo_max_position) {
        tilt_direction = false;
      }
    } else {
      tilt_position -= servo_speed;
      if (tilt_position < servo_min_position) {
        tilt_direction = true;
      }
    }
  } else {
    digitalWrite(red_led_pin, HIGH); // Turn OFF red indicator LED
  }

  delay(20); // Smooth 50Hz update rate
}
