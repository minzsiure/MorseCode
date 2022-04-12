#include <string.h>
#include <mpu6050_esp32.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <WiFi.h>

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
const char USER[] = "EVA_XIE";
uint32_t primary_timer = 0;
//button setup
const int BUTTON_dot = 45;
const int BUTTON_dash = 39;
const int BUTTON_check = 38;
const int BUTTON_post = 34;

//wifi
const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const int GETTING_PERIOD = 2000; //periodicity of getting a number fact.
const int BUTTON_TIMEOUT = 1000; //button timeout in milliseconds
const uint16_t IN_BUFFER_SIZE = 1000; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response
char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response

const uint8_t LOOP_PERIOD = 10; //milliseconds
//uint32_t primary_timer = 0;
char network[] = "EECS_Labs";
char password[] = "";

uint8_t scanning = 0;//set to 1 if you'd like to scan for wifi networks (see below):
uint8_t channel = 1; //network channel on 2.4 GHz
byte bssid[] = {0x04, 0x95, 0xE6, 0xAE, 0xDB, 0x41}; //6 byte MAC address of AP you're targeting.


char str[100];
char input[100];
int input_index = 0;

//button 1 
const uint8_t PRESS_B1 = 0;
const uint8_t RES_B1 = 1;
uint8_t b1_state = PRESS_B1;
uint8_t prev_b1 = 1;

//button 2 
const uint8_t PRESS_B2 = 0;
const uint8_t RES_B2 = 1;
uint8_t b2_state = PRESS_B2;
uint8_t prev_b2 = 1;


//button 3 state machine
const uint8_t START = 0;
const uint8_t PUSH = 1;
const uint8_t PUSH_AGAIN_SPACE = 2;
const uint8_t RESET = 3;
const uint8_t W1 = 4;
const uint8_t W2 = 5;
uint8_t button_state = START;
uint8_t prev_button = 1;
unsigned long timer;  //used for storing millis() readings.

//button 4
const uint8_t PRESS_B4 = 0;
const uint8_t RES_B4 = 1;
uint8_t b4_state = PRESS_B4;
uint8_t prev_b4 = 1;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); //begin serial comms

  //SET UP SCREEN
  tft.init();  //init screen
  tft.setRotation(2); //adjust rotation
  tft.setTextSize(1); //default font size
  tft.fillScreen(TFT_BLACK); //fill background
  tft.setTextColor(TFT_GREEN, TFT_BLACK); //set color of font to green foreground, black background

  // NETWORK
  if (scanning) {
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
      Serial.println("no networks found");
    } else {
      Serial.print(n);
      Serial.println(" networks found");
      for (int i = 0; i < n; ++i) {
        Serial.printf("%d: %s, Ch:%d (%ddBm) %s ", i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "open" : "");
        uint8_t* cc = WiFi.BSSID(i);
        for (int k = 0; k < 6; k++) {
          Serial.print(*cc, HEX);
          if (k != 5) Serial.print(":");
          cc++;
        }
        Serial.println("");
      }
    }
  }
  delay(100); //wait a bit (100 ms)
  //if using regular connection use line below:
  WiFi.begin(network, password);
  //if using channel/mac specification for crowded bands use the following:
  //WiFi.begin(network, password, channel, bssid);
  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count < 6) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);

  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.printf("%d:%d:%d:%d (%s) (%s)\n", WiFi.localIP()[3], WiFi.localIP()[2],
                  WiFi.localIP()[1], WiFi.localIP()[0],
                  WiFi.macAddress().c_str() , WiFi.SSID().c_str());
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }
  memset(str, 0, sizeof str);

  //button setup
  pinMode(BUTTON_dot, INPUT_PULLUP); //set input pin as an input!
  pinMode(BUTTON_dash, INPUT_PULLUP);
  pinMode(BUTTON_check, INPUT_PULLUP);
  pinMode(BUTTON_post, INPUT_PULLUP);
}


void loop() {
  // put your main code here, to run repeatedly:
  uint8_t button1 = digitalRead(BUTTON_dot);
  uint8_t button2 = digitalRead(BUTTON_dash);
  uint8_t button3 = digitalRead(BUTTON_check);
  uint8_t button4 = digitalRead(BUTTON_post);

//  Serial.printf("button 1 %d\t", button1);
//  Serial.printf("button 2 %d\t", button2);
//  Serial.printf("button 3 %d\t", button3);
//  Serial.printf("button 4 %d\n", button4);
  check_button_dot(button1);
  check_button_dash(button2);
  check_button_check(button3);
  check_button_post(button4);

  if (sizeof str >0){
  tft.setCursor(0, 0, 2);
  tft.setTextSize(2);
  tft.println(str);}
  
}

