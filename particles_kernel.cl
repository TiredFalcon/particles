/* particles_kernel.cl
 * Device code.
 */


/* Randomise position and velocity of a single ball. Used at the beginning.
 * Parameters:
 * - balls_data: the memory where the balls are stored linearly with position
 *   and velocity as (x, y, vx, vy)
 * - n: the number of balls
 * - w: the width of the window
 * - h: the height of the window
 * - r: the radius of a ball
 * - s: the MAGIC_SPEED constant
 */
__kernel void
random_init_kernel(__global float * balls_data,
									 float n,
									 int w,
									 int h,
									 float RADIUS,
									 float INIT_SPEED)
{

	int i = get_global_id(0);
	if (i > n) return;

	/* go to this ball */
	__global float * b = balls_data + i * 4;

	float spiral_speed = 32;			/* speed at which the spiral will rotate */
	float u = (i + 1) / (n + 1);	/* unique number in [0,1] for each ball */

	/* set (x, y, vx, vy) for a single ball */
  *(b)		 = (float)w/2;																			/* center x */
  *(b + 1) = (float)h/2;																			/* center y */
  *(b + 2) = cos((float) spiral_speed * u) * u * INIT_SPEED;	/* vx spiral */
  *(b + 3) = sin((float) spiral_speed * u) * u * INIT_SPEED;	/* vy spiral */

	return;
}





/* Reduce a pixel part (R, G, or B), by `trace` %.
 * We have to do this for each pixel, so for each part of a pixel, so we do not
 * care about where we are in the pixbuf.
 * Parameters:
 * - pixels: the memory where the pixels of the host's pixbuf are stored
 * - size: the number of bytes in the pixels
 * - trace: the dimming factor
 */
__kernel void
image_alpha_kernel(__global unsigned char * pixels,
									 int size,
									 float trace)
{

  int i = get_global_id(0);
	if (i >= size) return;

	/* just decrement the intensity of the pixel, but do so veeery slowly because
	 * it just looks nicer.
	 * With a trace above 1 (which would be a wrong number according to the usage)
	 * it fails (silently? There's just no trace like for when it is set to 1).
	 * With traces under 0 (negative) it becomes cool, with pretty colourful
	 * traces if the initial colour is not just white. */
	pixels[i] = pixels[i] * sqrt(sqrt((1 - trace)));
}



/* Compute the new position of a single ball based on its velocity and the given
 * force. Manage bounces off walls.
 * Draw the ball on the correct pixels in the pixbuf.
 * Parameters:
 * - balls_data: the memory where the balls are stored linearly with position
 *   and velocity as (x, y, vx, vy)
 * - n: the number of balls
 * - pixels: the memory where the pixels of the host's pixbuf are stored
 * - w: the width of the window
 * - h: the height of the window
 * - row_stride: the row_stride of the host's pixbuf
 * - n_channels: the number of channels of each pixel in the host's pixbuf
 * - fx: the x component of the force field
 * - fy: the y component of the force field
 * - r: the radius of a ball
 * - heat: the dissipation factor when hitting a wall
 * - rgb: an int containing three bytes for R, G, and B values for color
 */
/* Helpers:
 * - draw_circle: draws a full circle around the given (x,y) coordinates
 * - in_circle: checks if a coordinate falls in a radius
 */
static void draw_circle(int x, int y, int ball, int RADIUS, int n_channels, int row_stride, __global unsigned char * pixels, unsigned int RGB);
static int in_circle(int x, int y, int i, int j, int RADIUS);

__kernel void
update_balls_kernel(__global float * balls_data,
										float n,
										__global unsigned char * pixels,
										int w,
										int h,
										int row_stride,
										int n_channels,
										float FX,
										float FY,
										float R,
										float DELTA,
										float HEAT,
										unsigned int RGB)
{

	int i = get_global_id(0);
	if (i >= (int)n) return;

	float t = DELTA;					/* the time interval */
	int p_x, p_y;							/* coordinates of centre of ball for drawing */
	__global float * p;				/* to store pointer to this ball */
	float x, y, vx, vy;				/* position and velocities of this ball */
	float new_x, new_y;				/* new position of this ball */
	float new_vx, new_vy;			/* new velocity of this ball */

	/* go to this ball and get data */
	p = balls_data + i * 4;
	x  = *(p);
	y  = *(p + 1);
	vx = *(p + 2);
	vy = *(p + 3);


	/* find new position */
	// new_x = FX * t * t + vx * t + x;
	// new_y = FY * t * t + vy * t + y;
	new_x = vx * t + x;															/* don't use FX to keep E */
	new_y = vy * t + y;															/* don't use FY to keep E */

	/* check new position, invert velocities if necessary */
	if (new_x - R <= 0 || new_x + R >= w) {						/* if out of boundaries */
		vx = - vx;                          						/* invert vx */
		new_vx = vx * (1 - HEAT);               				/* energy dissipation */
    p_x = (new_x < w / 2) ? R : w - R;              /* graphical x on edge */

		/* more correct physical new position, but results in incorrect height of
		 * bounce, sometimes even higher than starting point */
		// if (new_x > w/2) *(p) = w - R - (new_x - w + R);
		// else *(p) = (fabs((float)new_x - R)) + R;
	}
	else {                                            /* else */
		new_vx = FX * t + vx;														/* update velocity vx */
		p_x = new_x;																		/* store graphical x */
	}

	if (new_y - R <= 0 || new_y + R >= h) {						/* if out of boundaries */
		vy = - vy;                          						/* invert vy */
		new_vy = vy * (1 - HEAT);               				/* energy dissipation */
    p_y = (new_y < h / 2) ? R : h - R;              /* graphical y on edge */

		/* more correct physical new position, but results in incorrect height of
		 * bounce, sometimes even higher than starting point */
		// if (new_y > h/2) *(p + 1) = h - R - (new_y - h + R);
		// else *(p + 1) = (fabs((float)new_y - R)) + R;
	}
	else {                                            /* else */
		new_vy = FY * t + vy;														/* update velocity vy */
		p_y = new_y;																		/* store graphical y */
	}

	/* update positions and velocities */
	*(p)     = new_x;
	*(p + 1) = new_y;
	*(p + 2) = new_vx;
	*(p + 3) = new_vy;

	/* paint the pixels for this ball */
	draw_circle(p_x, p_y, i, (int)R, n_channels, row_stride, pixels, RGB);
}

/* Draw the pixels for a single ball
 */
static void draw_circle(int x, int y, int ball, int RADIUS, int n_channels, int row_stride, __global unsigned char * pixels, unsigned int RGB) {

	__global unsigned char * pixel;
	unsigned char colors[3];
	colors[0] = (unsigned char) ((RGB & 0xFF0000) >> 16);	/* get red */
	colors[1] = (unsigned char) ((RGB & 0x00FF00) >> 8);	/* get green */
	colors[2] = (unsigned char) (RGB & 0x0000FF);					/* get blue */

  for (int j = y - RADIUS; j <= y + RADIUS; ++j) {
    for (int i = x - RADIUS; i <= x + RADIUS; ++i) {
      if (in_circle(x, y, i, j, RADIUS)) {
        /* color a single pixel */
        pixel = pixels + row_stride * j + n_channels * i;
        for (size_t k = 0; k < n_channels; ++k) {
					pixel[k] = colors[k];
        }
      }
    }
  }
}
static int in_circle(int x, int y, int i, int j, int RADIUS) {
  return (x - i) * (x - i) + (y - j) * (y - j) < RADIUS * RADIUS;
}
