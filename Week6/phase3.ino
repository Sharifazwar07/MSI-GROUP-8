/*
 * Phase 3 (Smooth Version): Manual Motion Trigger on GPIO 13
 * Servo movement runs independently in loop() for smooth, lag-free panning.
 */

#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"

// WiFi network credentials
const char* ssid = "Sarepppp";
const char* password = "hello124";

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;
int stream_port_number; // Stream server port number

const int servo_pin = 2;   // Servo motor connected to GPIO 2
const int button_pin = 13; // Pushbutton connected to GPIO 13

// PWM values for servo's min and max positions
const int servo_min_position = 3700;
const int servo_max_position = 7000;
const int servo_speed = 40; // Step size per 20ms update

int servo_position = servo_min_position;
bool servo_direction = true; // true = forward, false = backward

// stream_handler: handles live video streaming only
static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

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
      if (fb->width > 400) {
        if (fb->format != PIXFORMAT_JPEG) {
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if (!jpeg_converted) {
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        } else {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
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

  char webPage[200] = "";
  strcat(webPage, "<!doctype html><html><head><title id='title'>ESP32-CAM</title></head><body><img id='stream' src='http://");

  char address[30] = "";
  sprintf(address, "%d.%d.%d.%d:%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3], stream_port_number);
  strcat(webPage, address);
  strcat(webPage, "/stream'></body></html>");

  int pageLength = strlen(webPage);
  return httpd_resp_send(req, (const char *)webPage, pageLength);
}

// startCameraServer: starts the web and stream servers
void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

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

// setup: camera, button, servo, and WiFi initialization
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("====");

  // Enable internal pull-up resistor for the pushbutton on GPIO 13
  pinMode(button_pin, INPUT_PULLUP);

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
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_VGA;
  config.jpeg_quality = 10;
  config.fb_count     = 2;

  // Camera initialization
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Servo motor initialization on Channel 2 (50Hz, 16-bit resolution)
  ledcSetup(2, 50, 16);
  ledcAttachPin(servo_pin, 2);
  ledcWrite(2, servo_position);

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

// loop: handles smooth servo panning when pushbutton is pressed
void loop() {
  if (digitalRead(button_pin) == LOW) {
    ledcWrite(2, servo_position);
    if (servo_direction) {
      servo_position += servo_speed;
      if (servo_position > servo_max_position) {
        servo_direction = false;
      }
    } else {
      servo_position -= servo_speed;
      if (servo_position < servo_min_position) {
        servo_direction = true;
      }
    }
  }
  delay(20); // Updates every 20ms (50Hz) for smooth motion
}
