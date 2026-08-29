# Física secuencial

## Datos básicos

Cada `FruitCat` tendrá posición `position = (x, y, z)`, velocidad `velocity = (vx, vy, vz)` y radio `radius`.

## Fases de un frame secuencial

```text
1. Leer eventos y calcular deltaTime.
2. Actualizar temporizadores de estado e impactos.
3. Mover gatos activos.
4. Resolver rebotes contra las seis paredes.
5. Revisar colisiones entre pares de gatos.
6. Resolver impactos y crear eventos de explosión.
7. Actualizar partículas.
8. Renderizar la escena.
```

## Movimiento

Para cada gato activo:

```text
position = position + velocity × deltaTime
```

## Rebotes contra la arena

La arena se delimita con `minX/maxX`, `minY/maxY` y `minZ/maxZ`. Si el borde de una esfera sobrepasa una pared, se corrige su posición y se invierte solo la velocidad perpendicular a esa pared.

```text
Paredes X → invertir vx
Paredes Y → invertir vy
Paredes Z → invertir vz
```

## Colisiones entre gatos

La versión secuencial evalúa cada pareja una vez:

```cpp
for (int i = 0; i < N; ++i) {
    for (int j = i + 1; j < N; ++j) {
        // Comparar cats[i] con cats[j]
    }
}
```

Para dos gatos A y B:

```text
dx = B.x - A.x
dy = B.y - A.y
dz = B.z - A.z

distanceSquared = dx² + dy² + dz²
minimumDistance = radiusA + radiusB

Hay colisión si distanceSquared <= minimumDistance²
```

No se calcula raíz cuadrada durante la detección. Ante un choque se separan los gatos para evitar superposición, se modifica su velocidad para simular rebote y se aplica daño si su período de protección terminó.
