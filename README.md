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

- C++17.
- CMake 3.20 o superior.
- Compilador con soporte para OpenMP.
- GLFW.
- OpenGL.
- Ninja como generador recomendado; puede utilizarse otro generador compatible con CMake.

En Windows este proyecto se ha probado con MinGW-w64 y CMake. En Linux o macOS deben instalarse GLFW, OpenGL y un compilador con OpenMP usando el gestor de paquetes correspondiente.

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
fruitcat-chaos N benchmark HILOS SEMILLA FRAMES REPETICIONES ARCHIVO_CSV
```

| Argumento | Descripción |
|---|---|
| `N` | Cantidad de gatos, entre 1 y 2000. Predeterminado: `40`. |
| `modo` | `sequential`, `parallel` o `benchmark`. Predeterminado: `sequential`. |
| `hilos` | En `sequential` debe ser `1`; en `parallel` y `benchmark` debe estar entre 1 y 256. |
| `semilla` | Entero sin signo para repetir exactamente el estado inicial. Predeterminado: `20260828`. |
| `frames` | En `benchmark`, cantidad positiva de frames por medición. |
| `repeticiones` | En `benchmark`, cantidad de mediciones; debe ser al menos `10`. |
| `archivo CSV` | En `benchmark`, ruta donde se guardan las mediciones y su promedio. |
| `-f`, `--fullscreen` | Inicia en pantalla completa; no se admite en `benchmark`. |

Ejemplos:

```powershell
# Ejecución secuencial reproducible
.\build\fruitcat-chaos.exe 2000 sequential 1 20260828

# Ejecución paralela con ocho hilos y la misma carga/semilla
.\build\fruitcat-chaos.exe 2000 parallel 8 20260828

# Benchmark de simulación sin renderizado y con salida CSV
.\build\fruitcat-chaos.exe 2000 benchmark 8 20260828 120 10 resultados.csv