void check_button_dot(uint8_t button1){
  //1 is rest 0 is press
  switch(b1_state){
    case PRESS_B1:
      if(prev_b1 == 1 && button1 == 0){
        prev_b1 = 0;
        b1_state = RES_B1;
        while (millis()-primary_timer<LOOP_PERIOD); //wait for primary timer to increment
        primary_timer =millis();
       }
      break;

    case RES_B1:
      if (prev_b1 == 0 && button1 == 1) {
        Serial.println("pressed .");
        input[input_index] = '.';
        input_index++;
        b1_state = PRESS_B1;
        prev_b1 = 1;
        while (millis()-primary_timer<LOOP_PERIOD); //wait for primary timer to increment
        primary_timer =millis();
        
      }
      break;
    }  
}

void check_button_dash(uint8_t button2){
  switch(b2_state){
    case PRESS_B2:
      if(prev_b2 == 1 && button2 == 0){
        prev_b2 = 0;
        b2_state = RES_B2;
        while (millis()-primary_timer<LOOP_PERIOD); //wait for primary timer to increment
        primary_timer =millis();
       }
      break;

    case RES_B2:
      if (prev_b2 == 0 && button2 == 1) {
        Serial.println("pressed -");
        input[input_index] = '-';
        input_index++;
        b2_state = PRESS_B2;
        prev_b2 = 1;
        while (millis()-primary_timer<LOOP_PERIOD); //wait for primary timer to increment
        primary_timer =millis();
      }
      break;
    }  
}

void check_button_check(uint8_t button3){
  switch(button_state){
    //button 3 state machine
    case START:
      if (prev_button == 1 && button3 == 0){
        prev_button = 0;
        button_state = W1;
        }
      break;

    case W1:
      if (prev_button == 0 && button3 == 1) {
        button_state = PUSH;
        prev_button = 1;
        timer = millis();
      }
      break;

    case PUSH:
      //wait for a second to see if there's another push in a row
      if (millis() - timer < 1000) {
        //if press again
        if (prev_button == 1 && button3 == 0) {
          button_state = W2;
          prev_button = 0; } //0 is pressed 
        
        
        //if button on press again
        
      }
      else{button_state = RESET;}
      break;

    case W2:
      if (prev_button == 0 && button3 == 1) {
        button_state = PUSH_AGAIN_SPACE;
        prev_button = 1;
      }
      break;

    case PUSH_AGAIN_SPACE:
      Serial.println("pressed space");
      strcat(str, " ");
      while (millis()-primary_timer<LOOP_PERIOD); //wait for primary timer to increment
      primary_timer =millis();
      button_state = START;
      break;

    case RESET:
      Serial.println("pressed reset");
      //reset to a new letter
      //TODO: look up the current letter in morse, append to show_screen, then clear input[]
      //      char *pch;
      //      pch = strtok(input, '\0');
      mose_lookUp(input);
      memset(input, 0, sizeof input);
      input_index = 0;
      button_state = START;
      break;
    }
  }

