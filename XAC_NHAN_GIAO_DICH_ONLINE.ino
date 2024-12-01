#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include "time.h"

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7;
const int   daylightOffset_sec = 3600;

// Thông tin mạng Wi-Fi
const char* ssid = "Inception Escape Room";
const char* password = "IloveInception";

// Đặt API key và URL
#define API_KEY "AK_CS.5c7e21d05d5811efa5dbff93fab61642.nkkEfQj9AkqJCYiO5gkvrMW5hxkbphzDTFciOqeSOUZBTKcgHFZYkjrXkxqQH17ySqe4DnVT"
const char* url = "https://oauth.casso.vn/v2/transactions";

// Biến để lưu trữ ID giao dịch đầu tiên
String last_first_id;
String new_first_id;

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Không thể lấy thời gian");
    return;
  }

  int day = timeinfo.tm_mday;
  int month = timeinfo.tm_mon + 1; // Tháng từ 0-11
  int year = timeinfo.tm_year + 1900; // Năm từ 1900
  
  int hour = timeinfo.tm_hour;
  int minute = timeinfo.tm_min;
  
  //Serial.printf("Ngày: %04d-%02d-%02d\n", year, month, day);
  //Serial.printf("Giờ: %02d:%02d\n", hour, minute);
}

void setup() {
  // Khởi động kết nối Serial
  Serial.begin(115200);
  delay(10);

  // Kết nối Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to Wi-Fi");

  // Cấu hình thời gian
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
}

void loop() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Không thể lấy thời gian");
    delay(5000);
    return;
  }

  // Lấy ngày tháng năm và giờ phút
  int day = timeinfo.tm_mday;
  int month = timeinfo.tm_mon + 1;
  int year = timeinfo.tm_year + 1900;
  
  // Định dạng ngày và tháng với số 0 dẫn đầu nếu cần
  char dateStr[11];
  snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d", year, month, day);
  
  // Định dạng giờ và phút với số 0 dẫn đầu nếu cần
  char timeStr[6];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Tạo đối tượng WiFiClientSecure cho HTTPS
    WiFiClientSecure client;

    // Đặt URL yêu cầu
    String requestUrl = String(url) + "?fromDate=2021-04-01&toDate=" + String(dateStr) + "&page=1&pageSize=1&sort=DESC";
    //Serial.println("Request URL: " + requestUrl);  // In URL để kiểm tra

    // Kết nối đến URL với HTTPS
    http.begin(client, requestUrl);

    // Thêm header cho yêu cầu
    http.addHeader("Authorization", "Apikey " + String(API_KEY));

    int httpCode = http.GET();

    // Xử lý mã trạng thái HTTP
    if (httpCode == 200) {
      String payload = http.getString();
      //Serial.println("Nhận được dữ liệu từ API:");
      //Serial.println(payload);

      // Phân tích dữ liệu JSON
      StaticJsonDocument<2048> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        Serial.println("Lỗi phân tích JSON");
        return;
      }

      // Lấy thông tin giao dịch đầu tiên
      JsonObject data = doc["data"];
      JsonArray records = data["records"];
      if (records.size() > 0) {
        JsonObject first_record = records[0];
        new_first_id = first_record["id"].as<String>();
        int new_amount = first_record["amount"];
        String description = first_record["description"].as<String>();
        /*Serial.println("ID giao dịch: " + new_first_id);
        Serial.println("Số tiền: " + String(new_amount));
        Serial.println("Mô tả: " + description);*/

        // Kiểm tra xem 'id1' có xuất hiện trong Description không
        if (description.indexOf("id1") != -1) {
          if (new_first_id != last_first_id) {
            // Nếu ID khác và số tiền dương
            if (new_amount > 0) {
              Serial.printf("Máy 1 đã nhận thanh toán mới với số tiền giao dịch = %d\n", new_amount);
              Serial.println("Thời gian giao dịch: " + String(dateStr) + " " + String(timeStr));
            }
          }
        }
        if (description.indexOf("id2") != -1) {
          if (new_first_id != last_first_id) {
            // Nếu ID khác và số tiền dương
            if (new_amount > 0) {
              Serial.printf("Máy 2 đã nhận thanh toán mới với số tiền giao dịch = %d\n", new_amount);
              Serial.println("Thời gian giao dịch: " + String(dateStr) + " " + String(timeStr));
            }
          }
        }
      } else {
        Serial.println("Không có giao dịch nào để kiểm tra.");
      }
    } else {
      Serial.printf("Lỗi: %d - %s\n", httpCode, http.errorToString(httpCode).c_str());
    }

    http.end();
    
    // Cập nhật ID đầu tiên
    last_first_id = new_first_id;
  } else {
    Serial.println("Không kết nối được Wi-Fi.");
  }

  // Đợi 60 giây trước khi thực hiện yêu cầu tiếp theo
  delay(5000);
}