# Prueba rápida con pantalla completa
.\build\fruitcat-chaos.exe 100 sequential 1 42 --fullscreen
```

El modo `benchmark` no admite pantalla completa ni abre una ventana. Requiere al menos diez repeticiones y guarda cada medición, junto con la fila de promedio, en el archivo CSV indicado.

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

En `parallel`, el movimiento y los rebotes se distribuyen con `schedule(static)`, ya que el trabajo por gato es similar. La barrera implícita al final de esta fase garantiza que todos los movimientos terminen antes de detectar colisiones. La detección reparte los primeros índices de los pares con `schedule(static, 1)` y almacena los eventos en un vector local por hilo. Después, los vectores se combinan y los eventos se ordenan determinísticamente.

Cada gato mantiene su propio estado pseudoaleatorio, por lo que su secuencia no depende del hilo que lo actualice. No se necesitan regiones `critical`, operaciones `atomic` ni mutex durante la detección: cada hilo escribe exclusivamente en su propio vector. La revalidación y resolución final de los eventos permanece secuencial, al igual que OpenGL, para modificar el estado compartido en un orden estable y conservar la seguridad del contexto gráfico.

## Reproducibilidad

Con la misma `N`, semilla y cantidad de frames, las rutas secuencial y paralela producen estados equivalentes. Esta equivalencia se conserva al ejecutar la ruta paralela con 1, 2, 4 u 8 hilos, porque la generación pseudoaleatoria pertenece a cada gato y los eventos de colisión se ordenan antes de resolverlos.

## Pruebas

Después de compilar, ejecutar:

```powershell
ctest --test-dir build --output-on-failure
```

Las cuatro pruebas verifican:

1. Reproducibilidad y equivalencia de la simulación.
2. Validación defensiva de argumentos.
3. Runtime y cantidad de hilos OpenMP.
4. Benchmark sin ventana y estructura del CSV.

## Mediciones de rendimiento

Las mediciones oficiales usan el modo `benchmark` sin renderizado y `std::chrono::steady_clock`. Cada configuración mide 120 frames durante 10 repeticiones. Ambas rutas comienzan con las mismas semillas derivadas y el orden secuencial/paralelo se alterna entre repeticiones para reducir sesgos transitorios.

El intervalo medido incluye únicamente las actualizaciones de simulación. Excluye construcción, renderizado, salida de consola, comparación de equivalencia y escritura del CSV. Las métricas finales utilizan los promedios de tiempo:

```text
speedup = tiempo_secuencial_promedio / tiempo_paralelo_promedio
eficiencia_porcentaje = (speedup / numero_de_hilos) × 100
```

Los FPS informados por este benchmark corresponden únicamente a la simulación, sin renderizado.

### Resultados oficiales

| N | Hilos | Secuencial ms/frame | Paralelo ms/frame | FPS secuencial | FPS paralelo | Speedup | Eficiencia |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 500 | 1 | 0.2491 | 0.2430 | 4014.86 | 4114.46 | 1.025x | 102.48% |
| 500 | 2 | 0.2578 | 0.2050 | 3878.57 | 4877.40 | 1.258x | 62.88% |
| 500 | 4 | 0.2540 | 0.1993 | 3936.24 | 5017.75 | 1.275x | 31.87% |
| 500 | 8 | 0.2714 | 0.2490 | 3683.94 | 4015.67 | 1.090x | 13.63% |
| 1000 | 1 | 1.1147 | 1.0793 | 897.12 | 926.52 | 1.033x | 103.28% |
| 1000 | 2 | 1.1047 | 0.6175 | 905.18 | 1619.53 | 1.789x | 89.46% |
| 1000 | 4 | 1.1595 | 0.5710 | 862.42 | 1751.43 | 2.031x | 50.77% |
| 1000 | 8 | 1.1183 | 0.4981 | 894.24 | 2007.78 | 2.245x | 28.07% |
| 2000 | 1 | 4.6125 | 4.1441 | 216.80 | 241.31 | 1.113x | 111.30% |
| 2000 | 2 | 6.4522 | 3.2718 | 154.99 | 305.64 | 1.972x | 98.60% |
| 2000 | 4 | 6.3738 | 2.4794 | 156.89 | 403.32 | 2.571x | 64.27% |
| 2000 | 8 | 6.5077 | 2.0571 | 153.66 | 486.13 | 3.164x | 39.55% |

Con `N=500`, el mejor speedup fue aproximadamente `1.275x` con 4 hilos; usar 8 hilos agregó costo de coordinación para una carga pequeña. Con `N=1000`, el resultado llegó a aproximadamente `2.245x` con 8 hilos, y con `N=2000` alcanzó aproximadamente `3.164x` con 8 hilos. La mejor eficiencia usando más de un hilo fue aproximadamente `98.60%`, con `N=2000` y 2 hilos. Los valores superiores al 100% con un hilo reflejan variación de medición y diferencias entre rutas, no escalamiento real.

Los archivos completos pueden consultarse en:

- [Resumen oficial del benchmark](resultados/benchmark-oficial/resumen_benchmark.csv)
- [Entorno del equipo](resultados/benchmark-oficial/entorno_equipo.txt)

## Estructura

```text
assets/        Texturas y sprites PNG.
docs/design/   Decisiones de simulación, física y paralelización.
include/       Interfaces y estructuras compartidas.
include/benchmark/   Interfaz del benchmark.
src/           Aplicación, renderizado, opciones y simulación.
src/benchmark/       Medición y exportación CSV.
scripts/             Automatización de mediciones oficiales.
resultados/          CSV y salidas oficiales.
tests/         Pruebas de argumentos, semilla y OpenMP.
```

## Limitaciones

- El renderizado se mantiene secuencial en el hilo principal.
- La revalidación y resolución final de colisiones es secuencial.
- La eficiencia disminuye cuando la carga es pequeña frente al costo de coordinación de los hilos.
- Los tiempos, FPS, speedup y eficiencia dependen del hardware y de la carga del sistema durante la medición.

## Referencias

- [OpenMP](https://www.openmp.org/)
- [CMake: FindOpenMP](https://cmake.org/cmake/help/latest/module/FindOpenMP.html)
- [GLFW](https://www.glfw.org/)

## Créditos

Proyecto académico para Computación Paralela y Distribuida, Universidad del Valle de Guatemala.
