# Simulación de FruitCat Chaos

## Propósito

Mantener un *screensaver* infinito de `N` gatos-fruta dentro de una arena 3D. Los gatos acumulan impactos, explotan al alcanzar un umbral y reaparecen para conservar la población total.

## Tipos de gato

| Tipo | Apariencia | Explosión |
|---|---|---|
| `Banana` | Amarillo/dorado | Amarillo, dorado y naranja |
| `Avocado` | Verde | Verde lima y verde oscuro |

Los gatos se representarán como *billboards*: sprites 2D ubicados en coordenadas 3D que siempre se orientan hacia la cámara.

## Estado de un gato

```text
Alive → Exploding → Respawning → Alive
```

| Estado | Física | Renderizado | Transición |
|---|---|---|---|
| `Alive` | Se mueve, rebota y colisiona | Sprite normal o dañado | Al llegar a `maxImpacts` |
| `Exploding` | Sin movimiento ni colisiones | Sprite de explosión y partículas | Tras `explosionDuration` |
| `Respawning` | Sin movimiento ni colisiones | Invisible | Tras `respawnDelay` |

## Impactos y daño visual

Valores predeterminados:

```text
maxImpacts       = 7
wallDamage       = 1
catDamage        = 1
hitCooldown      = 0.25 segundos
explosionDuration = 0.50 segundos
respawnDelay     = 0.75 segundos
```

Un choque con pared o con otro gato suma el daño indicado, siempre que el gato no tenga un `hitCooldown` activo. Esto evita contar múltiples impactos mientras dos objetos siguen en contacto.

| Impactos | Apariencia del sprite |
|---:|---|
| 0–2 | Normal |
| 3–4 | Tinte rojo/naranja suave |
| 5–6 | Tinte rojo intenso y posible parpadeo |
| 7 | Explosión |

## Explosión y reaparición

Al alcanzar el umbral, el gato deja de participar en la física, genera partículas según los tipos implicados en el último impacto y muestra un sprite de explosión. Después espera y reaparece con:

- posición pseudoaleatoria dentro de la arena;
- velocidad pseudoaleatoria;
- impactos reiniciados a cero;
- estado `Alive`;
- separación mínima de las paredes y de gatos activos cercanos.

El arreglo de gatos no se redimensiona. Los `N` objetos continúan ocupando sus mismas posiciones de memoria y solo cambian de estado.
