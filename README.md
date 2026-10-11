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

