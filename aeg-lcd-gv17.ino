#include <Bounce2.h>

//
// 6 x 38 segment LCD
// Based on an original sketch by Ian Dickinson (@iandickinson) of @HSNotts.
//
// For the November 2020 clock object I'm using a 5v 16MHz Pro Micro which is
// a cheap copy of the Sparkfun Pro Micro (https://www.sparkfun.com/products/12640)
//
// In the Arduino IDE, add Sparkfun boards and use the 5v 16MHz setting. Using the
// 8 MHz setting to upload will brick it until recovered using one of the techniques
// listed here...
// https://learn.sparkfun.com/tutorials/pro-micro--fio-v3-hookup-guide/troubleshooting-and-faq


#define DI  2
#define R   3
#define FL  4
#define LD  5
#define CLK 6

#define BTN1 7
#define BTN2 8
const int RXLED = 17;

Bounce btn1 = Bounce();
Bounce btn2 = Bounce();

uint32_t adj = 0;

long secs;
int hours, mins;
char message[256];
int msg_cnt = 0;
int msg_disp = 0;
int lcd_data[30];
int state = 0;
unsigned long next_update = 0;

int font[][5] = {
  {0x53, 0x7e, 0xe9, 0xbf, 0x34}, // 0
  {0x4f, 0x85, 0xdf, 0xc0, 0xc8}, // 1
  {0x51, 0x9f, 0xe9, 0xc9, 0xb4}, // 2
  {0x51, 0x84, 0xc9, 0xbd, 0xb4}, // 3
  {0x33, 0xfc, 0x01, 0xff, 0xb6}, // 4
  {0x73, 0xfc, 0xc9, 0xbc, 0x86}, // 5
  {0x53, 0xfe, 0xe9, 0xbc, 0x84}, // 6
  {0x71, 0x9b, 0xe0, 0x00, 0x5e}, // 7
  {0x53, 0xde, 0xe9, 0xbd, 0xb4}, // 8
  {0x53, 0xcc, 0xc9, 0xbf, 0xb4}, // 9
  {0x53, 0xff, 0xe1, 0xff, 0xb4}, // A
  {0x73, 0xff, 0xe9, 0xbd, 0xb4}, // B
  {0x52, 0x7a, 0xe9, 0x80, 0x04}, // C
  {0x72, 0x7b, 0xe9, 0xbf, 0x34}, // D
  {0x73, 0xff, 0xe9, 0xc0, 0x86}, // E
  {0x73, 0xff, 0xe0, 0x00, 0x86}, // F
  {0x52, 0x7a, 0xe9, 0xbf, 0x84}, // G
  {0x33, 0xff, 0xe1, 0xff, 0xb6}, // H
  {0x7d, 0x85, 0xdf, 0xc0, 0xce}, // I
  {0x7d, 0x84, 0xde, 0x00, 0xce}, // J
  {0x33, 0xfb, 0xe1, 0xfc, 0x96}, // K
  {0x32, 0x7b, 0xe9, 0xc0, 0x00}, // L
  {0x37, 0x7b, 0xe1, 0xff, 0x7e}, // M
  {0x37, 0x7b, 0xe1, 0xff, 0xf6}, // N
  {0x52, 0x7a, 0xe9, 0xbf, 0x34}, // O
  {0x73, 0xff, 0xe0, 0x09, 0xb4}, // P
  {0x52, 0x7e, 0xeb, 0xff, 0x34}, // Q
  {0x73, 0xff, 0xe1, 0xfd, 0xb4}, // R
  {0x53, 0xcc, 0xc9, 0xbc, 0x84}, // S
  {0x7d, 0x84, 0x1e, 0x00, 0xce}, // T
  {0x32, 0x7a, 0xe9, 0xbf, 0x36}, // U
  {0x32, 0x78, 0x7f, 0x1f, 0x36}, // V
  {0x32, 0x7e, 0xf3, 0xbf, 0x36}, // W
  {0x33, 0xdf, 0xe1, 0xfd, 0xb6}, // X
  {0x32, 0xcc, 0x1e, 0x09, 0xb6}, // Y
  {0x71, 0x9b, 0xe9, 0xc0, 0x5e}, // Z
  {0x00, 0x00, 0x00, 0x00, 0x00}, //
  {0x4d, 0x84, 0x08, 0x00, 0xc8}, // !
  {0x52, 0x04, 0x1a, 0x09, 0xb4}, // ?
  {0x53, 0xff, 0xe9, 0xc8, 0x84}, // Â£
  {0x53, 0xde, 0xeb, 0xd0, 0x5c}, // &
  {0x01, 0xfc, 0x00, 0x0f, 0x80}, // -
  {0x0d, 0xfc, 0x16, 0x0f, 0xc8}, // +
  {0x00, 0x00, 0x08, 0x00, 0x00}, // .
  {0x0c, 0x00, 0x00, 0x00, 0x00}, // '
};

