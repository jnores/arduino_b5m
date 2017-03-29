/**
        El siguiente codigo es el control de un brzo dobotico de 4 articulaciones
        con pinza diseñado para la materia PPS1 de la carrera Licenciatura en
        sistemas de la Universidad Nacional de General Sarmiento, Buenos Aires,
        Argentina.

        Se estructura el diseño en 3 etapas:
        1. Lectura de controles y generacion de acciones.
        2. Ejecucion de acciones. Estas acciones estaran reflejadas en fuciones.
        3. Loggin de las acciones ejecutadas.

        Para esto se deben resolver las siguientes cuestiones:
        - Definicion de la estructura de las acciones
        - Definicion del empaquetado de estas acciones para interactuar con la PC
        - Definir paradigma de programacion POO o estructurado.

        @author Nores, José Ezequiel
        @created 2017-03-23
        DONE    0. Programar las acciones de mover, accionGrabar( y reproducir con un poco más de diseño.
        DONE    1. Definir la estructura de las acciones.
                Cada accion se define como una funcion que recibe un char** como parametro
                La asociacion se deberia hacer con un HashMap<string, *fp>. Pero esta en desarrollo
                por ahora se acosia en una funcion disparadora.
        SKIP    2. Modificar el codigo para que efectue una secuencia pregrabada.
        DONE    3. Agregar la posibilidad de recibir la secuencia de acciones por USB
        4. Generar acciones a partir de Joystick PS2.
        5. Generar acciones a partir de potenciometros y botones.
        6. Comprimir el binario para que entre en los 32k del Arduino nano.
        7. Generar un compresor de codigo para minimizar al maximo el binario.
        8. Ver como este compresor podria comprimir tambien las librerias.
*/
///////////////////////////////////////////////////////////////////////////////////////
//////////////////// Agrego las librerias necesarias
///////////////////////////////////////////////////////////////////////////////////////
#include <stdlib.h> // Usar strtol(string, &garbage, base[0-autodetect])
#include <Servo.h>

///////////////////////////////////////////////////////////////////////////////////////
//////////////////// Defino constantes
///////////////////////////////////////////////////////////////////////////////////////
// -- Habilito o desahilito el debug
#define B5M_DEBUG
// #undef B5M_DEBUG

// -- Constantes asociadas al control de servos
#define SERVO_BASE 0
#define SERVO_HOMBRO 1
#define SERVO_CODO 2
#define SERVO_MUNIECA 3
#define SERVO_PINZA 4

#define CANTIDAD_SERVOS 5
#define PIN_SERVOS {4,5,6,7,8}

#define POSICIONES_INICIALES {90,90,90,90,90}
#define POSICION_MAXIMA 200
#define POSICION_MINIMA 30
#define DESPLAZAMIENTO_MAXIMO 8

// -- Constantes asociadas al grabado de secuencia

#define BUFFER_LENGHT 32

// -- Constantes asociadas a Acciones

#define ACCION_MOVER "mover"
#define ACCION_GRABAR "grabar"
#define ACCION_DETENER "detener"
#define ACCION_REPRODUCIR "reproducir"

#define CANTIDAD_ACCIONES 4

///////////////////////////////////////////////////////////////////////////////////////
//////////////////// Declaracion de Tipos de datos
///////////////////////////////////////////////////////////////////////////////////////

typedef void (*fnAccion)(String params[]);

struct ItemAccion {
        String accion;
        fnAccion funcion;
};
///////////////////////////////////////////////////////////////////////////////////////
//////////////////// Declaro las funciones asociadas a las acciones. Se implementan al final.
///////////////////////////////////////////////////////////////////////////////////////
void accionMover(String params[]);
void accionGrabar(String params[]);
void accionDetener(String params[]);
void accionReproducir(String params[]);

///////////////////////////////////////////////////////////////////////////////////////
//////////////////// Declaracion de variables y objetos
///////////////////////////////////////////////////////////////////////////////////////
Servo servos[CANTIDAD_SERVOS];
ItemAccion acciones[CANTIDAD_ACCIONES];

String comando = "";

short pinServos[] = PIN_SERVOS;
short posicionServos[] = POSICIONES_INICIALES;

short secuenciaAutomatica[BUFFER_LENGHT][CANTIDAD_SERVOS];
bool estaReproduciendo = false;
bool estaGrabando = false;

short posicionGrabacion = 0;
short posicionReproduccion = 0;
short duracionSecuencia = 0;

///////////////////////////////////////////////////////////////////////////////////////
//////////////////// DEFINICION DE FUNCIONES OBLIGATORIAS
///////////////////////////////////////////////////////////////////////////////////////

