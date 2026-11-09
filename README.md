# Monitor analógico con alarma configurable — STM32F302R8

**Asignatura:** Sistemas digitales · Code Challenge integrador GPIO / TIMER / UART / ADC  
**Estudiante:** jOSMIL GOMEZ · **Matrícula:** 2022-0522 
**Hardware:** placa NUCLEO-F302R8 (STM32F302R8Tx, Cortex-M4)  
**Herramientas:** STM32CubeIDE, STM32CubeMX, firmware STM32CubeF3 , Tera Term , Git/GitHub.

## Objetivo y funcionamiento  

El potenciómetro simula una variable analógica. El STM32 muestrea ADC1 cada 100 ms, calcula un porcentaje entero, actualiza una alarma con histéresis y admite modo RUN/PAUSE mediante un botón. TIM6 genera la base de 1 ms, el LED de actividad cambia de estado cada 500 ms y USART2 envía telemetría cada 1000 ms y procesa comandos ASCII. Todo se ejecuta en un mismo firmware sin retardos bloqueantes para programar las tareas.

## Materiales, conexiones y esquema

- NUCLEO-F302R8, potenciómetro de 10 kΩ, LED rojo, resistencia de 330 Ω, cables y protoboard, USB ST-LINK y computadora.
- LED de actividad LD2 y pulsador B1 (USER) integrados en la placa.

| Función | Pin MCU | Conexión | Ajustes |
|---|---|---|---|
| Entrada analógica | PA0 / A0 | Cursor (pata central) del potenciómetro | `ADC1_IN1`, single-ended |
| Alimentación potenciómetro | 3V3 y GND | Extremos laterales del potenciómetro | Nunca conectar a 5 V |
| LED de actividad | PB13 | LED LD2 integrado | GPIO salida push-pull |
| LED de alarma | PA8 / D7 | PA8 → resistor 330 Ω → ánodo LED; cátodo → GND | GPIO salida push-pull |
| Pulsador | PC13 | Botón USER integrado | GPIO entrada, activo en bajo, pull-up |
| USART2 TX | PA2 | ST-LINK Virtual COM Port | 115200 baudios |
| USART2 RX | PA3 | ST-LINK Virtual COM Port | 8N1 |

**Esquema de potenciómetro:** `3V3 ─ [extremo de 10 kΩ] ─ (cursor → PA0) ─ [otro extremo] ─ GND`.

Si se utiliza un adaptador UART externo en vez del puerto virtual USB, debe funcionar con niveles de **3,3 V**, compartir GND con la placa y cruzar TX/RX. El puerto virtual del ST-LINK ya está integrado en la placa.

## ADC

- Periférico/canal: **ADC1, IN1 (PA0)**; lectura de una conversión regular, inicio por software.
- Resolución: **N = 12 bits**; rango de lectura: **0–4095**.
- Referencia analógica: **VREF+ = VDDA ≈ 3,3 V nominal**. Rango de tensión de entrada: **0 a VDDA** (medir VDDA para mayor precisión).
- Tiempo de muestreo: **61,5 ciclos**; calibración ADC al iniciar.
- Conversión a entero con redondeo y sin desbordamiento: `PCT = (100 × ADC + 2047) / 4095`, usando multiplicación en `uint32_t`.

## TIMER y planificación

Según la configuración de CubeMX, `SYSCLK = HCLK = 64 MHz`, `APB1 / 2` → `PCLK1 = 32 MHz`; al dividir APB1, el reloj de TIM6 es `2 × PCLK1 = 64 MHz`.

`TIM6`: **prescaler PSC = 63**, **auto-reload ARR = 999**.

```
T = (PSC + 1) × (ARR + 1) / f_TIM6
  = 64 × 1000 / 64 000 000 = 0,001 s = 1 ms
```

En la interrupción `HAL_TIM_PeriodElapsedCallback()` solo se incrementa `tiempo_ms`. El bucle principal atiende:

| Evento | Intervalo |
|---|---:|
| Comprobación de entrada y antirrebote | continuamente; estabilidad ≥30 ms |
| Lectura ADC | 100 ms |
| Cambio de LED de actividad | 500 ms |
| Telemetría UART | 1000 ms |

