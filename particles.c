#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <math.h>

/*  This program uses the Gdk-Pixbuf and Gtk libraries, which are part
 *  of the GIMP Toolkit, a toolkit for graphical user interfaces.  See
 *  http://www.gtk.org, and in particular
 *  http://www.gtk.org/download/macos.php for a Mac OS version.
 */

#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

/* OpenCL:
 * - includes
 * - kernel sources
 * - global variables
 * - general flag for initialisation
 * - device memory pointers
 */
#include <OpenCL/opencl.h>
#include "opencl_util.h"
static const char * kernel_sources[] = { "particles_kernel.cl" };
static const char * random_init_kernel = "random_init_kernel";
static const char * image_alpha_kernel = "image_alpha_kernel";
static const char * update_balls_kernel = "update_balls_kernel";
static cl_device_id DEVICE;
static cl_context CONTEXT;
static cl_kernel INIT_KERNEL;
static cl_kernel ALPHA_KERNEL;
static cl_kernel BALLS_KERNEL;
static cl_command_queue QUEUE;
/* Flag: if init happened */
static int opencl_framework_available = 0;
/* Device memory: pixels (with flag) */
static cl_mem DEVICE_PIXELS;
static int device_pixels_allocated = 0;
/* Device memory: pixels (with flag) */
static cl_mem DEVICE_BALLS;
static int device_balls_allocated = 0;



/* #############################################################################
 * #                                 CONSTANTS                                 #
 */
/* Window */
#define WINDOW_IS_RESIZABLE 1
#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 800
/* Simulation */
#define DEFAULT_N_PARTICLES 100.0f
#define DEFAULT_TRACE 0.15f
#define DEFAULT_RADIUS 10.0f
#define DEFAULT_DELTA 0.04f
#define MILLI 1000.0f
#define PRECISION 0.05f
/* Physics */
#define DEFAULT_FORCE_X 0.0f
#define DEFAULT_FORCE_Y 100.0f
#define FORCE 10.0f
#define DEFAULT_INIT_SPEED 100.0f
#define DEFAULT_DISSIPATION 0.0f
/* Colours */
#define DEFAULT_R 100
#define DEFAULT_G 20
#define DEFAULT_B 237





/* #############################################################################
 * #                                 FUNCTIONS                                 #
 */
/* Setup */
int read_args(int argc, const char * argv[]);
void print_usage(void);

/* Tick */
static void randomize_balls(void);
static gboolean update_and_draw_balls(GtkWidget * widget);
static int alpha(void);
static int move_balls(void);
int draw_image(GtkWidget * widget);

/* Controls */
static void destroy_window(void);
static gint keyboard_input(GtkWidget * widget, GdkEventKey * event);
#if WINDOW_IS_RESIZABLE
static gint resize_pixbuf(GtkWidget * widget, GdkEventConfigure * event);
#endif

/* OpenCL */
static void initialize_opencl_framework(void);
static void shutdown_opencl_framework(void);
static void allocate_device_pixels(void);
static void allocate_device_balls(void);

/* Util */
static void print_balls(void);
static gboolean remove_keep_above(GtkWidget * widget);
static gint remover; /* used by the above function */





/* #############################################################################
 * #                                   MAIN                                    #
 */
/*
 * Global variables
 */
/* Pixbuf for image */
static GdkPixbuf * PIXBUF = NULL;
/* Set default simulation values */
static float N = DEFAULT_N_PARTICLES;
static float TRACE = DEFAULT_TRACE;
static float RADIUS = DEFAULT_RADIUS;
static float DELTA = DEFAULT_DELTA;
/* Set default physics values */
static float INIT_SPEED = DEFAULT_INIT_SPEED;
static float DISSIPATION = DEFAULT_DISSIPATION;
static float FX = DEFAULT_FORCE_X;
static float FY = DEFAULT_FORCE_Y;
/* Set default graphics values */
static unsigned int R = DEFAULT_R;
static unsigned int G = DEFAULT_G;
static unsigned int B = DEFAULT_B;


/* Main
 */
