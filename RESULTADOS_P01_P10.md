# Resultados de las pruebas P01 a P10

**Placa:** NUCLEO-F302R8 · **Proyecto:** Monitor analógico con alarma configurable  
**Nombre y matrícula:** JOSMIL GOMEZ 2022-0522 · **Fecha de ejecución:** 8/11/2026 

| ID | Procedimiento breve | Resultado esperado | Observado (rellenar) | Estado | Evidencia (archivo / instante) |
|---|---|---|---|---|---|
| P01 | Reiniciar con potenciómetro al mínimo; STATUS después de 200 ms | RUN, THR=70, ALARM=0, PCT≈0 | — | PENDIENTE | — |
| P02 | Registrar 10 s y 10 telemetrías; medir ADC/LED/TX | ADC 100 ms, LED 500 ms, TX 1000 ms, ±5 % | — | PENDIENTE | — |
| P03 | Aplicar ≈0 %, 50 %, 100 % VREF y medir voltajes | PCT ±3 puntos; ADC 0–4095; redondeo correcto | Se observó variar ADC ≈0–4095 y PCT 0–100 % en pruebas preliminares; faltan voltajes y 50 % | PENDIENTE | Captura preliminar de terminal; agregar nombre del archivo |
| P04 | THR 70; PCT 60→72→67→62 | ALARM 0→1→1→0 | — | PENDIENTE | — |
| P05 | Cinco pulsaciones, una sostenida 2 s; rebotes si posible | Una alternancia por pulsación, ninguna repetición | — | PENDIENTE | — |
| P06 | PCT≈80; STOP, cambiar potenciómetro, STATUS, START | PAUSE: alarma 0 y tareas siguen; START: RUN/alarma 1 | — | PENDIENTE | — |
| P07 | RUN PCT≈60; THR 50, THR 80, THR 10, THR 95 | Todos OK; con 50 enciende y con 80 apaga | — | PENDIENTE | — |
| P08 | THR 9, THR 96, THR abc, THR 50x, THR 50 extra, desconocido | Cada línea ERR; parámetros inalterados | — | PENDIENTE | — |
| P09 | STATUS fragmentado; LF/CRLF; 33 caracteres + LF, luego STATUS | Se recupera tras ERR, sin ejecutar fragmentos | — | PENDIENTE | — |
| P10 | 30 s; STATUS 5/s, variar entrada y alternar RUN/PAUSE | Respuestas ≤100 ms, telemetría y tiempos ±5 %, sin líneas corruptas | — | PENDIENTE | — |

## Mediciones representativas (rellenar)

| Medida | Valor obtenido | Instrumento / método | Tolerancia | Estado |
|---|---|---|---|---|
| Período ADC | — ms | — | 95–105 ms | PENDIENTE |
| Cambio de LED actividad | — ms | — | 475–525 ms | PENDIENTE |
| Inicio de telemetría | — ms | — | 950–1050 ms | PENDIENTE |
| Latencia máxima UART STATUS bajo P10 | — ms | — | ≤100 ms | PENDIENTE |
| Vin≈0 % / lectura ADC / PCT | — V / — / — % | multímetro + terminal | ±3 puntos | PENDIENTE |
| Vin≈50 % / lectura ADC / PCT | — V / — / — % | multímetro + terminal | ±3 puntos | PENDIENTE |
| Vin≈100 % / lectura ADC / PCT | — V / — / — % | multímetro + terminal | ±3 puntos | PENDIENTE |

## Secuencia rápida de pruebas seriales

Configurar **115200 8N1** y envío de nueva línea en **LF o CR+LF**. Registrar texto de la terminal con timestamps, screenshots o archivo log. No usar un puerto COM simultáneamente en dos programas.

```text
STATUS
THR 50
THR 80
THR 10
THR 95
THR 9
THR 96
THR abc
THR 50x
THR 50 extra
DESCONOCIDO
STOP
STATUS
START
STATUS
```

**P09:** enviar primero `STAT` sin fin de línea y después `US` con LF. Comprobar una sola respuesta. Luego enviar 33 letras `X` y LF, seguido de `STATUS` y LF. El primer envío debe producir `ERR`, el segundo una línea de estado.

**P10:** enviar `STATUS` a intervalos de 200 ms durante 30 segundos; variar el potenciómetro y alternar RUN/PAUSE con botón. Registrar timestamps de recepción, el inicio de cada línea, las respuestas y la latencia de 100 ms como máximo. Si no hay medición de latencia de envío/recepción, no declarar PASS de P10.

## Evidencias y observaciones

- E01: [captura de terminal tras reinicio]
- E02: [log de diez líneas con timestamps y medición temporal]
- E03: [capturas / voltajes en extremos y mitad de potenciómetro]
- E04: [video LED alarma durante histéresis]
- E05: [video botón RUN/PAUSE, pulsación sostenida]
- E06–E09: [logs Tera Term de comandos y respuestas]
- E10: [traza y valores de latencia de la prueba de carga]

**Incidencias, resultados FAIL y explicaciones:**

[Registrar aquí los fallos efectivamente observados, cambios efectuados y las pruebas repetidas. No inventar resultados.]

**Versión final y enlace:**

- Repositorio: https://github.com/SupremeKnigth/Monitor-Analogico-Alarma-STM32
- Commit final: [COMPLETAR]
