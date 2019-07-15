// Скетч для управления моторами используя драйвер (ПИД-регулятор) (демо)

// выводы к которым подключен драйвер моторов
#define LEFT_MOTOR_PIN_1          5
#define LEFT_MOTOR_PIN_2          6
#define RIGHT_MOTOR_PIN_1         9
#define RIGHT_MOTOR_PIN_2         10

#define LEFT_ENCODER_INTERRUPT_NB   0  // номер прерывания
#define RIGHT_ENCODER_INTERRUPT_NB  1  // номер прерывания

#define TIME_DEMO_SWITCH_PERIOD_MS  5000  // интервал публикации значений

#define TIME_PUB_PERIOD_MS          100  // интервал публикации значений

#define Kp                        630.0    // пропорциональный коэффициент для ПИД регулятора (41.7)
#define Ki                        0.4      // интегральный коэффициент для ПИД регулятора
#define Kd                        0.3      // дифференциальный коэффициент для ПИД регулятора
#define I_SIZE                    15      // длинна выборки последних интегральных состовляющих   
#define K_SPEED                   0.42      

#define MOTOR_VALUE_MAX           255      // максимальное значение подаваемое на драйвер
#define MOTOR_VALUE_MIN           50       // минимальное значение подаваемое на драйвер

#define ROBOT_LINEAR              0.15      // линейная скорость робота
#define ROBOT_ANGULAR             0.0      // угловая скорость робота

#define WHEEL_BASE                0.184     // база колесная в метрах
#define WHEEL_DIAMETER            0.084     // диаметр колеса в метрах
#define WHEEL_IMPULSE_COUNT       1225.0    // количество импульсов на оборот колеса

#define COUNT_MOTORS              2         // количество моторов

// стороны робота
#define LEFT                      0
#define RIGHT                     1

// режимы управления
#define STOP                      0
#define MOVE_FRONT                1
#define MOVE_REAR                 2
#define ROTATE_LEFT               3
#define ROTATE_RIGHT              4

unsigned long last_time_switch; // время последней публикации
unsigned long last_time_pub; // время последней публикации
int mode = STOP;         // режим управления

float linear = ROBOT_LINEAR;
float angular = ROBOT_ANGULAR;

float enc_count[COUNT_MOTORS] = {0.0, 0.0};      // счетчик для левого колеса
float speed_wheel[COUNT_MOTORS] = {0.0, 0.0};    // скорость моторов

float e_prev[COUNT_MOTORS] = {0.0, 0.0};          //последнее значение разницы скорости движения
float I_prev[COUNT_MOTORS][I_SIZE];               //последние значения выборки интегральной составляющей ПИД регулятора
int i_count = 0;

void setup() {
  // инициализация выходов на драйверы управления моторами
  pinMode(LEFT_MOTOR_PIN_1, OUTPUT);
  pinMode(LEFT_MOTOR_PIN_2, OUTPUT);
  pinMode(RIGHT_MOTOR_PIN_1, OUTPUT);
  pinMode(RIGHT_MOTOR_PIN_2, OUTPUT);

  // инициализация прерываний
  attachInterrupt(LEFT_ENCODER_INTERRUPT_NB,  callBackInterruptLeftEncoder, CHANGE);
  attachInterrupt(RIGHT_ENCODER_INTERRUPT_NB, callBackInterruptRightEncoder, CHANGE);

  Serial.begin(115200);
}

void loop(){
    // демонстрационное вращение моторами с заданной скоростью и сменой направления по таймеру
    switchMode();
    go(mode);

  unsigned long t = millis() - last_time_pub;

  // проверяем нужно ли менять режим
  if (t > TIME_PUB_PERIOD_MS) {

    // вычисление требуемой скорости вращения колес
    speed_wheel[LEFT] = (linear - WHEEL_BASE * angular);
    speed_wheel[RIGHT]  = (linear + WHEEL_BASE * angular);

    //вычисление текущей скорости вращения колес
    float speed_actual_left = impulse2meters(enc_count[LEFT]) / ((float)t / 1000.0);
    float speed_actual_right = impulse2meters(enc_count[RIGHT]) / ((float)t / 1000.0);

    //вычисление значения для драйвера используя ПИД-регулятор
    float pid_left = linear2driverMotor(speed_wheel[LEFT], speed_actual_left, LEFT);
    float pid_right = linear2driverMotor(speed_wheel[RIGHT], speed_actual_right, RIGHT);

    moveMotor(pid_left, LEFT);
    moveMotor(pid_right, RIGHT);
    
    Serial.print("L_count: ");
    Serial.print(enc_count[LEFT]);
    Serial.print(", R_count: ");
    Serial.println(enc_count[RIGHT]);

    Serial.print("L_meters: ");
    Serial.print(impulse2meters(enc_count[LEFT]));
    Serial.print(", R_meters: ");
    Serial.println(impulse2meters(enc_count[RIGHT]));

    Serial.print("L_speed: ");
    Serial.print(speed_wheel[LEFT]);
    Serial.print(", R_speed: ");
    Serial.println(speed_wheel[RIGHT]);

    Serial.print("L_speed_actual: ");
    Serial.print(speed_actual_left);
    Serial.print(", R_speed_actual: ");
    Serial.println(speed_actual_right);

    Serial.print("L_pid_value: ");
    Serial.print(pid_left);
    Serial.print(", R_pid_value: ");
    Serial.println(pid_right);

    Serial.println("--------------------------------");

    enc_count[LEFT]=0.0;
    enc_count[RIGHT]=0.0;
    
    last_time_pub = millis(); // фиксируем время последней публикации
  }
}