int main(int argc, const char *argv[]) {

  /* Init OpenCL */
  initialize_opencl_framework();

  /* Read arguments, if failed print usage */
  if (read_args(argc, argv)) {
    print_usage();
    return EXIT_FAILURE;
  }
  printf("n=%f\nfx=%f\nfy=%f\ntrace=%f\nradius=%f\ndelta=%f\nspeed=%f\n",
    N, FX, FY, TRACE, RADIUS, DELTA, INIT_SPEED);

  /* Allocate pixbuf for image, allocate space on device for copy */
  PIXBUF = gdk_pixbuf_new(GDK_COLORSPACE_RGB, 0, 8,
    DEFAULT_WIDTH, DEFAULT_HEIGHT);
  allocate_device_pixels();

  /* Allocate space for balls data (x, y, dx, dy) on device, then call the first
   * kernel (INIT_KERNEL) to randomise the data.
   * This kernel will never be called again.
   */
  allocate_device_balls();
  randomize_balls();

  /* Initialise GTK */
  gtk_init(0, 0);
  GtkWidget * window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_resize(GTK_WINDOW(window), DEFAULT_WIDTH, DEFAULT_HEIGHT);
  gtk_window_set_title(GTK_WINDOW(window), "Particles");

  /* Set listeners */
  gtk_signal_connect(GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(destroy_window), NULL);
  gtk_signal_connect(GTK_OBJECT(window), "key_press_event", GTK_SIGNAL_FUNC(keyboard_input), NULL);
  #if WINDOW_IS_RESIZABLE
  gtk_signal_connect(GTK_OBJECT(window), "configure_event", GTK_SIGNAL_FUNC(resize_pixbuf), NULL);
  #endif

  /* Show window, set timeout, start main */
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_widget_show_all(window);
  gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
  gtk_window_present(GTK_WINDOW(window));
  gtk_timeout_add(MILLI * DELTA, (GSourceFunc) update_and_draw_balls,
    (gpointer) window);

  remover = gtk_timeout_add(MILLI * DELTA, (GSourceFunc) remove_keep_above,
    (gpointer) window);

  /* Draw initial image and call gtk main */
  draw_image(window);
  gtk_main();

  // gtk_window_set_keep_above(GTK_WINDOW(window), FALSE);

  return EXIT_SUCCESS;
}





/* #############################################################################
 * #                                   SETUP                                   #
 */
/*
 * Read arguments from `argv` and stores them correctly.
 * - n=integer sets the number of particles in the simulation.
 * - fx=number horizontal component of the force field.
 * - fy=number vertical component of the force field.
 * - trace=number shading factor for the trace of a particle. This is the factor
 *   by which a frame of the animation is dimmed at each successive frame. In
 *   other words, the trace factor determines the exponential decay of the
 *   brightness in tracing a particle, thus a value of 1.0 results in no traces
 *   at all, while a value of 0.0 results in infinite traces.
 * - radius=number radius of the particles in pixels.
 * - delta=time-in-seconds inter-frame interval.
 * - speed=number the initial speed of the balls.
 * Returns 0 if the arguments were correctly read and stored.
 * Returns -1 if the arguments were wrong, or if there were too many arguments.
 */
int read_args(int argc, const char *argv[]) {

  /* keywords to parse */
  int n = 7; /* number of keywords in the below array */
  char * args[] = { "n=", "fx=", "fy=", "trace=", "radius=", "delta=", "speed="};
  float * args_p[] = { &N, &FX, &FY, &TRACE, &RADIUS, &DELTA, &INIT_SPEED};

  /* no more than 6 args should be given */
  if (argc > n + 1) return -1;

  /* search for args */
  for (size_t i = 1; i < argc; ++i) {
    int found = 0;
    for (size_t j = 0; j < n; ++j) {
      /* if arg given */
      if (!memcmp(argv[i], args[j], strlen(args[j]))) {
        /* save arg */
        float read = strtod((argv[i] + strlen(args[j])), NULL);
        *args_p[j] = read;
        found = 1;
        break;
      }
    }
    if (!found) {
      printf("read_args: unknown argument %s\n", argv[i]);
    }
  }
  return 0;
}

