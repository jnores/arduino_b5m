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
        --- 4. Generar acciones a partir de Joystick PS2. ---
        --- 5. Generar acciones a partir de potenciometros y botones. ---
        6. Comprimir el binario para que entre en los 32k del Arduino nano.
        --- 7. Generar un compresor de codigo para minimizar al maximo el binario. ---
        --- 8. Ver como este compresor podria comprimir tambien las librerias. ---
        --- 9. Modularizar las acciones de movimiento y grabado/reproduccion de manera
        independiente. Para esto se debe respetar un diagrama de estados. ---
        NO ME SALIO 10. Aplicar la logica de estados Iniciando -> Preparado <-> [Grabando/Reproduciendo]
*/
///////////////////////////////////////////////////////////////////////////////////////
//////////////////// Agrego las librerias necesarias
///////////////////////////////////////////////////////////////////////////////////////
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
#define POSICION_MAXIMA 180
#define POSICION_MINIMA 0
#define DESPLAZAMIENTO_MAXIMO 8

// -- Constantes asociadas al grabado de secuencia

#define BUFFER_LENGHT 32

// -- Constantes asociadas a Acciones

#define ACCION_MOVER "mover"
#define ACCION_GRABAR "grabar"
#define ACCION_DETENER "detener"
#define ACCION_REPRODUCIR "reproducir"

#define CANTIDAD_ACCIONES 4

#define ESTADO_PREPARADO 0
#define ESTADO_GRABANDO 1
#define ESTADO_REPRODUCIENDO 2

#define CANTIDAD_ESTADOS 3

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))


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

//ItemAccion** accionesPorEstado[CANTIDAD_ESTADOS];
ItemAccion accionesDisponibles[CANTIDAD_ACCIONES];
String comando = "";

short pinServos[] = PIN_SERVOS;
short posicionServos[] = POSICIONES_INICIALES;



short secuenciaAutomatica[BUFFER_LENGHT][CANTIDAD_SERVOS];

short posicionGrabacion = 0;
short posicionReproduccion = 0;
short duracionSecuencia = 0;
// graba y reproduce la posicon cada medio segundo
unsigned long recordInterval = 500;
// registra el tiempo de la ultima grabacion/reproduccion para respetar el
// intervalo configurado.
unsigned long timeLastRecodrReplay = 0;


short estadoActual=-1;

////////////////////////////////////////////////////////////////////////////////
//////////////////// DEFINICION DE FUNCIONES OBLIGATORIAS
////////////////////////////////////////////////////////////////////////////////

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
        estadoActual=ESTADO_PREPARADO;
}

/**
        Ciclo infinito de ejecucion.
*/
void loop()
{
        serialEvent();

        if (estadoActual == ESTADO_REPRODUCIENDO)
        {
                reproducirPosicion();
        }
        else if(estadoActual == ESTADO_GRABANDO)
        {
                grabarPosicion();
        }

        locateServos();
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

void locateServos()
{
        for (int i = 0; i < CANTIDAD_SERVOS; i++)
        {
                moverServoA(i, posicionServos[i] );
        }
}
/**
 * [iniciarAcciones description]
 * Aqui se inicializa el arreglo bidimencional de acciones asociado al los cada
 * estado posible del controlador.
 */
void iniciarAcciones()
{
        accionesDisponibles[0] = {ACCION_MOVER, &accionMover};
        accionesDisponibles[1] = {ACCION_GRABAR, &accionGrabar};
        accionesDisponibles[2] = {ACCION_REPRODUCIR, &accionReproducir};
        accionesDisponibles[3] = {ACCION_DETENER, &accionDetener};
//         ItemAccion* accionesPreparado = new ItemAccion[3];
//
//         accionesPreparado[0] = {ACCION_MOVER, &accionMover};
//         accionesPreparado[1] = {ACCION_GRABAR, &accionGrabar};
//         accionesPreparado[2] = {ACCION_REPRODUCIR, &accionReproducir};
//
//         accionesPorEstado[ESTADO_PREPARADO] = &accionesPreparado;

        // accionesPorEstado[ESTADO_GRABANDO][0] = {ACCION_MOVER, &accionMover};
        // accionesPorEstado[ESTADO_GRABANDO][1] = {ACCION_DETENER, &accionDetener};

        // accionesPorEstado[ESTADO_REPRODUCIENDO][0] = {ACCION_DETENER, &accionDetener};

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
        if (millis() - timeLastRecodrReplay > recordInterval)
        {
#ifdef B5M_DEBUG
                Serial.print("# GRABAR POSICION: ");
                Serial.println(posicionGrabacion);
#endif
                for (int i = 0; i < CANTIDAD_SERVOS; i++)
                        secuenciaAutomatica[posicionGrabacion][i] = posicionServos[i];

                timeLastRecodrReplay = millis();

                duracionSecuencia = ++posicionGrabacion;
                if (posicionGrabacion == BUFFER_LENGHT)
                        detenerGrabacion();
        }
}

void reproducirPosicion()
{
        if (millis() - timeLastRecodrReplay > recordInterval)
        {
                for (int i = 0; i < CANTIDAD_SERVOS; i++)
                        posicionServos[i] = secuenciaAutomatica[posicionReproduccion][i];

                timeLastRecodrReplay = millis();

                posicionReproduccion++;
                if (posicionReproduccion == duracionSecuencia)
                        detenerReproduccion();

        }
}

/**
 * Se enarga de realizar el control del desplazamiento, respetando el
 * desplazamiento maximo configurado y de verificar los limites de movimiento
 * del servo.
 * Devuelve una posicion valida que respete el desplazamiento maximo..
 *
 * @param  actual         posicion actual que se desea desplazar.
 * @param  desplazamiento Se pasa por referencia para poder actualizar el estado
 * si este exede los limites configurados.
 * @return                posicion a la que debe desplazarce el servo,
 * respetando los limites y la distancia de desplazamiento maxima configurada.
 */
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
        delay(5);
        // LOG ACCION
#ifdef B5M_DEBUG
        if (millis()%recordInterval == 0 )
        {
                Serial.print("# MOVER SERVO ");
                Serial.print(servoId);
                Serial.print(" a ");
                Serial.println(posicion);
        }
#endif
}