void check_button_post(uint8_t button4){
  switch(b4_state){
    case PRESS_B4:
      if(prev_b4 == 1 && button4 == 0){
        prev_b4 = 0;
        b4_state = RES_B4;
       }
      break;

    case RES_B4:
      if (prev_b4 == 0 && button4 == 1) {
        Serial.println("pressed POST");
        //post it
        char body[100]; //for body
        //{"user":"jodalyst","message":"cats and dogsss"}
        sprintf(body,"{\"user\":\"%s\",\"message\":\"%s\"}",USER, str);//generate body, posting to User, 1 step
        int body_len = strlen(body); //calculate body length (for header reporting)
        sprintf(request_buffer,"POST http://608dev.net/sandbox/morse_messenger HTTP/1.1\r\n");
        strcat(request_buffer,"Host: 608dev.net\r\n");
        strcat(request_buffer,"Content-Type: application/json\r\n");
        sprintf(request_buffer+strlen(request_buffer),"Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
        strcat(request_buffer,"\r\n"); //new line from header to body
        strcat(request_buffer,body); //body
        strcat(request_buffer,"\r\n"); //new line
        Serial.println(request_buffer);
        do_http_request("608dev.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT,true);
        Serial.println(response_buffer); //viewable in Serial Terminal
        
        //re-zero local arrays
        memset(str, 0, sizeof str);
        memset(input, 0, sizeof input);
        tft.fillScreen(TFT_BLACK);
        b4_state = PRESS_B4;
        prev_b4 = 1;
      }
      break;
    }  
  }

//process morse code
typedef struct
{
  char *morse;
  char *ascii;
} morse_table;


void mose_lookUp(char input[100]){
  morse_table table[] = {
      {".-", "A"},
      {"-...", "B"},
      {"-.-.", "C"},
      {"-..","D"},
      {".","E"},
      {"..-.","F"},
      {"--.","G"},
      {"....","H"},
      {"..","I"},
      {".---","J"},
      {"-.-","K"},
      {".-..","L"},
      {"--","M"},
      {"-.","N"},
      {"---","O"},
      {".--.","P"},
      {"--.-","Q"},
      {".-.","R"},
      {"...","S"},
      {"-","T"},
      {"..-","U"},
      {"...-","V"},
      {".--","W"},
      {"-..-","X"},
      {"-.--","Y"},
      {"--..","Z"},
      {".----","1"},
      {"..---","2"},
      {"...--","3"},
      {"....-","4"},
      {".....","5"},
      {"-....","6"},
      {"--...","7"},
      {"---..","8"},
      {"----.","9"},
      {"-----","0"}
  };

  for(int i = 0; i<36; ++i){
    if (!strcmp(input, table[i].morse))
      strcat(str,table[i].ascii);
    }
}

/*----------------------------------
   char_append Function:
   Arguments:
      char* buff: pointer to character array which we will append a
      char c:
      uint16_t buff_size: size of buffer buff

   Return value:
      boolean: True if character appended, False if not appended (indicating buffer full)
*/
uint8_t char_append(char* buff, char c, uint16_t buff_size) {
  int len = strlen(buff);
  if (len > buff_size) return false;
  buff[len] = c;
  buff[len + 1] = '\0';
  return true;
}

/*----------------------------------
   do_http_GET Function:
   Arguments:
      char* host: null-terminated char-array containing host to connect to
      char* request: null-terminated char-arry containing properly formatted HTTP GET request
      char* response: char-array used as output for function to contain response
      uint16_t response_size: size of response buffer (in bytes)
      uint16_t response_timeout: duration we'll wait (in ms) for a response from server
      uint8_t serial: used for printing debug information to terminal (true prints, false doesn't)
   Return value:
      void (none)
*/
void do_http_GET(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial) {
  // first time querying will just do this. and turn into another state that you have to wait
  Serial.println("this is my request");
  WiFiClient client; //instantiate a client object
  if (client.connect(host, 80)) { //try to connect to host on port 80
    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      client.readBytesUntil('\n', response, response_size);
      if (serial) Serial.println(response);
      if (strcmp(response, "\r") == 0) { //found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis() - count > response_timeout) break;
    }
    memset(response, 0, response_size);
    count = millis();
    while (client.available()) { //read out remaining text (body of response)
      char_append(response, client.read(), OUT_BUFFER_SIZE);
    }
    if (serial) Serial.println(response);
    client.stop();
    if (serial) Serial.println("-----------");
  } else {
    if (serial) Serial.println("connection failed :/");
    if (serial) Serial.println("wait 0.5 sec...");
    client.stop();
  }

}


/*----------------------------------
 * do_http_request Function:
 * Arguments:
 *    char* host: null-terminated char-array containing host to connect to
 *    char* request: null-terminated char-arry containing properly formatted HTTP request
 *    char* response: char-array used as output for function to contain response
 *    uint16_t response_size: size of response buffer (in bytes)
 *    uint16_t response_timeout: duration we'll wait (in ms) for a response from server
 *    uint8_t serial: used for printing debug information to terminal (true prints, false doesn't)
 * Return value:
 *    void (none)
 */
void do_http_request(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial){
  WiFiClient client; //instantiate a client object
  if (client.connect(host, 80)) { //try to connect to host on port 80
    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      client.readBytesUntil('\n',response,response_size);
      if (serial) Serial.println(response);
      if (strcmp(response,"\r")==0) { //found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis()-count>response_timeout) break;
    }
    memset(response, 0, response_size);  
    count = millis();
    while (client.available()) { //read out remaining text (body of response)
      char_append(response,client.read(),OUT_BUFFER_SIZE);
    }
    if (serial) Serial.println(response);
    client.stop();
    if (serial) Serial.println("-----------");  
  }else{
    if (serial) Serial.println("connection failed :/");
    if (serial) Serial.println("wait 0.5 sec...");
    client.stop();
  }
}        