void setup(void) {
  // mess with LEDs - try and shut them off
  TX_RX_LED_INIT;
  pinMode(RXLED, OUTPUT);
  TXLED0;
  RXLED0;
  // strobe required on FL...
  tone(FL, 64);
  pinMode(DI, OUTPUT);
  pinMode(R, OUTPUT);
  pinMode(LD, OUTPUT);
  pinMode(CLK, OUTPUT);
  digitalWrite(DI, LOW);
  digitalWrite(R, LOW);
  digitalWrite(LD, HIGH);
  digitalWrite(CLK, HIGH);
  btn1.attach(BTN1, INPUT_PULLUP);
  btn2.attach(BTN2, INPUT_PULLUP);
  btn1.interval(25);
  btn2.interval(25);
  Serial.begin(9600);
  Serial.println("Starting clock");
}

void loop(void)
{
  time_update(millis());
  time2lcd();
  if (millis() > next_update) {
    next_update = millis() + 300;
    if (state == 0 && msg_cnt)
      msg2lcd();
    lcd_update();
  }
  do_serial();
  do_buttons();
}

void do_buttons() {
  btn1.update();
  btn2.update();
  if (btn1.fell()) {
    adj += (1000uL * 3600uL);
  }
  if (btn2.fell()) {
    adj += (1000uL * 60uL);
  }
  adj %= (1000uL * 3600uL * 24uL);
}

void do_serial() {
  if (!Serial.available()) return;
  char c = Serial.read();
  switch (state) {
    case 0:              // not receiving message
      msg_cnt = 0;
      msg_disp = 0;
      if (c != 0xd) {
        message[msg_cnt++] = c;
        state = 1;
      }
      break;
    case 1:              // receiving message
      if (c == 0xd)
        state = 0;
      else if (msg_cnt < 256)
        message[msg_cnt++] = c;
      else
        state = 0;
      break;
    default:
      break;
  }
}

void time_update(uint32_t mil) {
  secs = (mil + adj) / 1000;
  hours = (secs / 3600) % 24;
  mins = (secs / 60) % 60;
  secs %= 60;
}

void msg2lcd(void) {
  int i, p;
  char c;
  for (i = 0; i < 6; i++) {
    p = msg_disp + i - 6;
    if (p < 0)
      c = ' ';
    else if (p < msg_cnt)
      c = message[p];
    else
      c = ' ';
    char2lcd(i, c);
  }
  msg_disp++;
  if (msg_disp > msg_cnt + 6)
    msg_disp = 0;
}

void time2lcd(void) {
  num2lcd(0, hours / 10);
  num2lcd(1, hours % 10);
  num2lcd(2, mins / 10);
  num2lcd(3, mins % 10);
  num2lcd(4, secs / 10);
  num2lcd(5, secs % 10);
}

void num2lcd(int pos, int d) {
  int i;
  for (i = pos * 5; i < (pos * 5 + 5); i++) {
    lcd_data[i] = font[d][i % 5];
  }
}

void char2lcd(int pos, char c) {
  int i, d;
  if (c >= '0' && c <= '9')      d = c - '0';
  else if (c >= 'A' && c <= 'Z') d = c - 'A' + 10;
  else if (c >= 'a' && c <= 'z') d = c - 'a' + 10;
  else if (c == ' ')             d = 36;
  else if (c == '!')             d = 37;
  else if (c == '?')             d = 38;
  else if (c == 'Â£')            d = 39;
  else if (c == '&')             d = 40;
  else if (c == '-')             d = 41;
  else if (c == '+')             d = 42;
  else if (c == '.')             d = 43;
  else if (c == '\'')            d = 44;
  else                           d = 39;
  for (i = pos * 5; i < (pos * 5 + 5); i++)
    lcd_data[i] = font[d][i % 5];
}

void lcd_update(void) {
  int i, j;
  digitalWrite(LD, LOW);
  for (i = 0; i < 30; i++) {
    for (j = 0; j < 8; j++) {
      if (lcd_data[i] & (1 << (7 - j)))
        digitalWrite(DI, HIGH);
      else
        digitalWrite(DI, LOW);
      digitalWrite(CLK, LOW);
      digitalWrite(CLK, HIGH);
    }
  }
  digitalWrite(LD, HIGH);
}