void moverServo(short servoId, short desplazamiento)
{
        if (estadoActual != ESTADO_REPRODUCIENDO)
        {
                posicionServos[servoId] = calcularPosicion(posicionServos[servoId], desplazamiento);
                moverServoA(servoId, posicionServos[servoId]);
        }
}

void iniciarGrabacion()
{
        if (estadoActual != ESTADO_GRABANDO)
        {
                if (estadoActual == ESTADO_REPRODUCIENDO)
                        detenerReproduccion();
                estadoActual = ESTADO_GRABANDO;
        }
}

void detenerGrabacion()
{
        posicionGrabacion = 0;
        estadoActual=ESTADO_PREPARADO;

}

void iniciarReproduccion()
{
        if (estadoActual != ESTADO_REPRODUCIENDO)
        {
                if (estadoActual == ESTADO_GRABANDO)
                        detenerGrabacion();
                estadoActual = ESTADO_REPRODUCIENDO;
        }

}

void detenerReproduccion()
{
        posicionReproduccion = 0;
        estadoActual=ESTADO_PREPARADO;

}

void detener()
{
        switch(estadoActual)
        {
        case ESTADO_GRABANDO:
                detenerGrabacion();
                break;
        case ESTADO_REPRODUCIENDO:
                detenerReproduccion();
                break;
        }
}

///////////////////////////////////////////////////////////////////////////////////////
////////////////// <DEFINICION DE FUNCIONES PROPIAS DE LA COMUNICACION POR USB>
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
        Procesa el comando enviado y lo parsea para poder ejecutar la accion solicitada
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

        if (ejecutarAccion(accion, params))
        {
                Serial.println("#Comando Ejecutada:");
                Serial.print(comando);
                Serial.println("&||");
        }
#ifdef B5M_DEBUG
        else
        {
                Serial.println("");
                Serial.print("#Comando DESCARTADO: ");
                Serial.print(comando);
                Serial.println("&||");
        }
#endif

        if ( params != NULL )
                delete[] params;
}

/**
        Funcion encargada de ejecutar la funcion asociada a la accion recivida.
        TODO: Esta funcion se debe reemplazar por un map<String, *fp>
*/
bool ejecutarAccion(String accion, String params[] )
{
    if (NULL != accionesDisponibles)
    {
        for (int i = 0; i < NELEMS(accionesDisponibles); i++)
        {
                if ( accion == accionesDisponibles[i].accion )
                {
                        // (*(*accionesPorEstado[estadoActual])[i].funcion)(params);
                        (* accionesDisponibles[i].funcion)(params);
                        return true;
                }
        }
    }
    return false;
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
        Serial.print(" desplazamiento(" + params[1] + "): ");
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

        detener();
}

///////////////////////////////////////////////////////////////////////////////////////
//////////////////// </DEFINICION DE FUNCIONES PROPIAS DE LA COMUNICACION POR USB>
///////////////////////////////////////////////////////////////////////////////////////
