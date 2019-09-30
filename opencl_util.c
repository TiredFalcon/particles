#include <stdlib.h>
#include <stdio.h>

#include <OpenCL/opencl.h>

#include "opencl_util.h"

struct error_descr {
  cl_int code;
  const char * description;
};

static struct error_descr error_descriptions [] = {
  { .code = CL_SUCCESS, .description = "CL_SUCCESS" },
  { .code = CL_DEVICE_NOT_FOUND, .description = "CL_DEVICE_NOT_FOUND" },
  { .code = CL_DEVICE_NOT_AVAILABLE, .description = "CL_DEVICE_NOT_AVAILABLE" },
  { .code = CL_COMPILER_NOT_AVAILABLE, .description = "CL_COMPILER_NOT_AVAILABLE" },
  { .code = CL_MEM_OBJECT_ALLOCATION_FAILURE, .description = "CL_MEM_OBJECT_ALLOCATION_FAILURE" },
  { .code = CL_OUT_OF_RESOURCES, .description = "CL_OUT_OF_RESOURCES" },
  { .code = CL_OUT_OF_HOST_MEMORY, .description = "CL_OUT_OF_HOST_MEMORY" },
  { .code = CL_PROFILING_INFO_NOT_AVAILABLE, .description = "CL_PROFILING_INFO_NOT_AVAILABLE" },
  { .code = CL_MEM_COPY_OVERLAP, .description = "CL_MEM_COPY_OVERLAP" },
  { .code = CL_IMAGE_FORMAT_MISMATCH, .description = "CL_IMAGE_FORMAT_MISMATCH" },
  { .code = CL_IMAGE_FORMAT_NOT_SUPPORTED, .description = "CL_IMAGE_FORMAT_NOT_SUPPORTED" },
  { .code = CL_BUILD_PROGRAM_FAILURE, .description = "CL_BUILD_PROGRAM_FAILURE" },
  { .code = CL_MAP_FAILURE, .description = "CL_MAP_FAILURE" },
  { .code = CL_MISALIGNED_SUB_BUFFER_OFFSET, .description = "CL_MISALIGNED_SUB_BUFFER_OFFSET" },
  { .code = CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST, .description = "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST" },
  { .code = CL_COMPILE_PROGRAM_FAILURE, .description = "CL_COMPILE_PROGRAM_FAILURE" },
  { .code = CL_LINKER_NOT_AVAILABLE, .description = "CL_LINKER_NOT_AVAILABLE" },
  { .code = CL_LINK_PROGRAM_FAILURE, .description = "CL_LINK_PROGRAM_FAILURE" },
  { .code = CL_DEVICE_PARTITION_FAILED, .description = "CL_DEVICE_PARTITION_FAILED" },
  { .code = CL_KERNEL_ARG_INFO_NOT_AVAILABLE, .description = "CL_KERNEL_ARG_INFO_NOT_AVAILABLE" },
  { .code = CL_INVALID_VALUE, .description = "CL_INVALID_VALUE" },
  { .code = CL_INVALID_DEVICE_TYPE, .description = "CL_INVALID_DEVICE_TYPE" },
  { .code = CL_INVALID_PLATFORM, .description = "CL_INVALID_PLATFORM" },
  { .code = CL_INVALID_DEVICE, .description = "CL_INVALID_DEVICE" },
  { .code = CL_INVALID_CONTEXT, .description = "CL_INVALID_CONTEXT" },
  { .code = CL_INVALID_QUEUE_PROPERTIES, .description = "CL_INVALID_QUEUE_PROPERTIES" },
  { .code = CL_INVALID_COMMAND_QUEUE, .description = "CL_INVALID_COMMAND_QUEUE" },
  { .code = CL_INVALID_HOST_PTR, .description = "CL_INVALID_HOST_PTR" },
  { .code = CL_INVALID_MEM_OBJECT, .description = "CL_INVALID_MEM_OBJECT" },
  { .code = CL_INVALID_IMAGE_FORMAT_DESCRIPTOR, .description = "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR" },
  { .code = CL_INVALID_IMAGE_SIZE, .description = "CL_INVALID_IMAGE_SIZE" },
  { .code = CL_INVALID_SAMPLER, .description = "CL_INVALID_SAMPLER" },
  { .code = CL_INVALID_BINARY, .description = "CL_INVALID_BINARY" },
  { .code = CL_INVALID_BUILD_OPTIONS, .description = "CL_INVALID_BUILD_OPTIONS" },
  { .code = CL_INVALID_PROGRAM, .description = "CL_INVALID_PROGRAM" },
  { .code = CL_INVALID_PROGRAM_EXECUTABLE, .description = "CL_INVALID_PROGRAM_EXECUTABLE" },
  { .code = CL_INVALID_KERNEL_NAME, .description = "CL_INVALID_KERNEL_NAME" },
  { .code = CL_INVALID_KERNEL_DEFINITION, .description = "CL_INVALID_KERNEL_DEFINITION" },
  { .code = CL_INVALID_KERNEL, .description = "CL_INVALID_KERNEL" },
  { .code = CL_INVALID_ARG_INDEX, .description = "CL_INVALID_ARG_INDEX" },
  { .code = CL_INVALID_ARG_VALUE, .description = "CL_INVALID_ARG_VALUE" },
  { .code = CL_INVALID_ARG_SIZE, .description = "CL_INVALID_ARG_SIZE" },
  { .code = CL_INVALID_KERNEL_ARGS, .description = "CL_INVALID_KERNEL_ARGS" },
  { .code = CL_INVALID_WORK_DIMENSION, .description = "CL_INVALID_WORK_DIMENSION" },
  { .code = CL_INVALID_WORK_GROUP_SIZE, .description = "CL_INVALID_WORK_GROUP_SIZE" },
  { .code = CL_INVALID_WORK_ITEM_SIZE, .description = "CL_INVALID_WORK_ITEM_SIZE" },
  { .code = CL_INVALID_GLOBAL_OFFSET, .description = "CL_INVALID_GLOBAL_OFFSET" },
  { .code = CL_INVALID_EVENT_WAIT_LIST, .description = "CL_INVALID_EVENT_WAIT_LIST" },
  { .code = CL_INVALID_EVENT, .description = "CL_INVALID_EVENT" },
  { .code = CL_INVALID_OPERATION, .description = "CL_INVALID_OPERATION" },
  { .code = CL_INVALID_GL_OBJECT, .description = "CL_INVALID_GL_OBJECT" },
  { .code = CL_INVALID_BUFFER_SIZE, .description = "CL_INVALID_BUFFER_SIZE" },
  { .code = CL_INVALID_MIP_LEVEL, .description = "CL_INVALID_MIP_LEVEL" },
  { .code = CL_INVALID_GLOBAL_WORK_SIZE, .description = "CL_INVALID_GLOBAL_WORK_SIZE" },
  { .code = CL_INVALID_PROPERTY, .description = "CL_INVALID_PROPERTY" },
  { .code = CL_INVALID_IMAGE_DESCRIPTOR, .description = "CL_INVALID_IMAGE_DESCRIPTOR" },
  { .code = CL_INVALID_COMPILER_OPTIONS, .description = "CL_INVALID_COMPILER_OPTIONS" },
  { .code = CL_INVALID_LINKER_OPTIONS, .description = "CL_INVALID_LINKER_OPTIONS" },
  { .code = CL_INVALID_DEVICE_PARTITION_COUNT, .description = "CL_INVALID_DEVICE_PARTITION_COUNT" },
  // { .code = CL_INVALID_PIPE_SIZE, .description = "CL_INVALID_PIPE_SIZE" },
  // { .code = CL_INVALID_DEVICE_QUEUE, .description = "CL_INVALID_DEVICE_QUEUE" }
};