void print_usage(void) {
    fprintf(stderr, "usage: ./particles [n=num_particles] [fx=force_x] "
      "[fy=force_y] [trace=shading] [radius=ball_r] [delta=sec_x_frame]"
      "[speed=num]\n");
};





/* #############################################################################
 * #                                  RUNNING                                  #
 */

/* Randomise data of balls
*/
static void randomize_balls(void) {
  cl_int err;

  int width = gdk_pixbuf_get_width(PIXBUF);
  int height = gdk_pixbuf_get_height(PIXBUF);

  err  = clSetKernelArg(INIT_KERNEL, 0, sizeof(cl_mem), &DEVICE_BALLS);
  err |= clSetKernelArg(INIT_KERNEL, 1, sizeof(float), &N);
  err |= clSetKernelArg(INIT_KERNEL, 2, sizeof(int), &width);
  err |= clSetKernelArg(INIT_KERNEL, 3, sizeof(int), &height);
  err |= clSetKernelArg(INIT_KERNEL, 4, sizeof(float), &RADIUS);
  err |= clSetKernelArg(INIT_KERNEL, 5, sizeof(float), &INIT_SPEED);

  if (err != CL_SUCCESS) {
    fprintf(stderr, "randomize_balls: error setting kernel parameters: %s\n", util_error_message(err));
    return;
  }

  size_t init_kernel_size = (size_t)N;
  err = clEnqueueNDRangeKernel(QUEUE, INIT_KERNEL, 1, NULL, &init_kernel_size,
    NULL, 0, NULL, NULL);

  /* Wait for kernel to finish */
  clFinish(QUEUE);

  /* used for debug */
  // print_balls();

  if (err != CL_SUCCESS) {
    fprintf(stderr, "error launching the kernel: %s\n", util_error_message(err));
    return;
  }
  return;
}

/* Applies and alpha shading on the pixbuf using the device kernel ALPHA_KERNEL.
 * Updates the position of all balls using the device kernel BALLS_KERNEL.
 * Copies the pixbuf pixels back from the device and renders it.
 * This is the callback passed to `gtk_timeout_add`, and it is executed every
 * `MILLI * DELTA` milliseconds.
 */
static gboolean update_and_draw_balls(GtkWidget * widget) {

  /* Decrease alpha of previous frame */
  if (alpha()) return FALSE;

  /* Wait for kernel to finish */
  clFinish(QUEUE);

  /* Update positions of all balls and set their pixels */
  if (move_balls()) return FALSE;

  /* Wait for kernel to finish */
  clFinish(QUEUE);

  /* Get pixels back and draw image */
  if (draw_image(widget)) return FALSE;

  return TRUE;
}

/* Dim the pixbuf pixels using ALPHA_KERNEL.
 * Returns 0 on success, -1 on failure.
 */
static int alpha(void) {
  cl_int err;

  int height = gdk_pixbuf_get_height(PIXBUF);
  int row_stride = gdk_pixbuf_get_rowstride(PIXBUF);

  int size = (int) (height * row_stride);

  err  = clSetKernelArg(ALPHA_KERNEL, 0, sizeof(cl_mem), &DEVICE_PIXELS);
  err |= clSetKernelArg(ALPHA_KERNEL, 1, sizeof(int), &size);
  err |= clSetKernelArg(ALPHA_KERNEL, 2, sizeof(float), &TRACE);

  size_t alpha_kernel_size = (size_t) (height * row_stride);
  err = clEnqueueNDRangeKernel(QUEUE, ALPHA_KERNEL, 1, NULL, &alpha_kernel_size,
    NULL, 0, NULL, NULL);

  if (err != CL_SUCCESS) {
    fprintf(stderr, "error launching the kernel: %s\n", util_error_message(err));
    return -1;
  }

  return 0;
}

/* Computes the new positions for all balls, with bounce and force using
 * BALLS_KERNEL.
 * Returns 0 on success, -1 on failure.
 */
