# FruitCat Chaos 3D - Purrallel Arena

Screensaver 3D en C++ y OpenGL para el Proyecto 1 de Computación Paralela y Distribuida de UVG. Simula `N` gatos-fruta dentro de una arena de vidrio: flotan, rebotan, colisionan, acumulan impactos y explotan antes de reaparecer.

La aplicación incluye una ruta **secuencial** y una ruta **paralela con OpenMP**. El renderizado permanece en el hilo principal; OpenMP acelera la actualización de gatos y la detección de colisiones.

## Características

- Arena 3D transparente, terreno, cámara orbitante, iluminación y fondo espacial.
- Gatos-banana y gatos-aguacate como sprites 2D que siempre miran a la cámara (*billboards*).
- Movimiento tridimensional, rebote contra paredes y colisiones esféricas.
- Explosión y reaparición después de siete impactos.
- Contador de FPS, modo de ejecución, número de hilos, total de gatos y gatos activos en pantalla.
- Modo secuencial y paralelo reproducibles con una semilla configurable.
- Validación de argumentos y pruebas automatizadas.

## Requisitos

- Compilador C++ con soporte para C++17 y OpenMP.
- CMake 3.20 o superior.
- GLFW 3 de desarrollo.
- OpenGL.

En Windows este proyecto se ha probado con MinGW-w64, GLFW instalado en `C:\mingw64` y CMake. En Linux o macOS deben instalarse GLFW, OpenGL y un compilador con OpenMP usando el gestor de paquetes correspondiente.

## Compilación

Desde la raíz del repositorio:

```powershell
cmake -S . -B build
cmake --build build
```

Si `build/` ya existe, usa el mismo generador CMake con el que fue creado. Por ejemplo, no combines un directorio configurado con `MinGW Makefiles` con `-G Ninja`.

El ejecutable generado en Windows es:

```text
build\fruitcat-chaos.exe
```

## Ejecución

Primero se puede consultar la ayuda sin abrir una ventana:

```powershell
.\build\fruitcat-chaos.exe --help
```

La interfaz es:

```text
fruitcat-chaos [N] [modo] [hilos] [semilla] [-f|--fullscreen]
```

| Argumento | Descripción |
|---|---|
| `N` | Cantidad de gatos, entre 1 y 2000. Predeterminado: `40`. |
| `modo` | `sequential` o `parallel`. Predeterminado: `sequential`. |
| `hilos` | En `sequential` debe ser `1`; en `parallel` debe estar entre 1 y 256. |
| `semilla` | Entero sin signo para repetir exactamente el estado inicial. Predeterminado: `20260828`. |
| `-f`, `--fullscreen` | Inicia en pantalla completa. |

Ejemplos:

```powershell
# Ejecución secuencial reproducible
.\build\fruitcat-chaos.exe 2000 sequential 1 20260828

# Ejecución paralela con ocho hilos y la misma carga/semilla
.\build\fruitcat-chaos.exe 2000 parallel 8 20260828

# Prueba rápida con pantalla completa
.\build\fruitcat-chaos.exe 100 sequential 1 42 --fullscreen
```

## Controles

| Tecla | Acción |
|---|---|
| `Esc` | Cerrar la aplicación. |
| `F` o `F11` | Alternar pantalla completa. |

La ventana se crea a 1280 x 720, por encima del mínimo de 640 x 480 solicitado.

## Diseño de la simulación

El estado principal es un `std::vector<FruitCat>`. Cada gato posee posición, velocidad, radio de colisión, tipo de fruta, contador de impactos, temporizadores y estado pseudoaleatorio propio.

En cada *frame* se siguen estas fases:

1. Actualizar posiciones, temporizadores y rebotes contra las seis paredes.
2. Detectar pares de esferas que colisionan.
3. Ordenar y resolver los eventos de colisión para modificar posiciones, velocidades e impactos sin condiciones de carrera.
4. Renderizar terreno, gatos, explosiones y arena desde la cámara orbitante.

En `parallel`, la actualización se divide entre hilos con `#pragma omp parallel for`. La detección de colisiones usa un vector de eventos privado por hilo y luego los combina. La resolución final y OpenGL permanecen secuenciales para conservar la seguridad del estado compartido y del contexto gráfico.

## Pruebas

Después de compilar, ejecutar:

```powershell
ctest --test-dir build --output-on-failure
```

Las pruebas verifican la semilla de simulación, el análisis defensivo de argumentos y la disponibilidad/configuración del entorno OpenMP.

## Mediciones de rendimiento

Para comparar las versiones se debe mantener la misma `N` y la misma semilla. Por ejemplo:

```powershell
.\build\fruitcat-chaos.exe 2000 sequential 1 20260828
.\build\fruitcat-chaos.exe 2000 parallel 2 20260828
.\build\fruitcat-chaos.exe 2000 parallel 4 20260828
.\build\fruitcat-chaos.exe 2000 parallel 8 20260828
```

Registrar al menos diez mediciones por configuración para el informe. Con los promedios obtenidos:

```text
speedup = tiempo_secuencial / tiempo_paralelo
eficiencia = speedup / numero_de_hilos
```

## Estructura

```text
assets/        Texturas y sprites PNG.
docs/design/   Decisiones de simulación, física y paralelización.
include/       Interfaces y estructuras compartidas.
src/           Aplicación, renderizado, opciones y simulación.
tests/         Pruebas de argumentos, semilla y OpenMP.
```

## Créditos

Proyecto académico para Computación Paralela y Distribuida, Universidad del Valle de Guatemala.
