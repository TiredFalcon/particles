#ifndef OPENCL_UTIL_H_INCLUDED
#define OPENCL_UTIL_H_INCLUDED

#include <stddef.h>
#include <OpenCL/opencl.h>

extern int
util_read_file(char ** bytes, size_t * length, const char * filename);

extern void
util_print_platform_info(cl_platform_id platform);

extern const char *
util_error_message(cl_int err);

extern int
util_compile_kernel(const char * kernel_sources[], const size_t sources_count,
		    const char * kernel_name,
		    cl_device_id device, cl_context context, cl_kernel * kernel);

extern int
util_choose_device(cl_device_id * device_id);

#endif