static int move_balls(void) {

  cl_int err;

  int width = gdk_pixbuf_get_width(PIXBUF);
  int height = gdk_pixbuf_get_height(PIXBUF);
  int row_stride = gdk_pixbuf_get_rowstride(PIXBUF);
  int n_channels = gdk_pixbuf_get_n_channels(PIXBUF);
  unsigned int RGB = (unsigned int) R << 16 | (unsigned int) G << 8 | (unsigned int) B;

  err  = clSetKernelArg(BALLS_KERNEL, 0, sizeof(cl_mem), &DEVICE_BALLS);
  err |= clSetKernelArg(BALLS_KERNEL, 1, sizeof(float), &N);
  err |= clSetKernelArg(BALLS_KERNEL, 2, sizeof(cl_mem), &DEVICE_PIXELS);
  err |= clSetKernelArg(BALLS_KERNEL, 3, sizeof(int), &width);
  err |= clSetKernelArg(BALLS_KERNEL, 4, sizeof(int), &height);
  err |= clSetKernelArg(BALLS_KERNEL, 5, sizeof(int), &row_stride);
  err |= clSetKernelArg(BALLS_KERNEL, 6, sizeof(int), &n_channels);
  err |= clSetKernelArg(BALLS_KERNEL, 7, sizeof(float), &FX);
  err |= clSetKernelArg(BALLS_KERNEL, 8, sizeof(float), &FY);
  err |= clSetKernelArg(BALLS_KERNEL, 9, sizeof(float), &RADIUS);
  err |= clSetKernelArg(BALLS_KERNEL, 10, sizeof(float), &DELTA);
  err |= clSetKernelArg(BALLS_KERNEL, 11, sizeof(float), &DISSIPATION);
  err |= clSetKernelArg(BALLS_KERNEL, 12, sizeof(unsigned int), &RGB);

  if (err != CL_SUCCESS) {
    fprintf(stderr, "move_balls: error setting kernel parameters: %s\n", util_error_message(err));
    return -1;
  }

  size_t balls_kernel_size = (size_t)N;
  err = clEnqueueNDRangeKernel(QUEUE, BALLS_KERNEL, 1, NULL, &balls_kernel_size,
    NULL, 0, NULL, NULL);

  if (err != CL_SUCCESS) {
    fprintf(stderr, "error launching the kernel: %s\n", util_error_message(err));
    return -1;
  }

  return 0;
}

/* Reads the device pixels back into the host's pixbuf, then draws the pixbuf
 * using Gtk.
 * Returns 0 on success, -1 on failure.
 */
int draw_image(GtkWidget *widget) {
  int w = gdk_pixbuf_get_width(PIXBUF);
  int h = gdk_pixbuf_get_height(PIXBUF);
  int row_stride = gdk_pixbuf_get_rowstride(PIXBUF);
  guchar * pixels = gdk_pixbuf_get_pixels(PIXBUF);
  cl_int err;

  /* Get the pixbuf back */
  err = clEnqueueReadBuffer(QUEUE, DEVICE_PIXELS, CL_TRUE,
		0, sizeof(unsigned char)*h*row_stride,
    pixels,
    0, NULL, NULL);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "error reading pixbuf from GPU: %s\n", util_error_message(err));
    return -1;
  }

  /* Draw */
  gdk_draw_pixbuf(widget->window, NULL, PIXBUF,
    0, 0, 0, 0, w, h,
    GDK_RGB_DITHER_NONE, 0, 0);
  return 0;
}






/* #############################################################################
 * #                                  CONTROLS                                 #
 */

/* Stops the execution of the app
 */
static void destroy_window(void) {
  gtk_main_quit();
}

/* Reads keyboard input and acts accordingly
 */