int linear2driverMotor(float linear_speed, float speed_actual, int side)
{
  if (linear_speed == 0) {
    I_prev[i_count][side] = 0.0;
    e_prev[side] = 0.0;
    return 0;
  }

  //Расчет средней скорости движения между публикациями
  float e = -(speed_actual - (linear_speed / K_SPEED)_;          //разница в скорости текущая в m/s и желаемая m/s

  //ПИД регулятор для рассчета значения для драйвера моторов
  float P = Kp * e;
  float I = sumIprev(side) * Ki;
  float D = Kd * (e - e_prev[side]);
  float motor_value = round(P + I + D);

  if (i_count == I_SIZE) {
    i_count = 0;
  }

  I_prev[i_count][side] = I;                  //фиксируем интегральную составляющую
  e_prev[side] = e;                     //фиксируем последнее значение разницы в скорости
  i_count++;

  if (motor_value < 0 && motor_value >= -MOTOR_VALUE_MIN) {
    motor_value = -MOTOR_VALUE_MIN;
  }
  if (motor_value > 0 && motor_value <= MOTOR_VALUE_MIN) {
    motor_value = MOTOR_VALUE_MIN;
  }

  //Убираем переполнение ШИМ
  if (motor_value > MOTOR_VALUE_MAX) {
    return MOTOR_VALUE_MAX;
  }

  if (motor_value < -MOTOR_VALUE_MAX) {
    return -MOTOR_VALUE_MAX;
  }

  return motor_value;
}

// управление мотором на определенной стороне робота
void moveMotor(int value, int side){
  // избавляемся от переполнения ШИМ
  if (value>255)
    value = 255;
  if (value<-255)
    value = -255;

  // определяем направление вращения и передаем значения на драйвер
  if (value>=0) {
    if (value==0){
      // стоп мотор
      analogWrite(side==LEFT ? LEFT_MOTOR_PIN_1 : RIGHT_MOTOR_PIN_1, 0);
      analogWrite(side==LEFT ? LEFT_MOTOR_PIN_2 : RIGHT_MOTOR_PIN_2, 0);
    } else {
      // вращение вперед
      analogWrite(side==LEFT ? LEFT_MOTOR_PIN_1 : RIGHT_MOTOR_PIN_1, abs(value));
      analogWrite(side==LEFT ? LEFT_MOTOR_PIN_2 : RIGHT_MOTOR_PIN_2, 0);
    }
  } else {
    // вращение назад
    analogWrite(side==LEFT ? LEFT_MOTOR_PIN_1 : RIGHT_MOTOR_PIN_1, 0);
    analogWrite(side==LEFT ? LEFT_MOTOR_PIN_2 : RIGHT_MOTOR_PIN_2, abs(value));
  }
}

float getRotationDir(float value){
  if (value>=0) {
    if (value==0){
      return 0.0;
    }
    else
    {
      return 1.0;
    }
  }
  else
  {
    return -1.0;
  }
}

// обработчик прерывания для левого колеса
inline void callBackInterruptLeftEncoder() {
  enc_count[LEFT] += getRotationDir(speed_wheel[LEFT]);
}

// обработчик прерывания для правого колеса
inline void callBackInterruptRightEncoder() {
  enc_count[RIGHT] += getRotationDir(speed_wheel[RIGHT]);
}

// преобразование импульсов в метры
inline float impulse2meters(float x) {
  return (x / WHEEL_IMPULSE_COUNT) * M_PI * WHEEL_DIAMETER;
}

// изменение режима управления по заданному интервалу
void switchMode(){
  // вычисление интервала времени от последней публикации
  unsigned long t = millis() - last_time_switch;

  // проверяем нужно ли менять режим
  if (t > TIME_DEMO_SWITCH_PERIOD_MS) {
    mode++;
    if(mode > ROTATE_RIGHT)
    {
      mode = STOP;
    }
    last_time_switch = millis(); // фиксируем время последней публикации
  }
}

// режимы управления (передача значений драйверу)
void go(int mode){
  switch(mode){
  case MOVE_FRONT:
    angular = 0.0;
    linear = ROBOT_LINEAR;
    break;
  case MOVE_REAR:
    angular = 0.0;
    linear = -ROBOT_LINEAR;
    break;
  case ROTATE_LEFT:
    angular = 2.0;
    linear = 0.0;
    break;
  case ROTATE_RIGHT:
    angular = -2.0;
    linear = 0.0;
    break;
  case STOP:
    angular = 0.0;
    linear = 0.0;
    break;
  }
}

float sumIprev(int side) {
  float sum_i_prev = 0.0;
  for (int i = 0; i < I_SIZE; i++)
  {
    sum_i_prev += I_prev[side][i];
  }
  return sum_i_prev;
}