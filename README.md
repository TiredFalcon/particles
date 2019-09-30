# Particles
> Assignment in the context of the Systems Programming course at USI Lugano in Spring 2017

> Author: Aron Fiechter\
> Course: Systems Programming\
> Submitted: 2017-05-31

____
## 0. Description

System requirements:
- gcc 10.0.0 or later
- gtk+2

____
## 1. Description

The program simulates `n` particles bouncing in a window.
Call the program by giving up to 7 arguments:
 - `n`: the number of particles in the simulation.
 - `fx`: horizontal component of the force field.
 - `fy`: vertical component of the force field.
 - `trace`: shading factor for the trace of a particle. This is the factor by
   which a frame of the animation is dimmed at each successive frame. In other
   words, the trace factor determines the exponential decay of the brightness
   in tracing a particle, thus a value of 1.0 results in no traces at all,
   while a value of 0.0 results in infinite traces.
 - `radius`: radius of the particles in pixels.
 - `delta`: inter-frame interval (in seconds).
 - `speed`: the initial speed of the particles.

Giving more than 7 arguments will result in the printing of usage info.
The arguments can be given in any order. Example:
```bash
  ./particles fy=0 n=1000 trace=0.15 speed=50 radius=5
```

The default values are:
 - n: 100
 - fx: 0
 - fy: 100
 - trace: 0.15
 - radius: 10
 - delta: 0.04
 - speed: 100

and will result in 100 bouncing particles.
Hit Q to end the simulation and close the program.


---
## 2. Physics

The simulation computes the new position and the new velocity of each particle at each frame. It also checks for bounces at each frame.\
In computing the new position, only the velocity is taken into account. The acceleration of the force field is only taken into account when computing the
new velocities.\
The bounce can sometimes look a bit odd because the simulation just lets the
particle go outside the window, inverts the velocity and draws the particle on
the edge of the window (thanks to Lara Bruseghini for suggesting this method).

Other solutions (commented in `particles_kernel.cl`) were to actually compute the new position of the particle after the bounce and render it there, but that solution resulted in odd-looking bounces (the particle bounced sometimes even higher than where it started from).\
In earlier implementations, the bounce simply did not update the position of the particle and inverted the velocity component, but this looked odd since in some cases it appeared as if the particle would bounce before hitting the border.

---
## 3. Added functionality

### Parameters
As seen in the description, there is an extra parameter, `speed`, which is used to compute the initial speed of the particles.

### Constants
Inside `particles.c`, there are parameters that can be changed, such as the windows size (set to 800x800).\
There are five constants, `PRECISION`, `FORCE`, `DISSIPATION` and `R`, `G`, `B` that are related to extra functionality.

Changing the constant DISSIPATION introduces energy loss with each bounce. This is set to 0, so by default the simulation never loses energy.

### Controls
The other constants &ndash; `PRECISION`, `FORCE` and `R`, `G`, `B` &ndash; are important here.
 - `PRECISION`: the precision with which the trace can be changed.
 - `FORCE`: the steps with which the forces are changed.
 - `R`, `G`, `B`: the three components for the colors of the particles.

These are the control keys:
 - `UP`/`DOWN` arrow keys &ndash; change the vertical component of the force field
 - `LEFT`/`RIGHT` arrow keys &ndash; change the horizontal component of the force field
 - `A`/`D` keys &ndash; change the length of the trace of the particles
 - `R`/`G`/`B`/`I` keys &ndash; set the colour of the particles to (R)ed, (G)reen, (B)lue or (I)nitial (the one defined in the file)
 - `Q` key &ndash; quit the simulation

### Graphics
The trace is much longer than normal with low (close to 0) values because each frame just multiplies each pixel by `sqrt(sqrt(1 - trace))`.

The window can be resized while the simulation is running.

---
## 4. Known limitations

 - The simulation works with up to about 75 million particles, but with this high number it is better to keep a very short radius (1 pixel) and a high delta (the simulation already lags with 0.1, which is 10 frames per second).
 - With a radius of 2 and 3 pixels, the particles are shown as small squares.
 - Resizing the window works but is a bit rough.


__________
## 5. Comments

The usage of the CPU remains around 40% for any number of particles if the the simulation is started with a delta of 0.1 and if the size of the window is 800x800.

P.S.: run this for fun
```bash
`./particles fy=0 n=1000 trace=-1 speed=50 radius=5`
```