/**
        Inicializamos Variables, clases y configuramos librerias
*/
void setup()
{
        Serial.begin(9600);
        while (!Serial) {
                ; // wait for serial port to connect. Needed for native USB port only
        }
#ifdef B5M_DEBUG
        Serial.println("# INICIADO B5M");
#endif

        iniciarServos();
        iniciarAcciones();

        borrarSecuencia();
        comando.reserve(32);
        delay(500);
}

/**
        Ciclo ifinito de ejecucion.
*/
void loop()
{
        if (estaReproduciendo)
        {
                reproducirPosicion();
        }
        else
        {
                // leerJoystick();
        }
        serialEvent();
}

///////////////////////////////////////////////////////////////////////////////////////
//////////////////// DEFINICION DE FUNCIONES PROPIAS DE SETUP
///////////////////////////////////////////////////////////////////////////////////////

void iniciarServos()
{
        for (int i = 0; i < CANTIDAD_SERVOS; i++)
        {
                servos[i].attach( pinServos[i] );
                servos[i].write( posicionServos[i] );
        }
}

void iniciarAcciones()
{
        acciones[0] = {ACCION_MOVER, &accionMover};
        acciones[1] = {ACCION_GRABAR, &accionGrabar};
        acciones[2] = {ACCION_DETENER, &accionDetener};
        acciones[3] = {ACCION_REPRODUCIR, &accionReproducir};
}

// -- Funciones de grebacion y reproduccion
void borrarSecuencia()
{
        posicionGrabacion = 0;
        duracionSecuencia = 0;
}

// FUNCIONES EXPLICITAS DEL CONTROL DEL BRAZO


// -- Funciones de grebacion y reproduccion

void grabarPosicion()
{
        Serial.print("# GRABAR POSICION: ");
        Serial.println(posicionGrabacion);
        for (int i = 0; i < CANTIDAD_SERVOS; i++)
                secuenciaAutomatica[posicionGrabacion][i] = posicionServos[i];

        posicionGrabacion++;
        if (posicionGrabacion == BUFFER_LENGHT)
                detenerGrabacion();
}

void reproducirPosicion()
{

        for (int i = 0; i < CANTIDAD_SERVOS; i++)
                moverServoA(
                        i,
                        secuenciaAutomatica[posicionReproduccion][i]
                );

        if (posicionReproduccion == 0)
        {
                delay(300);
                Serial.println("# ESPERO 300ms para retomar la posicion inicial");
        }


        posicionReproduccion++;
        if (posicionReproduccion == duracionSecuencia)
                detenerReproduccion();

}

short calcularPosicion(short actual, short &desplazamiento)
{

        if (desplazamiento > DESPLAZAMIENTO_MAXIMO)
                desplazamiento = DESPLAZAMIENTO_MAXIMO;
        else if (-desplazamiento > DESPLAZAMIENTO_MAXIMO)
                desplazamiento = -DESPLAZAMIENTO_MAXIMO;

        actual += desplazamiento;

        if (actual < POSICION_MINIMA)
                actual = POSICION_MINIMA;
        else if (actual > POSICION_MAXIMA)
                actual = POSICION_MAXIMA;

        return actual;
}
void moverServoA(short servoId, short posicion)
{
        servos[servoId].write(posicionServos[servoId]);
        // LOG ACCION
        Serial.print("# MOVER SERVO ");
        Serial.print(servoId);
        Serial.print(" a ");
        Serial.println(posicion);
}

void moverServo(short servoId, short desplazamiento)
{
        if (estaReproduciendo) return;

        posicionServos[servoId] = calcularPosicion(posicionServos[servoId], desplazamiento);

        moverServoA(servoId, posicionServos[servoId]);

        if (estaGrabando)
                grabarPosicion();

        // LOG ACCION
        Serial.print("||");
        Serial.print(ACCION_MOVER);
        Serial.print("&2&");
        Serial.print(servoId);
        Serial.print("&");
        Serial.print(desplazamiento);
        Serial.print("&||");
}

void iniciarGrabacion()
{

        if (estaReproduciendo)
                detenerReproduccion();
        if (estaGrabando)
                return;
        estaGrabando = true;

        // LOG ACCION
        Serial.print("||");
        Serial.print(ACCION_GRABAR);
        Serial.print("&||");
}

void detenerGrabacion()
{
        estaGrabando = false;
        duracionSecuencia = posicionGrabacion;
        posicionGrabacion = 0;

        // LOG ACCION
        Serial.print("||");
        Serial.print(ACCION_DETENER);
        Serial.print("&||");
}