static gint keyboard_input(GtkWidget *widget, GdkEventKey *event) {
  if (event->type != GDK_KEY_PRESS) return FALSE;

  switch(event->keyval) {
    case GDK_KEY_Up:
    FY -= FORCE;
    printf("FY: %f\n", FY);
    break;

    case GDK_KEY_Down:
    FY += FORCE;
    printf("FY: %f\n", FY);
    break;

    case GDK_KEY_Left:
    FX -= FORCE;
    printf("FX: %f\n", FX);
    break;

    case GDK_KEY_Right:
    FX += FORCE;
    printf("FX: %f\n", FX);
    break;

    case GDK_KEY_a:
    TRACE -= PRECISION;
    printf("TRACE: %f\n", TRACE);
    break;

    case GDK_KEY_d:
    TRACE += PRECISION;
    printf("TRACE: %f\n", TRACE);
    break;

    /* BREAKS IN SOME CASES: when a ball is on the edge and the radius is
     * increased, the ball gets clipped and it could cause an error */
    // case GDK_KEY_w:
    // RADIUS = (float)((int)(RADIUS + 1) % 400);
    // printf("RADIUS: %f\n", RADIUS);
    // break;
    //
    // case GDK_KEY_s:
    // RADIUS -= 1;
    // printf("RADIUS: %f\n", RADIUS);
    // break;

    case GDK_KEY_r:
    R = 255; G = 0; B = 0;
    printf("COLOUR: RED (%u, %u, %u)\n", R, G, B);
    break;

    case GDK_KEY_g:
    R = 0; G = 255; B = 0;
    printf("COLOUR: GREEN (%u, %u, %u)\n", R, G, B);
    break;

    case GDK_KEY_b:
    R = 0; G = 0; B = 255;
    printf("COLOUR: BLUE (%u, %u, %u)\n", R, G, B);
    break;

    case GDK_KEY_i:
    R = DEFAULT_R; G = DEFAULT_G; B = DEFAULT_B;
    printf("COLOUR: INITIAL (%u, %u, %u)\n", R, G, B);
    break;

    case GDK_KEY_Q:
    case GDK_KEY_q:
    gtk_main_quit();
    break;

    default:
    return FALSE;
  }
  return TRUE;
}

#if WINDOW_IS_RESIZABLE
static gint resize_pixbuf(GtkWidget *widget, GdkEventConfigure * event) {
  if (PIXBUF) {
    int width = gdk_pixbuf_get_width(PIXBUF);
    int height = gdk_pixbuf_get_height(PIXBUF);
    if (width == widget->allocation.width && height == widget->allocation.height) {
      return FALSE;
    }
    g_object_unref(PIXBUF);
  }

  PIXBUF = gdk_pixbuf_new(GDK_COLORSPACE_RGB, 0, 8,
    widget->allocation.width, widget->allocation.height);

  allocate_device_pixels();

  update_and_draw_balls(widget);

  return TRUE;
}
#endif





/* #############################################################################
 * #                                  OPENCL                                   #
 */

/* Get device, create context, compile kernels, create command queue.
 * handle errors
 */
static void initialize_opencl_framework(void) {
  cl_int err;

  if (util_choose_device(&DEVICE) != 0)
    goto device_unavailable;

  CONTEXT = clCreateContext(NULL, 1, &DEVICE, NULL, NULL, &err);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "failed to create context\n%s\n", util_error_message(err));
    goto device_unavailable;
  }

  if (util_compile_kernel(kernel_sources, sizeof(kernel_sources)/sizeof(const char *),
		random_init_kernel, DEVICE, CONTEXT, &INIT_KERNEL) != 0) {
    goto cleanup_init_kernel;
  }
  if (util_compile_kernel(kernel_sources, sizeof(kernel_sources)/sizeof(const char *),
		image_alpha_kernel, DEVICE, CONTEXT, &ALPHA_KERNEL) != 0) {
    goto cleanup_alpha_kernel;
  }
  if (util_compile_kernel(kernel_sources, sizeof(kernel_sources)/sizeof(const char *),
		update_balls_kernel, DEVICE, CONTEXT, &BALLS_KERNEL) != 0) {
    goto cleanup_balls_kernel;
  }

  QUEUE = clCreateCommandQueue(CONTEXT, DEVICE, 0, &err);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "failed to create command queue\n%s\n", util_error_message(err));
    goto cleanup_queue;
  }

  opencl_framework_available = 1;
  return;

  cleanup_queue:
    clReleaseKernel(BALLS_KERNEL);
  cleanup_balls_kernel:
    clReleaseKernel(ALPHA_KERNEL);
  cleanup_alpha_kernel:
    clReleaseKernel(INIT_KERNEL);
  cleanup_init_kernel:
    clReleaseContext(CONTEXT);
  device_unavailable:
    opencl_framework_available = 0;
    return;
}

