# Control Arduino para Brazo con 4 ejes mas pinza

El control recibe comando por el Serial, los interpreta y ejecuta en el controlador.

Dichos comandos son una secuencia de texto con el siguiente patron:

```
||accion[&cantidadDeParametros[[parametro1 parametro2 ...]]&||
```
Las acciones definidas son:
 * mover: espera 2 parametros, el ID del eje a mover y el desplazamiento(numero entero positivo o negativo).

Por ejemplo: ``` ||mover&2&idDelServo&desplazamiento||   ```
 * grabar: Sin parametros. Por Ejemplo:  ``` ||grabar&|| ```
 * parar: Sin Parametros. Por ejemplo: ``` ||parar&|| ```
 * reproducir: Sin Parametros. Por ejemplo: ``` ||reproducir&|| ```