Sin `HAL_Delay()` para la programación de tareas ni para el botón.

## Alarma y modos

Al reiniciar: `MODE=RUN`, `THR=70`, `ALARM=0`, tiempo interno en cero. La primera muestra válida determina el estado inicial de alarma.

- En **RUN**, la alarma enciende con `PCT >= THR` y apaga con `PCT < THR - 5`. En la banda intermedia conserva el estado.
- En **PAUSE**, el LED rojo se apaga; ADC, LED de actividad, UART y contador continúan trabajando.
- El botón B1 alterna RUN y PAUSE tras una pulsación estable de al menos 30 ms; mantenerlo presionado no repite la acción y se debe soltar para habilitar otra pulsación.
- Al pasar a RUN o cambiar THR estando en RUN, la alarma se decide inmediatamente según la **última** muestra: encendida solo si `PCT >= THR`; las muestras siguientes usan histéresis normal.

## UART: terminal y comandos

Abrir el puerto **COM correspondiente a ST-LINK** (en las pruebas iniciales fue COM3), configurar **115200, 8N1, sin control de flujo**. En Tera Term: transmitir nueva línea como `LF` o `CR+LF`. Respuestas terminan siempre en `CRLF`.

| Comando literal | Resultado |
|---|---|
| `STATUS` | Línea de estado inmediata |
| `THR n` | Umbral entero decimal entre 10 y 95, respuesta `OK THR=n` |
| `START` | Modo RUN, respuesta `OK RUN` |
| `STOP` | Modo PAUSE, respuesta `OK PAUSE` |
| Comando inválido o fuera de rango | `ERR` sin cambiar parámetros |

La recepción distingue mayúsculas, acepta líneas de hasta 32 caracteres sin LF/CRLF, ignora líneas vacías y descarta por completo líneas demasiado largas hasta LF. Se usa una cola para transmitir mensajes completos sin mezclarlos. No se utilizan retardos bloqueantes para recibir/transmitir.

**Ejemplo del formato** (los valores ilustrativos varían con la entrada):

```text
t=1200;ADC=2866;PCT=70;THR=70;MODE=RUN;ALARM=1
```

## Reproducir, compilar y cargar

1. Abrir STM32CubeIDE e importar la carpeta raíz mediante **File → Import → Existing Projects into Workspace**. Seleccionar la carpeta del repositorio.
2. Seleccionar el proyecto y ejecutar **Project → Build Project** (`Ctrl+B`).
3. Conectar la NUCLEO-F302R8 por USB ST-LINK, elegir la configuración de ejecución cuyo `.elf` pertenece al proyecto importado y pulsar **Run**.
4. Abrir Tera Term con los parámetros anteriores, mover el potenciómetro y probar los comandos.
5. Consultar `RESULTADOS_P01_P10.md` y completar los valores medidos y referencias a evidencias.

**Nota sobre el nombre heredado:** el archivo `.ioc` y ciertos metadatos del proyecto aún conservan el nombre `Inventario RFID`, porque se reutilizó un proyecto STM32CubeMX ya funcional. **El firmware de `Core/Src/main.c` implementa el monitor analógico y no el inventario RFID.** Si se regenera desde `.ioc`, se debe comprobar que las modificaciones manuales de `main.c` se mantengan o integren; no sobrescribirlo sin respaldo.

## Verificación y trazabilidad

- Pruebas obligatorias: **P01–P10**, registradas en `RESULTADOS_P01_P10.md`.
- Para la prueba temporal se requieren **marcas de tiempo** y analizador lógico, osciloscopio o trazas de un simulador apropiado. Las marcas UART ayudan a verificar telemetría, pero **no prueban por sí solas** cada conversión ADC ni todos los cambios del LED. No declarar una prueba aprobada sin evidencia suficiente.
- Historial GitHub: commits sucesivos de configuración, ADC, alarma/botón, UART y verificación/documentación. La versión final se identifica por su **hash Git** o una etiqueta.
- No se utiliza PWM, DMA, RTOS, pantalla ni sensores adicionales.
