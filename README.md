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

1. __Bloques contiguos:__ 