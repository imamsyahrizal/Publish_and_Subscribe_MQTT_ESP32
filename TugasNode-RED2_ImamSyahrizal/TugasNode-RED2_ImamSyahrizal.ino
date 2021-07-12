/*********
  Used tutorial : https://randomnerdtutorials.com/esp32-mqtt-publish-subscribe-arduino-ide/

  TUGAS Node-RED 2 
  Imam Syahrizal
  18/424967/TK/46662

  Semua Serial.print hanya sebagai informasi tambahan yang akan dikirim ke PC, tidak sebagai sarana transfer data
*********/

// menambahkan library yang diperlukan
#include<WiFi.h>
#include<PubSubClient.h>
#include<Wire.h>
#include<DHT.h>

// definisi pin yang digunakan
#define buttonPin 21
#define ledPin 19
#define DHTPIN 4

// definisi sensor DHT22
#define DHTTYPE DHT22 // definisi jenis sensor, yakni DHT22
DHT dht(DHTPIN, DHTTYPE); //definisi objek dht yang diperlukan untuk library DHT.h

// Definisi WiFi
const char* ssid = "HUAWEI-B311-2F1E";
const char* password = "AT7TNA61YF1";

// Definisi IP mqtt broker
const char* mqtt_server = "192.168.8.116";

// Definisi objek WiFiClient dan PubSubClient
WiFiClient espClient;
PubSubClient client(espClient);

// variabel untuk waktu pembacaan sensor
long waktuSebelumnya = 0;
char msg[50];
int value = 0;

// variable untuk menyimpan suhu dan kelembaban
float temperature = 0;
float humidity = 0;

// variable untuk menyimpan state
int buttonState = 0;
int ledState = 0;

void setup() {
  // put your setup code here, to run once:
  pinMode(buttonPin, INPUT);  // Definisi pin buttonPin (GPIO21) sebagai masukan berupa ADC
  pinMode(ledPin, OUTPUT); // Definisi pin ledPin (GPIO19) sebagai keluaran berupa sinyal High atau Low
  digitalWrite(ledPin, LOW); // Kondisi awal ledPin adalah LOW
  
  Serial.begin(115200); // memulai Sambungan serial usb dengan baudrate 115200
  dht.begin(); // memulai sensor dht

  // Kebutuhan setup WiFi dan mqtt
  setup_wifi();
  client.setServer(mqtt_server, 1883); // port 1883 sebagai default mqtt
  client.setCallback(callback); // callback digunakan untuk subscribe mqtt

}

//Fungsi setup WiFi
void setup_wifi(){
  delay(10);
  Serial.println();
  Serial.print("Berusaha terhubung ke ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Fungsi yang akan melakukan subscribe topik 
void callback(char* topic, byte* message, unsigned int length){
  Serial.print("Pesan diterima dengan topik : ");
  Serial.print(topic);
  Serial.print(". Pesan : ");
  String messageTemp;

  for (int i=0; i<length; i++){
  Serial.print((char)message[i]);
  messageTemp += (char)message[i];
  }
  Serial.println();

  if(String(topic) == "esp32/output"){
    if(messageTemp == "change"){
      Serial.print("Tombol LED ditekan pada dashboard");
      ledState = ~ledState;
      digitalWrite(ledPin, ledState);

      // kirim status LED ke topik esp32/led
      char statusString[8];
      dtostrf(ledState, 1, 2, statusString);
      Serial.print("LED State : ");
      Serial.println(statusString);
      client.publish("esp32/led", statusString);
    }
  }
}

// Fungsi untuk memastikan akan selalu Terkoneksi dengan WiFi
void reconnect(){
  while(!client.connected()){
    Serial.print("Berusaha menghubungkan MQTT...");
    if(client.connect("ESP8266Client")){
      Serial.println("Terhubung");
      client.subscribe("esp32/output");
    }else{
      Serial.print("gagal, rc=");
      Serial.print(client.state());
      Serial.println(" coba lagi dalam 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  
  // Cek Sambungan WiFi
  if(!client.connected()){
    reconnect();
  }
  client.loop();

  // Cek waktu untuk update, jika sudah sesuai lakukan publish data
  long sekarang = millis();
  if(sekarang - waktuSebelumnya > 5000){
    waktuSebelumnya = sekarang;

    temperature = dht.readTemperature();
    char tempString[8];
    dtostrf(temperature, 1, 2, tempString);
    Serial.print("Suhu: ");
    Serial.println(tempString);
    client.publish("esp32/temperature", tempString);

    humidity = dht.readHumidity();
    char humString[8];
    dtostrf(humidity, 1, 2, humString);
    Serial.print("Humidity: ");
    Serial.println(humString);
    client.publish("esp32/humidity", humString);
  }

  /* Deteksi masukkan dari push-button, jika ada maka akan mengganti state LED
   *  dan mengirim state LED
   */
  buttonState = digitalRead(buttonPin);
    if( buttonState == 1){
      debounce(buttonPin, HIGH);
      ledState = ~ledState;
      digitalWrite(ledPin, ledState);
      char statusString[8];
      dtostrf(ledState, 1, 2, statusString);
      Serial.print("LED State: ");
      Serial.println(statusString);
      client.publish("esp32/led", statusString);
      }
}

/* fungsi yang akan meminimalisir salah pembacaan state dari push-button karena
 *  dalam menekan push button pasti akan dibaca beberapa kali oleh ESP tanpa adanya
 *  waktu jeda tertentu
 */
void debounce(int pin, unsigned char active_logic){
  delay(10);
  if(digitalRead(pin)==active_logic){
    delay(100);
    while(digitalRead(buttonPin)==active_logic){};
  }
}