/* Cleanup everything.
 */
static void shutdown_opencl_framework(void) {
  if (opencl_framework_available) {
    clReleaseKernel(BALLS_KERNEL);
    clReleaseKernel(ALPHA_KERNEL);
    clReleaseKernel(INIT_KERNEL);
    clReleaseCommandQueue(QUEUE);
    clReleaseContext(CONTEXT);
    if (device_pixels_allocated) {
      clReleaseMemObject(DEVICE_PIXELS);
      device_pixels_allocated = 0;
    }
    if (device_balls_allocated) {
      clReleaseMemObject(DEVICE_BALLS);
      device_balls_allocated = 0;
    }
    opencl_framework_available = 0;
  }
}

/* Allocate memory for pixels on device
 */
static void allocate_device_pixels(void) {
  if (opencl_framework_available) {
    if (device_pixels_allocated) {
      clReleaseMemObject(DEVICE_PIXELS);
      device_pixels_allocated = 0;
    }
    cl_int err;
    int rows = gdk_pixbuf_get_height(PIXBUF);
    int row_stride = gdk_pixbuf_get_rowstride(PIXBUF);

    DEVICE_PIXELS = clCreateBuffer(CONTEXT, CL_MEM_READ_WRITE,
      sizeof(unsigned char)*row_stride*rows, NULL, &err);
    if (err != CL_SUCCESS) {
      fprintf(stderr,
		    "failed to create pixels buffer on device\n%s\n"
		    "shutting down OpenCL device.\n",
      util_error_message(err));
      shutdown_opencl_framework();
      return;
    }
    device_pixels_allocated = 1;
  }
}

static void allocate_device_balls(void) {
  if (opencl_framework_available) {
    if (device_balls_allocated) {
      clReleaseMemObject(DEVICE_BALLS);
      device_balls_allocated = 0;
    }
    cl_int err;
    int n_balls = (int)N;

    DEVICE_BALLS = clCreateBuffer(CONTEXT, CL_MEM_READ_WRITE,
      sizeof(float)*n_balls*4, NULL, &err);
    if (err != CL_SUCCESS) {
      fprintf(stderr,
		    "failed to create balls buffer on device\n%s\n"
		    "shutting down OpenCL device.\n",
      util_error_message(err));
      shutdown_opencl_framework();
      return;
    }
    device_balls_allocated = 1;
  }
}





/* #############################################################################
 * #                                   UTIL                                    #
 */

/* Get balls data from device, print them. Used for debug.
 */
static void print_balls(void) {
  cl_int err;
  float * BALLS = malloc(N * 4 * sizeof(float));
  err = clEnqueueReadBuffer(QUEUE, DEVICE_BALLS, CL_TRUE,
		0, sizeof(float) * 4 * N,
    BALLS,
    0, NULL, NULL);
  if (err != CL_SUCCESS) {
    fprintf(stderr, "error reading balls from GPU: %s\n", util_error_message(err));
    return;
  }
  float * p = BALLS;
  for (size_t i = 0; i < (int)N; ++i) {
    float x = *p++;
    float y = *p++;
    float dx = *p++;
    float dy = *p++;
    printf("B%zu: (%f,%f)\t %f %f\n", i + 1, x, y, dx, dy);
  }
  printf("\n");
  free(BALLS);
}

/* Remove keep above from window (so we can switch to other windows), then
 * remove this timeout (saved as `remover`).
 * Return FALSE so that even if removal fails, the timeout dies.
 */
static gboolean remove_keep_above(GtkWidget * widget) {
  gtk_window_set_keep_above(GTK_WINDOW(widget), FALSE);
  gtk_timeout_remove(remover);
  return FALSE;
}
