#define inputPin1 D0
#define inputPin2 D1
#define EN1 D5

void setup() {
  pinMode(EN1, OUTPUT);
  pinMode(inputPin1, OUTPUT);
  pinMode(inputPin2, OUTPUT);  
  Serial.begin(115200);
}

void loop() {
  int pwm = 100;
  digitalWrite(inputPin1, HIGH);
  digitalWrite(inputPin2, LOW);
  analogWrite(EN1, pwm);
}
