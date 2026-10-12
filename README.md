# Conjunto de Mandelbrot y Computación Paralela de Fractales

## Evaluación Parcial I

**Computación Científica / Metodos Numéricos II**   
**Universidad Nacional de Tucumán**  
**Ingeniería en Informática / Licenciatura en Informática** 
**Año:** 2026

### Integrantes

- Gomez, Rodrigo
- Rongetti Jaime, Bruno Alberto
- Tapia, Mario Facundo

---

## Índice

1. [Introducción](#1-introducción)
2. [Objetivos](#2-objetivos)
3. [Marco teórico](#3-marco-teórico)
   - [3.1. Fractales](#31-fractales)
   - [3.2. Conjunto de Mandelbrot](#32-conjunto-de-mandelbrot)
   - [3.3. Fundamentos de la computación paralela](#33-fundamentos-de-la-computación-paralela)
   - [3.4. Balance de carga y estrategias de distribución](#34-balance-de-carga-y-estrategias-de-distribución)
4. [Metodología](#4-metodología)
4. [Implementación](#5-implementación)
---

## 1. Introducción

La computación de alto rendimiento (HPC) consiste en hacer un uso óptimo de todos los recursos de cómputo disponibles, ya sea una computadora básica de escritorio, un cluster o infraestructura dedicada al cómputo. Se justifica cuando el problema es de gran complejidad o escala, requiriendo grandes simulaciones o volúmenes de datos, siempre y cuando el esfuerzo y el costo de paralelizar estos cálculos valgan la pena. La implementación de HPC implica tener conocimiento sobre el hardware, el lenguaje y compilador y del diseño de buenos algoritmos. 

El conjunto de Mandelbrot es un fractal definido en el plano complejo mediante la iteración:

$$
\begin{cases}
z_0 = 0, \\
z_{n+1} = z_n^2 + c, \quad n \geq 0.
\end{cases}
$$

Pertenecen al cnjunto los valores de c para los que la sucesión permanece acotada. Fue definido y dibujado por primera vez en 1978 por Brooks y Matelski, y en 1980 Benoit Mandelbrot obtuvo visualizaciones de alta calidad que lo popularizaron. Su cálculo es un problema de gran escala para HPC: una imagen requiere evaluar un punto por cada píxel y la cantidad de trabajo crece con la resolución y con el número máximo de iteraciones.

Para afrontar este problema se puede paralelizar fácilmente, ya que cada punto del plano se evalúa de forma independiente, sin comunicación entre tareas. Sin embargo, el costo de cada punto no es uniforme, los puntos alejados del conjunto divergen en una o dos iteraciones, los cercanos a su frontera necesitan muchas más, y los que pertenecen al conjunto consumen siempre el máximo permitido. Por ello, si el trabajo en partes iguales el procesa que reciba la zona central o la frontera va a trabajar mucho más que el que recibe una zona exterior, y esta última termina antes y queda esperando. 

Para enfrentarlo se puede usar dos familias de estrategias. En la distribución estática, el reparto se decide antes de ejecutar (por bloques o por filas). En la dinámica, es un proceso maestro el cual entrega trabajo a los demás a medida que vayan terminando (esquema maestro-trabajador), lo que absorbe el desbalance a cambio de un costo de comunicación. Comparar ambas y medir el impacto del balance de carga sobre el speedup es el eje en el cual se desarrolla este trabajo.

## 2. Objetivos

### 2.1 Objetivo general

Analizar la paralelización del generador del conjunto de Mandelbrot en C con MPI, comparando estrategias de distribución de trabajo estáticas y dinámicas, y evaluar el impacto del balance de carga en el speedup global.

### 2.2 Objetivos específicos

* Investigar el problema de aplicación, sus limitaciones y sus estrategias de resolución.
* Implementar una versión serial de referencia y versiones paralelas con MPI del generador del fractal.
* comparar la distribución estática del trabajo (por bloques o por filas) con la dinámica (maestro-trabajador).
* sleccionar o generar los datos de prueba necesarios para realizar diferentes tipos de pruebas.
* Ejecutar las implementaciones en diferentes sistemas (computadoras personales y cluster).
* Medir los tiempos con MPI_Wtime y calcular el speedup a partir de la mediana de múltiples ejecuciones.
* Realizar múltiples simulaciones variando los parámetros del problema y la cantidad de procesos.
* Analizar el impacto del balance de carga en el speedup global y concluir sobre los resultados obtenidos. 
* proponer mejoras en la implementación para futuros casos.

## 3. Marco teórico

### 3.1 Fractales

Un fractal es una forma geométrica con una estructura detallada a escalas muy pequeñas, cuya dimensión fractal suele superar strictamente su dimensión topológica. El término fractal fue acuñado en 1975 por Benoît Mandelbrot, a partir del latín frāctus, para extender la noción de dimensiones fraccionarias a los patrones geométricos de la naturaleza. Mandelbrot más tarde propuso describirlo como una forma irregular o fragmentada que puede dividirse en partes en donde cada una de ellas se, al menos aproximadamente, una copia reducida del todo. No hay una existe una definición formal única que satisfaga a todos los matemáticos, por lo que suele caracterizarse a los fractales con un conjunto de rasgos:

* **Autosimilitud**: El mismo patrón, o uno muy parecido, reaparecen al cambiar la escala. Puede ser exacta (como el copo de Koch), cuasi-autosimilitud (aproximada, con copias pequeñas del todo en formas distorcionadas como en los satelites del conjunto de Mandelbrot) o estadística (se consrvan las medidas numéricas, como en la costa de Gran Bretaña).
* **Estructura fina a escalas arbitrariamente pequeñas**: Cuando s amplía la imagen siguen apareciendo detalles.
* **Irregularidad**: Se presentan irregularidades que no puede ser descripta fácilmente con la geometría euclidiana, salvo como el límite de una secuencia de etapas definidas de manera recursiva.

*Dimensión fractal.* En las figuras ordinarias, una figura de dimensión *n* dividida en copias reducidas por un factor de $1/r$ produce $r^n$ piezas. La curva de Koch se divide en cuatro copias reducidas por un factor de $1/3$, así que su dimensión $D$ cumple $3^D = 4$, es decir, $D = \frac{\log 4}{\log 3} \approx 1{,}26$. Ese valor no entero es su dimensión fractal, y difiere de la dimensión topológica, que para una curva es $1$.

*Cómo se generan.* Entre las técnicas de generación, la que interesa para este trabajo en particular son los fractales de tiempo de escape (_escape-time_): Usan una fórmula o relación de recurrencia en cada punto de un espacio, como el plano complejo, y suelen ser cuasi-autosimilares. El conjunto de Mandelbrot es un ejemplo canónico, junto con los conjuntos de Julia. Su generación consiste, por lo tanto, en evaluar una misma regla de forma independiente en cada punto, lo que es la base del paralelismo que se busca estudiar en este proyecto.

### 3.2 Conjunto de Mandelbrot

*Definición.* El conjunto de Mandelbrot, $M$, es el conjunto de números complejos $c$ para los cuales la sucesión definida por $z_0 = 0$ y $z_{n+1} = z_n^2 + c$ permanece acotada en valor absoluto. Por ejemplo, para $c = 1$, la sucesión es $0, 1, 2, 5, 26, \ldots$, que tiende a infinito, por lo que $1 \notin M$. En cambio, para $c = -1$, la sucesión es $0, -1, 0, -1, \ldots$, que está acotada, por lo que $-1 \in M$.

*Origen.* Fue definido y dibujado por primera vez n 1978 por Robert Brooks y Pter Matelski, en un estudio sobre grupos kleinianos. En 1980, Benoit Mandelbrot obtuvo visualizaciones en alta calidad en el centro de investigación de IBM en Yorktown Heights. Las propiedades fundamentales del conjunto fueron establecidas luego por Douady y Hubbard (1985), quienes lo nombraron en su honor.

*Propiedades relevantes.*

* __Radio de escape:__ _M_ está contenido en el disco cerrado de radio 2 centrado en el origen. Si en algún punto paso $|z_n| > 2$, la sucesión escapa a infinito y c no pertenece a _M_. Esto permite decidir la divergencia sin iterar indefinidamente.
* __Cuasi-autosimilitud:__ En escalas arbitrariamente chicas aparecen copias del conjunto ligeramente distintas entre si.
* __Conexión:__ _M_ es conexo. Mandelbrot había conjeturado originalmente que era disconexo, basándose en imágenes de computadora que no detectaban los finos filamentos que unen las partes.

_Algoritmo de tiempo de escape_. Existen muchos algoritmos para graficar el conjunto, y es el más simple y popular es el de tiempo de escape (_escape time_). La región del plano complejo que se quiere graficar se divide en píxeles, y a cada uno le corresponde un valor _c_. Para cada píxel se itera la sucesión y se controla, en cada uno de los pasos, si se alcanzó la condición de escape. Si el punto escapa, se colorea según la cantidad de iteraciones empleadas. Si no escapa en un número máximo de iteraciones (definido en el programa), se lo considera "probablemente" parte del conjunto, o muy cercano a él, y se pinta de negro.

Escribiendo $z = x + iy$ y $c = x_0 + iy_0$, la iteración $z^2 + c$ queda expresada en términos de números reales: la parte real es $x^2 - y^2 + x_0$ y la parte imaginaria es $2xy + y_0$. Esto permite implementar el algoritmo sin necesidad de utilizar un tipo de dato complejo. Además, la condición $|z| > 2$ equivale a $x^2 + y^2 > 4$, lo que evita calcular una raíz cuadrada en cada iteración.

En pseudocódigo:

```text
para cada píxel (px, py) hacer
    x0 ← coordenada x del píxel, escalada a [−2,00; 0,47]
    y0 ← coordenada y del píxel, escalada a [−1,12; 1,12]
    x ← 0; y ← 0; iter ← 0

    mientras (x² + y² ≤ 4) y (iter < max_iter) hacer
        xtemp ← x² − y² + x0
        y ← 2 · x · y + y0
        x ← xtemp
        iter ← iter + 1

    color ← paleta[iter]
    pintar(px, py, color)
```

Los límites $[-2{,}00;\ 0{,}47]$ para la parte real y $[-1{,}12;\ 1{,}12]$ para la parte imaginaria corresponden a la ventana estándar que contiene a todo el conjunto.

_Limitaciones_

* __El resultado es una aproximación.__ Como la máxima iteración es finita, la región negra es una sobreestimación real: hay puntos que no pertenecen a _M_ pero tardan más que el valor máximo de iteración dado para escapar, y estas se pintan igual de negro. Al aumentar la cantidad máxima de iteraciones, la región negra se contrae hacia el conjunto verdadero y se obtiene más detalle.
* __El costo crece con el zoom y con el valor máximo de iteración:__ Al ampliar la zona de la frontera hace falta incrementar el número de iteraciones para distinguir los detalles más finos, por ello el tiempo de cálculo aumenta.
* __Dependencia de la resolución.__ Como se vio con la conjetura inicial de Mandelbrot sobre la conexión, una imagen con una resolución baja puede no mostrar las estructuras finas.

_Costo computacional._ El trabajo total es, en el peor caso, proporcional al producto del ancho por el alto de la imagen y por la cantidad máxima de iteraciones. El costo de cada píxel es independiente del de los demás pero no es uniforme:

* Los puntos alejados del conjunto escapan en pocas iteraciones.
* Los puntos cercanos a la frontera necesitan muchas más, con un costo variable y difícil de predecir. 
* Los puntos que pertenecen al conjunto nunca escapan y consumen siempre las máximas iteraciones completadas.

Esta irregularidad en el costo por píxel es el origen del desbalance de carga que se analiza más adelante en la sección 3.4

![Figura 1. Conjunto de Mandelbrot](img/figura1_mandelbrot.jpg)

*Figura 1. Conjunto de Mandelbrot (en negro) con el exterior coloreado según la velocidad de escape. Fuente: [AUTOR], Wikimedia Commons*

### 3.3 Fundamentos de la computación paralela

_El tiempo de ejecución en un programa paralelo_ es el tiempo total de ejecución de un programa, el cual puede descomponerse como _T = Tproc + Tcom + Tidle_. __Tproc__ es el tiempo de cómputo, que depende de la complejidad y dimensión del problema ,además de las características de las unidades de procesamiento. __Tcom__ s el tiempo de comunicación, que depende de la localidad de procesos y datos, como así también del canal de comunicación. __Tidle__ se el tiempo ocioso, producido por el no determinismo de la ejecución, y es el que obliga a balancear la carga. 

_Speedup y eficiencia_, la manera de saber que tan bueno puede llegar a ser un programa en paralelo se calculo mediante el __speedup__, $S_p = \frac{T_1}{T_p}$, donde T1 es el tiempo de ejecución de un procesador y Tp el tiempo con p procesadores. si $S_P = p$, el speedup es lineal y ello implica que la parte serial es casi nula. La __eficiencia__ es  $E_f = \frac{S_p}{p} = \frac{T_1}{p \cdot T_p}$., y un speedup lineal equivale a una eficiencia del 100%. En la practica lo más habitual es que sea un speedup sublineal (menor al 100%), por problemas con errores de paralelización, un mal balanceo de carga, el costo de la comunicación y de conexión, o por la sincronización. Solo en algunos casos puede verse un speedup superlineal (mayor al 100%). Como la ejecución no es determinística y no se puede evaluar la eficiencia algorítmica de forma teórica, hay que __medir los tiempos__ y por ello se corre múltiples ejecuciones y se usan estadísticas como la mediana.

_Ley de Amdahl_ la parte serial de un programa determina la cota inferior para el tiempo de ejecución, aún cuando se utilicen al máximo las técnicas de paralelismo. Si _P_ es la fracción del programa que se puede paralelizar (_1 - P_ es la fracción serial), el speedup máximo con _N_ procesadores es:

$$
\text{Speedup} = \frac{1}{(1 - P) + \frac{P}{N}}
$$

A cosecuencia, por más número de procesadores que se usen, el speedup nunca supera $\frac{1}{1 - P}$. Por ejemplo si el 10% del programa es serial, el speedup no puede pasar de 10.

El _Compromiso entre el paralelismo y overhead_. Lograr un buen rendimiento implica un compromiso entre el grado de paralelismo obtenido y el overhead (la sincronización y la comunicación). Para lograrlo se utilizan técnicas de __scheduling__ y de __balance de carga__. Además, la programación paralela no siempre es la solución y no es fácil obtener resultado óptimos: exige una programación optimizada, el conocimiento sobre el hardware subyacente, mucho tiempo invertido en la programación, lo cual dificulta el debugging y el profiling.

_Los modelos de programación:_ Mucho antes de programar en paralelo se toman decisiones con respecto a tres puntos importantes: El modelo de memoria (compartida o distribuida), cómo se divirá el problema (particionar los datos o funcionalidades) y qué lenguaje se usará (C, C++ o Fortran). En este trabajo se utilizará memoria distribuida, con C y la librería MPI.

El _Message Passing Interface_ (MPI) es una interfaz de pasaje de mensajes pensada principalmente para un modelo de memoria distribuida, y es especialmente eficiente para la descomposición de dominio. Existen varias implementacionse como MPICH y OPENMPI. Su modo de funcionamiento es ejecutar el mismo código (el programa) en múltiples procesos, por lo que se necesita gestionar la comunicación y coordinación entre ellos. Cada proceso se identifica por su rango (rank) dentro de un comunicador, que es el conjunto de procesos que participan (MPI_COMM_WORLD los incluye a todos). 

Estructura de un programa MPI:

```c
MPI_Init(&argc, &argv);                     // inicializa MPI
MPI_Comm_rank(MPI_COMM_WORLD, &rango);      // identificador del proceso
MPI_Comm_size(MPI_COMM_WORLD, &n_procesos); // cantidad de procesos
/* ... programa paralelo ... */
MPI_Finalize();                             // finaliza MPI
```

La comunicación puede ser de dos tipos:

- **Punto a punto:** un proceso envía y otro recibe, identificado por su rango. `MPI_Send` y `MPI_Recv` son *bloqueantes*, y `MPI_Isend` e `MPI_Irecv`, seguidos de `MPI_Wait`, son *no bloqueantes*. Un mal orden de los envíos y recepciones puede provocar un **deadlock**, es decir, procesos que quedan esperándose mutuamente.
- **Colectiva:** participan todos los procesos de un comunicador. Las operaciones son bloqueantes, no usan etiquetas (*tags*) y los buffers que reciben deben ser exactamente del mismo tamaño. Las principales son `MPI_Barrier` (sincronizar), `MPI_Bcast` (*broadcast*), `MPI_Scatter` y `MPI_Gather`, y las reducciones globales `MPI_Reduce` y `MPI_Allreduce`. Estas últimas permiten realizar cómputos sobre datos distribuidos en un grupo de procesos (máximo o mínimo global, suma o producto global), y `MPI_Allreduce` es la variante sin proceso raíz.

Para comunicar datos debe haber una equivalencia entre los tipos de datos de MPI y los de C:

| Tipo MPI | Tipo C |
|---|---|
| `MPI_CHAR` | `signed char` |
| `MPI_SHORT` | `signed short int` |
| `MPI_INT` | `signed int` |
| `MPI_LONG` | `signed long int` |
| `MPI_UNSIGNED_CHAR` | `unsigned char` |
| `MPI_UNSIGNED_SHORT` | `unsigned short int` |
| `MPI_UNSIGNED` | `unsigned int` |
| `MPI_UNSIGNED_LONG` | `unsigned long int` |
| `MPI_FLOAT` | `float` |
| `MPI_DOUBLE` | `double` |
| `MPI_LONG_DOUBLE` | `long double` |

Los tiempos se miden con `MPI_Wtime`.

### 3.4 Balance de carga y estrategias de distribución

El __balance de carga__ (__scheduling__) es el proceso de asignar recursos a tareas para que el cómputo global resulte eficiente. En el conjunto de Mandelbrot las tareas son independientes (un punto, una fila o un bloque de ellas) y no se conoce de antemano cuánto tardará cada una, porque el costo depende de la cantidad de iteraciones que necesita cada punto en particular para escapar. Por ellos la forma de repartir el trabajo entre los processos determina cuánto tiempo ocioso (definido como _Tidle_) aparece y, con él, el speedup global.

La _granularidad_ , un grano es una secuencia de pasos computacionales que se ejecuta de manera secuencial en el procesador. Si el grano es pequeño se lo conoce como granularidad fina, de lo contrario, como granularidad gruesa. Hay un compromiso: la granularidad gruesa implica poco overhead de comunicación, pero puede limitar la cantidad de paralelismo, mientras que la fina favorece al paralelismo pero puede producir un overhead de comunicación excesiva. En Mandelbrot, la unidad de trabajo puede ser un píxel, la fila o un bloque de filas.

#### Distribución estática

En la distribución __estática__ la decisión de qué proceso calcula cada punto se fija de antemano, antes de ejecutar el programa. Hay dos extremnos en granularidad:

1. __Bloques contiguos:__ La imagen se divide en _p_ partes iguales y cada proceso calcula la suya (por ejemplo, 5000 filas entre 5 procesos, 1000 filas cada uno). La comunicación ocurre al final, cuando cada proceso envía su submatriz al proceso raíz. El problema es que esa comunicación no se solapa con el cómputo, y que se reparte la misma cantidad de filas sin considerar su costo. En el caso particular de Mandelbrot, algunas filas contiene muchos más puntos del conjunto que otras por lo que eeste reparto produce un mal balance.
2. __Píxel por píxel:__ el píxel (_i_,_j_) de una matriz de _n x n_ lo calcula el proceso  `(i·n + j) mod p`.La comunicación queda totalmente entrelazada con el cómputo. Como cada píxel cuesta pocas iteraciones en el mejor caso, los procesos pueden pasar casi por todo el tiempo enviando y recibiendo, en lugar de realizar calculos.

Como compromiso entre ambos extremos, se reparte el trabajo por __filas intercaladas__ (cíclico o _round-robin_): la fila _i_ le corresponde al proceso `i mod p`. Para Mandelbrot se espera que funcione bien, ya que las filas costosas quedan repartidas entre los procesos, pero no existe garantía de un buen balance en general: para cualquier forma fija de asignar tareas existen distribuciones de tamaños de tarea que dan un mal balance. En el peor caso de la asignación estática, todas las tareas largas terminan en un mismo proceso y las cortas en otro.

#### Distribución dinámica: esquema maestro-trabajador

En la distribución _dinámica_, el trabajo se asigna durante la ejecución. En este esquema _maestro-trabajador_, un proceso (el maestro) administra la cola de tareas y los demás (los trabajadores) las calculan. El funcionamiento se distribuye de la siguiente manera:

1. El maestro envía una tarea a cada trabajador.
2. cuando un trabajador termina, envía su resultado al maestro y queda a la espera.
3. Si todavía quedan tareas, el maestro le envía la siguiente al trabajador que acaba de terminar. Si no quedan, envía una señal para terminar.

De esta manera, los procesos más rápidos reciben más tareas, y se logra un buen balance sin necesidad de conocer de antemano el tamaño de cada una. En un esquema simple de implementar, incluso con una infraestructura de comunicación rudimentaria. Requiere pasaje de mensajes entre el maestro y los trabajadores por lo que el maestro debe poder atender al trabajador que termine su tarea primero.

Sin embargo este esquema consta de _limitaciones_, este funciona bien con un número considerable de procesadores, pero no escala a muchos, por lo que un único maestro puede convertirse en cuello de botella. Si el tamaño promedio de tareas no es mucho mayor que el costo de comunicación multiplicada por el número de procesos, el overhead de arranque limita el rendimiento. Una forma de mitigar este problema es entregando paquetes de varias tareas a la vez, lo que introduce un compromiso: paquetes grandes producen el overhead pero aumentan el desbalance, porque el último paquete puede dejar a un proceso trabajando de más.

En el caso de _Mandelbrot_, entregar píxeles individuales no es escalable, sobre todo si gran parte de la región queda fuera del conjunto y la mayoría de los puntos escapa en pocas iteraciones. Por eso conviene agrupar las tareas en paquetes, lo cual no consta de demasiada complejidad ya que un rango de filas se puede describir de forma compacta.

El __tamaño del paquete__ (la granularidad) es entonces el parámetro central de la estrategia dinámica y su elección condiciona tanto el balance como el costo de la comunicación.

## 4. Metodología

### 4.1 Sistemas de prueba

Las pruebas se ejecutan en dos sistemas de características muy distintas: una computadora personal y un cluster.

| | PC personal | Cluster |
|---|---|---|
| **CPU** | Intel Core i7-8700 @ 3,20 GHz | AMD EPYC 7B12 |
| **Núcleos / hilos** | 6 / 12 (2 hilos por núcleo, 1 socket) | hasta 96 núcleos |
| **RAM** | 24 GB | hasta 384 GB |
| **Entorno de ejecución** | WSL 2 | Linux |
| **Sistema operativo** | Ubuntu 26.04 LTS | Ubuntu |
| **Implementación de MPI** | Open MPI 5.0.10 | Open MPI 4.1.6 |
| **Nodos** | 1 | 8 (la sesión usa 1) |

Los programas se compilan con `mpicc` y se ejecutan con `mpirun -n <nro_procesos>`. Cuando el número de procesos supera la cantidad de núcleos disponibles se usa la opción `--oversubscribe`.

### 4.2 Datos de prueba

El problema no requiere de datos externos, sino que la entrada se genera a partir de 3 parámetros que definen cada caso de prueba.

* __Región del plano complejo:__ la ventana `[Re_min, Re_max] × [Im_min, Im_max]` en la que se grafica.
* __Resolución:__ La cantidad de píxeles, dados por `ancho × alto`.
* __Número máximo de iteraciones:__ el `max_iter` del programa.

A cada píxel `(px, py)` le corresponde el punto medio del píxel, es decir, el valor `c = x0 + i·y0` con

```
x0 = Re_min + (px + 0,5) · (Re_max − Re_min) / ancho
y0 = Im_min + (py + 0,5) · (Im_max − Im_min) / alto
```

La salida es una matriz con la cantidad de iteraciones de escape de cada píxel. Para validar la implementación, el resultado de cada versión paralela se compara con el de la versión serial, y debe ser idéntico para cualquier número de procesos.

**Regiones de prueba.** Como el desbalance depende de la zona que se grafica, se eligen cuatro regiones con características de carga distintas:

| Región | Ventana (Re × Im) | Carga esperada |
|---|---|---|
| R1. Vista completa | [−2,00; 0,47] × [−1,12; 1,12] | Mixta: exterior, frontera e interior |
| R2. Exterior | [0,30; 0,90] × [−0,30; 0,30] | Baja y casi uniforme: todos los puntos divergen rápido |
| R3. Interior | [−0,15; 0,15] × [−0,15; 0,15] | Máxima y uniforme: todos los puntos alcanzan `max_iter` |
| R4. Frontera (valle de los caballitos de mar) | [−0,78; −0,72] × [0,07; 0,13] | Alta e irregular: zona de la frontera con costo muy variable |

R1 es la ventana estándar que contiene a todo el conjunto. R2 y R3 sirven como casos de control, en ambas la carga es pareja entre pixeles por lo que no debería producirse un desbalance, y permiten aislar el efecto del costo de comunicación. R4 es una región de frontera, centrada en el punto -0.75 + 0.1i (conocida como "El valle de los caballitos de mar"), y se espera que sea la más desbalanceada.

**Resoluciones.** Se utilizan imágenes cuadradas de 1000 × 1000, 2000 × 2000 y 4000 × 4000 píxeles. El número de filas se elige de modo que, para algunas cantidades de procesos, no sea múltiplo de ellos, para verificar que el reparto maneja correctamente las filas sobrantes.

**Máximo de iteraciones.** Se parte de `max_iter = 1000`, y se lo varía en la sección de experimentación para observar su efecto sobre el costo y el desbalance.

### 4.3 Parámetros de prueba

Cada ejecución queda definida por la combinación de los siguientes parámetros.

| Estrategia | Descripción |
|---|---|
| Serial | Versión sin MPI. Es la referencia para calcular T1 y validar los resultados. |
| Estática por bloques | Cada proceso calcula un bloque de filas contiguas. Las filas sobrantes se reparten entre los primeros procesos. |
| Estática cíclica por filas | La fila `i` la calcula el proceso `i mod p`. |
| Dinámica (maestro-trabajador) | El proceso 0 reparte paquetes de filas a pedido y recolecta los resultados. |

Para ello variaremos los siguientes parámetros del problema:

* __Región del plano:__ vistas en la sección anterior

- **Número de procesos (`p`):**
  - PC personal: 2, 4 y 6
  - Cluster: 2, 4, 6, 32 y 64
- **Tamaño del paquete (`k`)** en la estrategia dinámica: cantidad de filas que el maestro entrega por pedido (1, 4, 16 y 64).

Los valores de `p` se incluyen teniendo en cuenta el número de núcleos físicos de cada sistema (6 en la PC y hasta 64 en el cluster) y el de hilos (12 y 96), para observar qué ocurre al superar los núcleos físicos. En la estrategia dinámica, `p` es la cantidad total de procesos lanzados, incluyendo al maestro, por lo que los trabajadores son `p-1`. Se usa este criterio para que todas las estrategias se comparen con la misma cantidad de procesos lanzados.

**Parámetros fijos.** Resolución de 2000 × 2000 píxeles y `max_iter` = 1000 en todas las pruebas.

**Experimentos**

| Experimento | Objetivo | Se varía | Se mantiene fijo |
|---|---|---|---|
| A. Comparación de estrategias | Medir el speedup de E1, E2 y E3 y su relación con el balance de carga | Estrategia, región, `p` | `k` de E3 |
| B. Granularidad | Estudiar el efecto del tamaño del paquete en E3 | `k`, región (R1 y R4) | Un valor de `p` por sistema |

Primero se fija un número de procesos por sistema para realizar el experimento B. A partir de sus resultados se selecciona el tamaño de tarea que se utilizará en el experimento A.

**Criterios comunes a todas las pruebas**
- Todas las versiones se compilan con las mismas opciones de optimización (`-O2`) y se verifica que el resultado coincida con el de la versión serial.
- El tiempo de referencia T1 es el de la **versión serial**, no el de la versión paralela con un proceso, porque el esquema maestro-trabajador necesita al menos dos procesos.
- Cada configuración se ejecuta varias veces (sección 4.4) y se trabaja con la mediana.
- En el cluster, todas las pruebas se realizan dentro de un único nodo, por lo que la comunicación entre procesos es por memoria compartida.

### 4.4 Medición

**Tiempos.** Los tiempos paralelos se miden con `MPI_Wtime`. Antes de iniciar la medición, cada proceso prepara sus parámetros y reserva la memoria necesaria. Luego se ejecuta `MPI_Barrier` para establecer un punto común de partida.

El intervalo medido incluye el cálculo, la asignación de tareas y la comunicación necesaria para reunir la matriz completa en P0. Se excluyen la preparación de memoria, la impresión, la conversión a colores, la escritura de archivos y la validación.

Cada proceso obtiene su duración utilizando su propio reloj. El tiempo paralelo `Tp` se obtiene como el máximo de esas duraciones mediante `MPI_Reduce` con `MPI_MAX`. La reducción de tiempos queda fuera del intervalo medido.

El tiempo secuencial `T1` corresponde al cálculo de la matriz completa en la versión secuencial de referencia, excluyendo igualmente preparación y escritura.

**Repeticiones.** Cada configuración se ejecuta seis veces: una ejecución de calentamiento que se descarta y cinco ejecuciones medidas. Se conservan los tiempos originales y se informa la mediana, junto con el mínimo y el máximo.

**Métricas.**

- **Speedup:** `Sp = T1 / Tp`, utilizando las medianas de los tiempos secuenciales y paralelos de la misma configuración y sistema.
- **Eficiencia:** `Ef = Sp / p`, donde `p` es la cantidad total de procesos utilizados, incluido el maestro de la versión dinámica.
- **Desbalance de trabajo:** `D_iter = W_max / W_promedio`, donde `W` es la suma de iteraciones realizadas por cada proceso que calcula píxeles.

Un valor de `D_iter = 1` indica igual cantidad de iteraciones por proceso. Valores mayores indican que el proceso más cargado realizó más iteraciones que el promedio. Esta métrica representa trabajo computacional, no tiempo: la comunicación y las características del hardware también afectan al rendimiento.

En las versiones estáticas, el desbalance se calcula sobre los `p` procesos. En la dinámica se calcula sobre los `p−1` trabajadores, ya que P0 solo coordina. El máximo y el promedio se obtienen de los contadores de cada ejecución.

**Validación.** Fuera del intervalo medido, P0 puede guardar la matriz de iteraciones en un archivo de texto mediante `escribir_iteraciones`. Los archivos se comparan con `cmp`, que comprueba que todos los contadores estén en las mismas posiciones.

La validación se realiza en ejecuciones específicas, antes de aceptar las mediciones de una configuración. La igualdad de imágenes coloreadas o de sumas totales de iteraciones no reemplaza la comparación de matrices.

Hasta el momento se comprobó la igualdad de las matrices de bloques, cíclico y dinámica para R1, resolución 2000×2000, `max_iter = 1000` y cuatro procesos. Queda pendiente compararlas con la versión secuencial de referencia.

## 5. Implementación

Para la implementación se usaron las herramientas vistas en clase, C con MPI para la comunicación entre procesos. El proyecto consta de 4 versiones del generador de Mandelbrot: una secuencial de referencia y tres paralelas con MPI. Todas utilizan la misma función para realizar el cálculo, región del plano complejo, resolución y límite de iteraciones. Nos centramos en que la diferencia principal sea cómo se distribuyen los píxeles entre los procesos.

### 5.1 Versión secuencial

La versión secuencial recorre todas las filas y columnas usando solo un proceso, para cada píxel se determina el punto del plano correspondiente y se calcula el número de iteraciones que le corresponde.

Esta versión se va a usar como referencia para comparar con los resultados de las implementaciones paralelas, vamos a tomar el tiempo que demore como el tiempo base para calcular el speedup. Su medición comprende el cálculo de la matriz completa, excluyendo la escritura de la imagen.

### 5.2 Versión estática por bloques

La imagen se divide en bloques de filas consecutivas, asignados antes de comenzar el cálculo. Todos los procesos, incluido P0, calculan una parte de la imagen.

Cada proceso determina los límites de su bloque utilizando su identificador y la cantidad total de procesos. Al finalizar, envía sus resultados a P0, que reúne la matriz completa.

De esta forma se distribuye cantidades similares de píxeles, pero puede generar desbalance cuando algunos bloques contienen puntos que requieren muchas más iteraciones que otros.

### 5.3 Versión estática cíclica

Las filas se reparten por turnos entre los procesos. El proceso de identificador `rank` calcula las filas `rank`, `rank + p`, `rank + 2p`, y así sucesivamente, donde `p` es la cantidad de procesos.

Todos los procesos calculan y posteriormente reúnen sus resultados en P0.

Al repartir filas de distintas zonas de la imagen, esta estrategia puede mezclar trabajo de diferente costo y reducir el desbalance respecto de los bloques consecutivos. Su efectividad se evalúa mediante las mediciones.

### 5.4 Versión dinámica maestro-trabajador

P0 actúa como maestro y los demás procesos como trabajadores. El maestro asigna bloques de filas y entrega una nueva tarea a cada trabajador cuando recibe su resultado anterior.

La asignación se realiza durante la ejecución: los trabajadores que terminan antes pueden recibir más tareas. Con `p` procesos totales hay `p−1` trabajadores, porque el maestro no calcula píxeles.

El tamaño de tarea controla el compromiso entre balance y comunicación. Las tareas pequeñas permiten un reparto más flexible, pero generan más mensajes; las grandes reducen la comunicación y pueden aumentar el desbalance.

### 5.5 Comparación de las versiones

| Versión | Asignación del trabajo | Procesos que calculan | Comunicación de resultados |
|---|---|---|---|
| Secuencial | Todas las filas en un proceso | Uno | No requiere MPI |
| Estática por bloques | Intervalos consecutivos definidos al inicio | Todos | Reunión en P0 |
| Estática cíclica | Filas intercaladas definidas al inicio | Todos | Reunión en P0 |
| Dinámica | Bloques asignados durante la ejecución | Todos excepto el maestro | Entrega de resultados y nuevas tareas |

La comparación estudia el tiempo global, el speedup, la eficiencia y el balance de trabajo. El intervalo medido debe incluir el cálculo y la comunicación necesaria para reunir la matriz completa, excluyendo la generación del archivo de imagen.

Todas las versiones paralelas registran la cantidad de iteraciones realizada por cada proceso que calcula. Estos contadores permiten analizar el balance de trabajo. La generación de imágenes y la exportación de matrices para validación se realizan fuera del intervalo medido.