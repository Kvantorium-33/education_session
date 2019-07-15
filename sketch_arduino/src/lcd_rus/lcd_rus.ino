#include <LiquidCrystalRus.h> //инициализация библиотеки 

// инициализируем объект-экран, передаём использованные пины
// для подключения контакты на Arduino в порядке:
// RS, E, DB4, DB5, DB6, DB7

LiquidCrystalRus lcd(12, 13, A2, A3, A4, A5); 

int counter; // простая переменная для счётчика

void setup() { 
  lcd.begin(16, 2);// описание экрана 1602(16 столбцов,2 строки):
  
  lcd.print("Привет, мир!"); //выводим текст 
}

void loop() {
  lcd.setCursor(0, 1); //переводим курсор на 0 столбец 1 строку
  lcd.print("Я ТЕЛЕГА..."); //выводим текст  
  delay(5000); // ждём
}