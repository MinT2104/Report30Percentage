#include "DHT.h" // Thêm thư viện DHT.
#include <Adafruit_GFX.h> // Thêm thư viện Adafruit_GFX.
#include <Adafruit_SSD1306.h>
#include <freertos/timers.h>
// #include <Buzzer.h>
#include "pitches.h"

//------------------------------------------------------Định nghĩa chân cắm-------------------------------------------------------------//
#define DHTTYPE DHT22 //Định nghĩa loại cảm biến DHT là DHT22.
#define DHTPIN 18 // Định nghĩa chân kết nối cảm biến DHT là chân số 18.
#define btn_se 19 // Định nghĩa chân nút nhấn 'btn_se' là chân số 19.
#define led1 26 // Định nghĩa chân đèn LED 'led1' là chân số 26.
#define led2 25 // Định nghĩa chân đèn LED 'led2' là chân số 25.
#define button2 4 // Định nghĩa chân nút nhấn 'button2' là chân số 4.
#define button3 5 // Định nghĩa chân nút nhấn 'button3' là chân số 5.
#define BUZZER_PIN 32 // Định nghĩa chân đầu ra cho loa 'BUZZER_PIN' là chân số 32.


SemaphoreHandle_t countingSemaphore; //  Khai báo biến Semaphore 'countingSemaphore'.
SemaphoreHandle_t ledSemaphore; // Khai báo biến Semaphore 'ledSemaphore'.

DHT dht(DHTPIN, DHTTYPE); // Khởi tạo đối tượng cảm biến DHT với chân và loại đã định nghĩa.
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET); // Khởi tạo đối tượng hiển thị OLED với kích thước và chân reset đã định nghĩa.
// Buzzer buzzer(32); 

TimerHandle_t timerHandle; // Khai báo biến xử lý timer 'timerHandle'.

int BTN_STATE=1;
bool ledState = false;

struct DataSensor {
  int humid;
  int temp;
  //sử dụng để lưu trữ dữ liệu đo từ cảm biến nhiệt độ và độ ẩm
};

void Task1(void * parameter); // Khai báo hàm Task1.
void Task2(void * parameter); // Khai báo hàm Task2.
void Task3(void * parameter); // Khai báo hàm Task3.
void Task4(void * parameter); // Khai báo hàm Task4.
void readSensorTask(TimerHandle_t xTimer); // Khai báo hàm readSensorTask.
void printLCD(int humid, int temp); // Khai báo hàm printLCD.
void led_on_off(); // Khai báo hàm led_on_off.
void TaskBuzzer(void * parameter); // Khai báo hàm TaskBuzzer.
DataSensor readSensorData();


void setup() {
  Serial.begin(115200);
  //------------------------------------------------------Thiết lập chân I/O---------------------------------------------------------------//
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(btn_se, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP); 
  pinMode(button3, INPUT_PULLUP); 
  //-----------------------------------------------------Thiết lập màn hình OLED----------------------------------------------------------//
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Địa chỉ 0x3C cho 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.display(); // Khởi tạo với màn hình với bộ đệm hiển thị rõ ràng

  dht.begin(); // Khởi tạo dht

  countingSemaphore = xSemaphoreCreateCounting(5, 0);
  ledSemaphore = xSemaphoreCreateBinary();
    //------------------------------------------------------Softwware Timer-------------------------------------------------------------//
  timerHandle = xTimerCreate("SensorTimer", pdMS_TO_TICKS(1000), pdTRUE, (void *)0, readSensorTask);// dùng hàm timer, mỗi 1s gọi hàm 1 lần
    if (timerHandle != NULL) {
      xTimerStart(timerHandle, 0);
    } else {
      Serial.println("Failed to create timer");
    }
   //------------------------------------------------------Khởi tạo các Task-------------------------------------------------------------//
  xTaskCreatePinnedToCore(Task1, "Task1", 10000, NULL, 3, NULL, 0);
  xTaskCreatePinnedToCore(Task2, "Task2", 10000, NULL, 3, NULL, 0);
  xTaskCreatePinnedToCore(Task3, "Task3", 10000, NULL, 3, NULL, 0);
  xTaskCreatePinnedToCore(Task4, "Task4", 10000, NULL, 3, NULL, 1); 
  xTaskCreatePinnedToCore(TaskBuzzer,"Task_buzz",10000,NULL,1,NULL,1);
}

