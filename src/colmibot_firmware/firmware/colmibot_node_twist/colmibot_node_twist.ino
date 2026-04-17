#include <micro_ros_arduino.h> // Incluye la librería principal para micro-ROS en entornos Arduino.

#include <stdio.h> // Incluye la librería estándar de entrada/salida.
#include <rcl/rcl.h> // Incluye la librería del cliente ROS (ROS Client Library).
#include <rcl/error_handling.h> // Incluye funciones para el manejo de errores de RCL.
#include <rclc/rclc.h> // Incluye la librería del cliente ROS C (ROS Client Library C).
#include <rclc/executor.h> // Incluye la librería para el ejecutor de ROS C.

#include <geometry_msgs/msg/twist.h> // Incluye la definición del tipo de mensaje Twist de geometry_msgs. Este mensaje se usa comúnmente para enviar comandos de velocidad.

rcl_subscription_t subscriber; // Declara un objeto suscriptor de ROS.
geometry_msgs__msg__Twist msg; // Declara una variable del tipo de mensaje Twist.
rclc_executor_t executor; // Declara un objeto ejecutor de ROS.
rcl_allocator_t allocator; // Declara un objeto asignador de memoria.
rclc_support_t support; // Declara un objeto de soporte para ROS.
rcl_node_t node; // Declara un objeto nodo de ROS.

// ------------- Pines ----------------

// Motores
#define LMot_dir 32       // Dir1 Cytron
#define LMot_pwm 33      // PWM1 Cytron
#define RMot_dir 25      // Dir2 Cytron 
#define RMot_pwm 26      // PWM2 Cytron

// Macro para verificar el código de retorno de una función de ROS. Si no es RCL_RET_OK, llama a error_loop().
#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();}}
// Macro para verificar el código de retorno de una función de ROS. Si no es RCL_RET_OK, llama a error_loop() (similar a RCCHECK en este caso).
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();}}

// Función que se ejecuta en caso de un error crítico de ROS.
void error_loop(){
  while(1){ // Bucle infinito.
    
    delay(100); // Pequeña pausa para que el parpadeo sea visible.
  }
}

// Función de callback para el mensaje Twist. Se llama cada vez que se recibe un mensaje en el tópico suscrito.
void subscription_callback(const void *msgin) {
  // Convierte el puntero genérico a un puntero del tipo de mensaje Twist.
  const geometry_msgs__msg__Twist * msg = (const geometry_msgs__msg__Twist *)msgin;
  // Si la velocidad lineal en el eje X es 0, apaga el LED (LOW); de lo contrario, enciende el LED (HIGH).
  //digitalWrite(LED_PIN, (msg->linear.x > 1) ? LOW : HIGH);
  moveRobot(msg->linear.x,msg->angular.z);
}


// ------------- Setup ----------------
#include <Arduino.h>
void setup() {
  Serial.begin(115200);
  delay(2000);

  set_microros_serial_transports(Serial);
  
  // Configuración de pines como salida o entrada
  pinMode(LMot_dir, OUTPUT);
  pinMode(RMot_dir, OUTPUT);
  
  allocator = rcl_get_default_allocator(); // Obtiene el asignador de memoria por defecto.

   // Crea las opciones de inicialización para el soporte de ROS.
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  // Crea el nodo de ROS.
  RCCHECK(rclc_node_init_default(&node, "colmibot_robot_node", "", &support));

  // Crea el suscriptor.
  RCCHECK(rclc_subscription_init_default(
    &subscriber, // Referencia al objeto suscriptor.
    &node, // Referencia al nodo.
    ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist), // Obtiene el tipo de soporte del mensaje Twist.
    "colmibot/cmd_vel")); // Nombre del tópico al que se suscribirá.

  // Crea el ejecutor.
  RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator)); // Inicializa el ejecutor con un handle.
  // Añade el suscriptor al ejecutor, asociándolo con la función de callback y el modo ON_NEW_DATA (llama al callback solo cuando hay datos nuevos).
  RCCHECK(rclc_executor_add_subscription(&executor, &subscriber, &msg, &subscription_callback, ON_NEW_DATA));

}

// ------------- Function ----------------

void moveRobot(float linealVel, float angularVel) {
  float WS = 0.2;        // Wheel_Separation (m) - 20 cm
  
  float max_vel = 0.5;     // velocidad máxima esperada (m/s) 0.52
  // 165 RPM = 17.28 rad/s
  // v = omega * wheel_radius = 17.28 * 0.03
  // v = 0.5283 m/s

  // Cinemática
  float v_left  = linealVel - (WS / 2.0) * angularVel;
  float v_right = linealVel + (WS / 2.0) * angularVel;

  // Normalización de la velocidad [-1, 1] - El escalamiento en el archivo joy_teleop.yaml cambia
  v_left  = v_left  / max_vel;
  v_right = v_right / max_vel;

  // Restringir velocidad
  v_left  = constrain(v_left,  -1.0, 1.0);            // Restringuiendo límites para rueda izquierda para no salturarla
  v_right = constrain(v_right, -1.0, 1.0);            // Restringuiendo límites para rueda derecha para no salturarla

  int pwm_left  = (int)(v_left  * 255.0);         // Se escala para tener el valor de PWM de la rueda izquierda
  int pwm_right = (int)(v_right * 255.0);         // Se escala para tener el valor de PWM de la rueda derecha
  

// Condiciones para establecer los 4 movimientos del robot
  if (pwm_left >= 0) {
    digitalWrite(LMot_dir, LOW);
  } else {
    digitalWrite(LMot_dir, HIGH);
  }

  if (pwm_right >= 0) {
    digitalWrite(RMot_dir, LOW);
  } else {
    digitalWrite(RMot_dir, HIGH);
  }

  // Aplicar señales PWM a los motores
  analogWrite(LMot_pwm, abs(pwm_left));
  analogWrite(RMot_pwm, abs(pwm_right));
}

// ------------- Loop ----------------

void loop() {
  // Hace girar el ejecutor para procesar eventos (como la recepción de mensajes) por 5 ms.
  RCCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(5)));
}