void iniciarReproduccion()
{
        if (estaGrabando)
                detenerGrabacion();
        if (estaReproduciendo)
        {
                Serial.print("# YA esta reproduciento. Termino");
                return;
        }
        if (duracionSecuencia == 0)
        {
                Serial.print("# No hay una secuencia grabada");
                return;
        }

        estaReproduciendo = true;




        // LOG ACCION
        Serial.print("||");
        Serial.print(ACCION_REPRODUCIR);
        Serial.print("&||");
}

void detenerReproduccion()
{
        estaReproduciendo = false;
        posicionReproduccion = 0;

        // LOG ACCION
        Serial.print("||");
        Serial.print(ACCION_DETENER);
        Serial.print("&||");
}

///////////////////////////////////////////////////////////////////////////////////////
//////////////////// <DEFINICION DE FUNCIONES PROPIAS DE LA COMUNICACION POR USB>
///////////////////////////////////////////////////////////////////////////////////////
/**
        Evento disparado dentro del loop. Pero solo si se recivio algo por RX
        se buferea lo recibido empezndo con ||
        se ejecutel comANDO AL LEER &||
*/
void serialEvent()
{
        static char prev1 = 0 , prev2 = 0;

        while (Serial.available())
        {
                char letra = (char) Serial.read();

                if ( letra == '|' and prev1 == '|' ) {
                        if ( prev2 == '&')
                                procesarComando(comando);
                        comando = "";
                }
                else
                        comando += prev2;

                prev2 = prev1;
                prev1 = letra;
        }

}
/**
        Procesa el coomando enviado y lo parsea para poder ejecutar la accion solicitada
        La estructura del comando es:
        ||nombreAccion[&cantidadParametros[&parametro1&parametro2]]
*/
void procesarComando(String comando)
{
#ifdef B5M_DEBUG
        Serial.print("# procesarComando: ");
        Serial.println(comando);
#endif
        String accion;
        String *params = NULL;
        int posAmp1 = comando.indexOf('&');

        if (posAmp1 < 0)
                accion = comando.substring(2);
        else
        {
                accion = comando.substring(2, posAmp1);
                int posAmp2 = comando.indexOf('&', posAmp1 + 1);
                int cantParams = 0;
                if (posAmp2 > posAmp1)
                {
                        String strCantParams = comando.substring(posAmp1 + 1, posAmp2);
                        cantParams = strCantParams.toInt();
                }
                params = new String[cantParams];
                for ( int i = 0; i < cantParams; i++)
                {
                        posAmp1 = posAmp2;
                        posAmp2 = comando.indexOf('&', posAmp1 + 1);

                        if (posAmp2 > posAmp1)
                                params[i] = comando.substring(posAmp1 + 1, posAmp2);
                        else
                                params[i] = comando.substring(posAmp1 + 1);
                }
        }

        ejecutarAccion(accion, params);

        if ( params != NULL )
                delete[] params;
}

/**
        Funcion encargada de ejecutar la funcion asociada a la accion recivida.
        TODO: Esta funcion se debe reemplazar por un map<String, *fp>
*/
void ejecutarAccion(String accion, String params[] )
{
        for (int i = 0; i < CANTIDAD_ACCIONES; i++)
        {
                if ( accion == acciones[i].accion )
                {
                        (*acciones[i].funcion)(params);
                        break;
                }
        }
}

// -- DEFINICION DE ACCIONES " @see typedef fnAccion "

void accionMover(String params[])
{
        short servoId = params[0].toInt();
        short desplazamiento = params[1].toInt();
#ifdef B5M_DEBUG
        Serial.print("# Ejecutando la accion: ");
        Serial.print(ACCION_MOVER);
        Serial.print(" motor_id(" + params[0] + "): ");
        Serial.print(servoId);
        Serial.print(" deslazamiento(" + params[1] + "): ");
        Serial.println(desplazamiento);
#endif
        moverServo(servoId, desplazamiento);
}

void accionGrabar(String params[])
{
#ifdef B5M_DEBUG
        Serial.print("# Ejecutando la accion: ");
        Serial.println(ACCION_GRABAR);
#endif

        iniciarGrabacion();
}

void accionReproducir(String params[])
{
#ifdef B5M_DEBUG
        Serial.print("# Ejecutando la accion: ");
        Serial.println(ACCION_REPRODUCIR);
#endif

        iniciarReproduccion();
}

void accionDetener(String params[])
{
#ifdef B5M_DEBUG
        Serial.print("# Ejecutando la accion: ");
        Serial.println(ACCION_DETENER);
#endif

        detenerGrabacion();
        detenerReproduccion();
}

///////////////////////////////////////////////////////////////////////////////////////
//////////////////// </DEFINICION DE FUNCIONES PROPIAS DE LA COMUNICACION POR USB>
///////////////////////////////////////////////////////////////////////////////////////