extern const char *
util_error_message(cl_int err) {
  for(int i = 0; i < sizeof(error_descriptions)/sizeof(struct error_descr); ++i)
  if (error_descriptions[i].code == err)
  return error_descriptions[i].description;
  return "unknown error";
}

int
util_read_file(char ** bytes, size_t * length, const char * filename) {
  FILE * f = fopen(filename, "rb");
  if (!f) {
    fprintf(stderr, "could not open file %s\n", filename);
    return 1;
  }
  long len;
  if (fseek(f, 0, SEEK_END) != 0 || (len = ftell(f)) == EOF) {
    fprintf(stderr, "could not check file size for file %s\n", filename);
    fclose(f);
    return 1;
  }
  *length = len;
  *bytes = malloc(*length);
  if (! *bytes) {
    fprintf(stderr, "could not allocate memory for file %s\n", filename);
    fclose(f);
    return 1;
  }
  rewind(f);
  while (len > 0) {
    len -= fread(*bytes + (*length - len), 1, len, f);
    if (ferror(f)) {
      fprintf(stderr, "WARNING: error reading file %s\n", filename);
      *length -= len;
      break;
    }
    if (feof(f))
    break;
  }

  fclose(f);
  return 0;
}

extern void
util_print_platform_info(cl_platform_id platform) {
  char buf[1024];
  cl_int err;

  err = clGetPlatformInfo (platform, CL_PLATFORM_NAME, 1024, &buf, NULL);
  if (err == CL_SUCCESS) {
    printf("Platform: %s\n", buf);
  }
  err = clGetPlatformInfo (platform, CL_PLATFORM_VENDOR, 1024, &buf, NULL);
  if (err == CL_SUCCESS) {
    printf("Vendor: %s\n", buf);
  }
  err = clGetPlatformInfo (platform, CL_PLATFORM_VERSION, 1024, &buf, NULL);
  if (err == CL_SUCCESS) {
    printf("Version: %s\n", buf);
  }
  err = clGetPlatformInfo (platform, CL_PLATFORM_PROFILE, 1024, &buf, NULL);
  if (err == CL_SUCCESS) {
    printf("Profile: %s\n", buf);
  }
  err = clGetPlatformInfo (platform, CL_PLATFORM_EXTENSIONS, 1024, &buf, NULL);
  if (err == CL_SUCCESS) {
    printf("Extensions: %s\n", buf);
  }
}

