# Plan de paralelización con OpenMP

## Principio

OpenMP acelerará cálculos de simulación, no llamadas de OpenGL. El renderizado permanece en el hilo principal.

## Fases por frame

```text
Movimiento paralelo
        ↓ barrera
Detección de colisiones paralela (solo lectura)
        ↓
Resolución segura de eventos
        ↓
Actualización paralela de partículas
        ↓
Renderizado secuencial
```

## Fases paralelizables

| Fase | Estrategia | Riesgo |
|---|---|---|
| Actualizar gatos | `parallel for` por índice | Bajo: cada hilo modifica un gato diferente |
| Detectar colisiones | `parallel for` con eventos locales | Medio: varios pares pueden compartir gato |
| Actualizar partículas | `parallel for` por índice | Bajo: cada hilo modifica una partícula diferente |

## Eventos de colisión

La detección no modificará directamente gatos ni reservará partículas. Cada hilo guardará eventos en una lista local; posteriormente el hilo principal los combina y resuelve de forma segura.

```text
CollisionEvent = gato A, gato B, punto de impacto, tipos de fruta
```

Esto evita una condición de carrera como dos hilos intentando cambiar la velocidad del mismo gato al mismo tiempo.

## Métricas

La comparación se hará con la misma semilla, resolución y `N`:

```text
speedup = tiempo secuencial / tiempo paralelo
eficiencia = speedup / número de hilos
```

Se realizarán al menos diez mediciones por combinación de `N` e hilos.