void loop() {
  
}

void Task1(void * parameter) {
    // Vòng lặp vô hạn để thực hiện công việc của Task
  while (1) {
    // Kiểm tra và giữ Semaphore countingSemaphore
    if (xSemaphoreTake(countingSemaphore, portMAX_DELAY) == pdTRUE) {
      // In ra thông báo Semaphore đã được lấy và hiển thị số lần Semaphore còn lại
      Serial.println("semaphore taken ");
      Serial.println(uxSemaphoreGetCount(countingSemaphore));
      // Gọi hàm để thực hiện chức năng bật/tắt đèn
      led_on_off();
    }
    // Chờ 1 giây trước khi kiểm tra lại Semaphore
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void Task2(void * parameter) {
  // Vòng lặp vô hạn để thực hiện công việc của Task
  while (1) {
    // Kiểm tra và giữ Semaphore ledSemaphore
    if (xSemaphoreTake(ledSemaphore, portMAX_DELAY) == pdTRUE) {
      // Đặt trạng thái của đèn led1 dựa trên giá trị của biến ledState
      digitalWrite(led1, ledState ? HIGH : LOW);
    }
    // Chờ 1 giây trước khi kiểm tra lại Semaphore
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void Task3(void * parameter) {
  // Vòng lặp vô hạn để thực hiện công việc của Task
  while (1) {
    // Đọc trạng thái của nút nhấn btn_se
    int BTN1 = digitalRead(btn_se);
    // Kiểm tra nếu nút nhấn được nhấn xuống (LOW)
    if (BTN1 == LOW) {
      // Chờ cho đến khi nút nhấn được thả ra
      while (digitalRead(btn_se) == LOW) {}; 
      // Gửi Semaphore countingSemaphore khi nút nhấn được nhấn
      xSemaphoreGive(countingSemaphore);
      // In ra Serial Monitor thông báo nút nhấn đã được nhấn
      Serial.println("Button on");
    }
    // Chờ 100ms trước khi kiểm tra lại trạng thái của nút nhấn
    vTaskDelay(100);
  }
}

void Task4(void *parameter) {
  // Vòng lặp vô hạn để thực hiện công việc của Task
  while (1) {
    // Đọc trạng thái của nút nhấn on_button và off_button
    int on_button = digitalRead(button2);
    int off_button = digitalRead(button3);

    // Kiểm tra nếu trạng thái của nút nhấn on_button thay đổi
    if (on_button != BTN_STATE) {
      // Kiểm tra nếu nút on_button được nhấn xuống
      if (!on_button) {
        // Chờ cho đến khi nút nhấn được thả ra
        while (!digitalRead(button2)) {}; 
        // Gửi Semaphore ledSemaphore khi nút on_button được nhấn
        xSemaphoreGive(ledSemaphore);
         // Đặt biến ledState thành true để bật đèn
        ledState = true;
        // In ra Serial Monitor thông báo nút off_button đã được nhấn
        Serial.println("On button pressed");
      }
      // Đặt lại trạng thái của BTN_STATE
      BTN_STATE = 1;
    }
    if (off_button != BTN_STATE) {
      if (!off_button) {
        while (!digitalRead(button3)) {}; 
        // Gửi Semaphore ledSemaphore khi nút off_button được nhấn
        xSemaphoreGive(ledSemaphore);
        // Đặt biến ledState thành false để tắt đèn
        ledState = false;
        // In ra Serial Monitor thông báo nút off_button đã được nhấn
        Serial.println("Off button pressed");
      }
      BTN_STATE = 1;
    }
    // Chờ 100ms trước khi kiểm tra lại trạng thái của nút nhấn
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void readSensorTask(void * parameter) {
    // Đọc dữ liệu từ cảm biến
    DataSensor data = readSensorData(); 
    // Cập nhật dữ liệu mới lên màn hình LCD
    printLCD(data.humid, data.temp);   
}

DataSensor readSensorData() {
  // Khởi tạo biến DataSensor để lưu trữ dữ liệu từ cảm biến
  DataSensor Data;
  // Đọc nhiệt độ từ cảm biến và gán vào trường temp của biến Data
  Data.temp = dht.readTemperature();
  // Đọc độ ẩm từ cảm biến và gán vào trường humid của biến Data
  Data.humid = dht.readHumidity();
  // Trả về dữ liệu đã đọc từ cảm biến
  return Data;
}

void printLCD(int humid, int temp) {
  // Kiểm tra xem nhiệt độ hoặc độ ẩm có phải là giá trị NaN (not a number) không
   if (isnan(temp) || isnan(humid)) {
    // In ra Serial Monitor thông báo lỗi nếu không đọc được dữ liệu từ cảm biến
    Serial.println("Failed to read from DHT sensor!");
    return;
   }
  // In ra Serial Monitor nhiệt độ và độ ẩm đã đọc được từ cảm biến
  Serial.print("Temperature: ");
  Serial.print(temp, 1); 
  Serial.print(" °C\t");      
  Serial.print("Humidity: ");
  Serial.print(humid, 1);  
  Serial.println(" %");         

  display.clearDisplay();
  display.setTextSize(2);      
  display.setTextColor(SSD1306_WHITE);  
  display.setCursor(0,0);     
  display.print("Temp: ");
  display.print(temp, 1);
  display.println("C");

  display.setCursor(0, 15);
  display.print("Hum: ");
  display.print(humid, 1);
  display.println("%");
  display.display(); 
}

void led_on_off() {
  // Lặp 3 lần để thực hiện chức năng bật/tắt đèn led2
  for (int i = 0; i < 3; i++) {
    digitalWrite(led2, HIGH);
    delay(200);
    digitalWrite(led2, LOW);
    delay(200);
  }
}
void TaskBuzzer(void * parameter) {
  for(;;){
    int time = 500; // Thời gian mỗi nốt nhạc
int melody[] = {
  NOTE_AS4, REST, NOTE_AS4, REST, NOTE_AS4, REST, NOTE_AS4, REST,
  NOTE_AS4, NOTE_B4, NOTE_DS5,
  NOTE_AS4, REST, NOTE_AS4, REST,
  NOTE_AS4, NOTE_B4, NOTE_DS5,
  NOTE_AS4, REST, NOTE_AS4, REST,
  NOTE_AS4, NOTE_B4, NOTE_DS5,
  NOTE_AS4, REST, NOTE_AS4, REST,
  NOTE_AS4, NOTE_B4, NOTE_DS5,
  NOTE_F5, REST, NOTE_F5, REST,
  NOTE_GS5, NOTE_FS5, NOTE_F5,
  NOTE_AS4, REST, NOTE_AS4, REST,
  NOTE_GS5, NOTE_FS5, NOTE_F5,
  NOTE_AS4, REST, NOTE_AS4, REST,
  NOTE_AS4, NOTE_B4, NOTE_DS5,
  NOTE_AS4, REST, NOTE_AS4, REST,
  NOTE_AS4, NOTE_B4, NOTE_DS5,
  NOTE_AS4, REST, NOTE_AS4, REST,
  REST
};

int durations[] = {
  4, 4, 4, 4, 4, 4, 4, 4,
  3, 3, 4,
  4, 4, 4, 4,
  3, 3, 4,
  4, 4, 4, 4,
  3, 3, 4,
  4, 4, 4, 4,
  3, 3, 4,
  4, 4, 4, 4,
  3, 3, 4,
  4, 4, 4, 4,
  3, 3, 4,
  4, 4, 4, 4,
  3, 3, 4,
  4, 4, 4, 4,
  3, 3, 4,
  4, 4, 4, 4,
  1
};

   int size = sizeof(durations) / sizeof(int);
  // Lặp qua từng nốt nhạc trong mảng melody và durations
  for (int note = 0; note < size; note++) {
    // Tính toán thời gian của nốt nhạc, lấy 1000 chia cho độ dài nốt nhạc
    // Ví dụ: nốt nhạc tròn = 1000 / 4, nốt nhạc tám = 1000/8, v.v.
    int duration = 1000 / durations[note];
    // Phát nốt nhạc với tần số và thời gian được chỉ định
    tone(BUZZER_PIN, melody[note], duration);

    // Để phân biệt giữa các nốt nhạc, đặt một khoảng thời gian tối thiểu giữa chúng.
    // Thời gian tạm dừng giữa các nốt nhạc + 30% có vẻ hoạt động tốt:
    int pauseBetweenNotes = duration * 1.30;
    // Chờ một khoảng thời gian giữa các nốt nhạc
    delay(pauseBetweenNotes);

    // Dừng phát nốt nhạc
    noTone(BUZZER_PIN);
  }
}
}