int
util_compile_kernel(const char * kernel_sources[], const size_t sources_count,
  const char * kernel_name,
  cl_device_id device, cl_context context, cl_kernel * kernel) {
    char * sources_bytes[sources_count];
    size_t sources_length[sources_count];
    cl_int err;

    for(size_t i = 0; i < sources_count; ++i) {
      if (util_read_file(&(sources_bytes[i]), &(sources_length[i]), kernel_sources[i]) != 0) {
        while(i > 0)
        free(sources_bytes[--i]);
        return 1;
      }
    }

    cl_program program = clCreateProgramWithSource(context, sources_count,
      (const char **)sources_bytes, sources_length,
      &err);

      if (err != CL_SUCCESS) {
        fprintf(stderr, "failed to create program from source files:");
        goto print_error_and_cleanup;
      }

      // Build the program executable
      err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
      if (err != CL_SUCCESS) {
        fprintf(stderr, "failed to create program from source files:");
        if (err == CL_BUILD_PROGRAM_FAILURE) {
          // Determine the size of the log
          size_t log_size;
          clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

          // Allocate memory for the log
          char *log = (char *) malloc(log_size);
          if (!log) {
            fprintf(stderr, "failed to allocate memory for compiler log.\n");
          } else {
            // Get the log
            clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

            // Print the log
            printf("%s\n", log);
            free(log);
          }
        }
        goto print_error_and_cleanup;
      }

      *kernel = clCreateKernel(program, kernel_name, &err);
      if (err != CL_SUCCESS) {
        fprintf(stderr, "failed to create kernel `%s' from source files\n", kernel_name);
        goto print_error_and_cleanup;
      }

      for(size_t i = 0; i < sources_count; ++i)
      free(sources_bytes[i]);
      clReleaseProgram(program);

      return 0;

      print_error_and_cleanup:
      for(size_t i = 0; i < sources_count; ++i) {
        fprintf(stderr, " %s", kernel_sources[i]);
        free(sources_bytes[i]);
      }
      fprintf(stderr, "\n%s\n", util_error_message(err));
      clReleaseProgram(program);
      return 1;
    }

    static const cl_uint MAX_PLATFORMS = 4;
    static const cl_uint MAX_DEVICES = 10;

    int
    util_choose_device(cl_device_id * device_id) {
      cl_int err;

      cl_platform_id platforms[MAX_PLATFORMS];
      cl_uint platforms_count;

      err = clGetPlatformIDs(MAX_PLATFORMS, platforms, &platforms_count);
      if (err != CL_SUCCESS) {
        fprintf(stderr, "failed to get platform ids\n%s\n", util_error_message(err));
        return 1;
      }

      cl_device_id device_ids[MAX_DEVICES];
      cl_uint devices_count = 0;

      for(cl_uint i = 0; i < platforms_count && devices_count < MAX_DEVICES; ++i) {
        cl_uint n;
        err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU,
          (MAX_DEVICES - devices_count), device_ids + devices_count, &n);
          if (err != CL_SUCCESS) {
            fprintf(stderr, "failed to get device id\n%s\n", util_error_message(err));
            return 1;
          }
          devices_count += n;
        }

        cl_uint max_max_compute_units = 0;
        for(cl_uint i = 0; i < devices_count; ++i) {
          cl_uint max_compute_units;
          cl_bool has_compiler;
          cl_bool is_available;
          err = clGetDeviceInfo(device_ids[i], CL_DEVICE_AVAILABLE,
            sizeof(cl_bool), &is_available, NULL);
            if (err != CL_SUCCESS) {
              fprintf(stderr, "failed to get device info (CL_DEVICE_AVAILABLE)\n%s\n",
              util_error_message(err));
              continue;
            }
            if (!is_available)
            continue;
            err = clGetDeviceInfo(device_ids[i], CL_DEVICE_COMPILER_AVAILABLE,
              sizeof(cl_bool), &has_compiler, NULL);
              if (err != CL_SUCCESS) {
                fprintf(stderr, "failed to get device info (CL_DEVICE_COMPILER_AVAILABLE)\n%s\n",
                util_error_message(err));
                continue;
              }
              if (!has_compiler)
              continue;

              err = clGetDeviceInfo(device_ids[i], CL_DEVICE_MAX_COMPUTE_UNITS,
                sizeof(cl_uint), &max_compute_units, NULL);
                if (err != CL_SUCCESS) {
                  fprintf(stderr, "failed to get device info (CL_DEVICE_MAX_COMPUTE_UNITS)\n%s\n",
                  util_error_message(err));
                  continue;
                }
                if (max_compute_units > max_max_compute_units) {
                  *device_id = device_ids[i];
                  max_max_compute_units = max_compute_units;
                }
              }
              return  (max_max_compute_units == 0) ? 1 : 0;
            }
