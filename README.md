Kopf-Lischinski
===============

An implementation in C++ of Kopf-Lischinski algorithm


A description of this algorithm can be found here: http://research.microsoft.com/en-us/um/people/kopf/pixelart/

I liked what I saw in this paper, but I wasn't able to find an implementation, and I begin to make one own 
for learning reasons (I don't know if someone had made another one now).

It's unfinished. It's in the step 3.3 of the algorithm more or less, but it has several bugs. 
The code is a little messy too... Ok very messy... I wanted to have fixed it a little before publishing, 
but I've hada lack of time, in the last months. By this reason, finally I've decided to publish such as, 
in case someone wants to make testings with it. The program can generate now graphics with the results of firsts steps 
of the algorithm. I think that these graphs can be useful for various tasks. By example, I'm thinking about making
with them, a tool for creating puzzles for 3D printers.

Some images created with the firsts steps can be founded here: https://www.box.com/kopf-lischinski/

The program use the SDL and SDL_gfx libraries for drawing the graphs. It's one of the things I want to change... Now, the code of the 
algorithm is very mixed with sdl. I hope to separate them in a future version. 

The first step use the hqx algorithm to generate similarity graph. I'm using this implementation: https://code.google.com/p/hqx/
That hqx implementation is under LGPL license. Not sure if is needed include some copyright file for using. I hope not to be breaking the LGPL license :S

As I said before, I am a little short of time, but I have not abandoned this... I hope to continue it soon. 

----------------------------------------------------------------------------------------------------------------------
======================================================================================================================

Esta es una implentación del algoritmo Kopf-Lischinski.

La descripción de este algoritmo se puede encontrar aquí: http://research.microsoft.com/en-us/um/people/kopf/pixelart/

Cuando vi el paper por primera vez, me resultó interesante lo que podía hacer este algoritmo, y quise probarlo, pero
no entontré ninguna implentación, así que decidí hacer la mia propia, sobre todo por aprender (no se si ahora alguien 
habrá hecho otra).

Está sin acabar. Va más o menos por el paso 3.3 del pdf, pero tiene varios bugs que he ido viendo.
El código está algo desastroso... Vale, muy desastroso... La verdad, quería haberlo arreglado un poco antes de
publicarlo, pero he andado falto de tiempo en los últimos meses. Por esto al final he decidido publicarlo como está, por
si alguien quiere hacer pruebas con ello. Actualmente el programa puede generar gráficos con los primeros pasos del 
algoritmo. Creo que los gráficos que generan estos pasos  pueden ser utiles para diversas cosas. Por ejemplo, estoy
pensando en usarlos, para hacer en un futuro, una herramienta que genere puzzles para impresoras 3D.

Algunas imágenes generadas por los primeros del algoritmo, pueden encontrarse aquí: https://www.box.com/kopf-lischinski/

El programa usa las librerías SDL y SDL_gfx para dibujar los gráficos. Es una de las cosas que quería cambiar 
antes de publicarlo... Ahora toda la parte del algoritmo está muy mezclada con las sdl. Espero separar todo eso en una
futura versión.

El primer paso del algoritmo necesita a su vez del algoritmo hqx para general el gráfico de similitudes. Para esto he 
usado la implementación que se encuentra en https://code.google.com/p/hqx/ 
Esta implementación del algoritmo hqx está bajo la licencia LGPL. No se si con esta licencia es necesario incluir algún
archivo de copyright con mi código o algo así. Espero no infringir la LGPL :S

Como dije antes, ahora ando falto de tiempo, pero no he abandonado esto. Espero continuar con ello en breve.
