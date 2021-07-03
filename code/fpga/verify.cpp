#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#include <CL/cl2.hpp>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>

#include "../fmindex.h"
#include "../util.h"

std::vector<cl::Device> get_xilinx_devices();
char *read_binary_file(const std::string &xclbin_file_name, unsigned &nb);

int main(int argc, char **argv) {
  if (argc < 6) {
    printf(
        "Usage: %s <FMINDEXFILE> <XCLBIN> <TESTFILE> <NDRANGE> <LOCALSIZE>\n",
        argv[0]);
    return 1;
  }

  int use_ndrange = atoi(argv[4]);
  int local_size = atoi(argv[5]);

  // Load FM-index.
  fm_index *index = FMIndexReadFromFile(argv[1], 1);
  if (!index) {
    printf("Could not read FM-index from file.\n");
    return 1;
  }

  // Load test file.
  unsigned pattern_count, pattern_sz, max_match_count;
  char *patterns;

  if (!(LoadTestData(argv[3], &patterns, &pattern_count, &pattern_sz,
                     &max_match_count, 1))) {
    printf("Could not read test data file.\n");
    return 1;
  }

  // Initialize OpenCL.
  cl_int err;
  unsigned fileBufSize;
  std::vector<cl::Device> devices = get_xilinx_devices();
  devices.resize(1);
  cl::Device device = devices[0];
  cl::Context context(device, NULL, NULL, NULL, &err);
  char *fileBuf = read_binary_file(argv[2], fileBufSize);
  cl::Program::Binaries bins{{fileBuf, fileBufSize}};
  cl::Program program(context, devices, bins, NULL, &err);
  cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
  cl::Kernel kernel(program, "fmindex", &err);

  // Create OpenCL buffers for host data.
  cl::Buffer bwt_buf(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                     sizeof(char) * index->bwt_sz, index->bwt, &err);
  cl::Buffer alphabet_buf(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                          sizeof(char) * index->alphabet_sz, index->alphabet,
                          &err);
  cl::Buffer ranks_buf(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                       sizeof(ranks_t) * index->bwt_sz * index->alphabet_sz,
                       index->ranks, &err);
  cl::Buffer sa_buf(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                    sizeof(sa_t) * index->bwt_sz, index->sa, &err);
  cl::Buffer ranges_buf(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                        sizeof(ranges_t) * 2 * index->alphabet_sz,
                        index->ranges, &err);
  cl::Buffer patterns_buf(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                          sizeof(char) * pattern_count * pattern_sz, patterns,
                          &err);
  cl::Buffer out_buf(context, CL_MEM_WRITE_ONLY,
                     sizeof(unsigned long) *
                         (pattern_count * (max_match_count + 1)),
                     NULL, &err);

  // Map OpenCL buffers to kernel arguments.
  kernel.setArg(0, bwt_buf);
  kernel.setArg(1, alphabet_buf);
  kernel.setArg(2, ranks_buf);
  kernel.setArg(3, sa_buf);
  kernel.setArg(4, ranges_buf);
  kernel.setArg(5, patterns_buf);
  kernel.setArg(6, out_buf);

  // Map OpenCL buffers to host pointers.
  size_t *out = (size_t *)q.enqueueMapBuffer(
      out_buf, CL_TRUE, CL_MAP_READ, 0,
      sizeof(unsigned long) * pattern_count * max_match_count);

  // Map OpenCL buffers to kernel arguments including scalars.
  kernel.setArg(0, bwt_buf);
  kernel.setArg(1, alphabet_buf);
  kernel.setArg(2, ranks_buf);
  kernel.setArg(3, sa_buf);
  kernel.setArg(4, ranges_buf);
  kernel.setArg(5, patterns_buf);
  kernel.setArg(6, out_buf);
  kernel.setArg(7, index->bwt_sz);
  kernel.setArg(8, index->alphabet_sz);
  kernel.setArg(9, pattern_count);
  kernel.setArg(10, pattern_sz);
  kernel.setArg(11, max_match_count + 1);

  // Schedule transfer of inputs and output.
  q.enqueueMigrateMemObjects(
      {bwt_buf, alphabet_buf, ranks_buf, sa_buf, ranges_buf, patterns_buf}, 0);

  if (use_ndrange)
    q.enqueueNDRangeKernel(kernel, 0, pattern_count, local_size);
  else
    q.enqueueTask(kernel);

  q.enqueueMigrateMemObjects({out_buf}, CL_MIGRATE_MEM_OBJECT_HOST);

  q.finish();

  // Verify whether kernel calculations are correct.
  for (unsigned i = 0; i < pattern_count; ++i) {
    char *pattern = &patterns[i * pattern_sz];
    ranges_t start, end;
    FMIndexFindMatchRange(index, pattern, pattern_sz, &start, &end);
    unsigned long match_count = end - start;
    unsigned long *match_indices =
        (unsigned long *)calloc(match_count, sizeof(unsigned long));
    FMIndexFindRangeIndices(index, start, end, &match_indices);

    printf("Verifying pattern \"%.*s\": ", pattern_sz, pattern);
    unsigned long *results = &out[i * (max_match_count + 1)];
    if (results[0] != match_count) {
      printf("INCORRECT (match count %lu != %lu)\n", results[0], match_count);
      continue;
    }
    int success = 1;
    for (unsigned j = 0; j < match_count; j++)
      if (results[j + 1] != match_indices[j]) {
        printf("INCORRECT (index %lu != %lu)\n", results[j + 1],
               match_indices[j]);
        success = 0;
        break;
      }
    if (success)
      printf("CORRECT\n");
  }

  // Release resources.
  delete[] fileBuf;
  FMIndexFree(index);
  return EXIT_SUCCESS;
}

// ------------------------------------------------------------------------------------
// Utility functions
// ------------------------------------------------------------------------------------
std::vector<cl::Device> get_xilinx_devices() {
  size_t i;
  cl_int err;
  std::vector<cl::Platform> platforms;
  err = cl::Platform::get(&platforms);
  cl::Platform platform;
  for (i = 0; i < platforms.size(); i++) {
    platform = platforms[i];
    std::string platformName = platform.getInfo<CL_PLATFORM_NAME>(&err);
    if (platformName == "Xilinx") {
      std::cout << "INFO: Found Xilinx Platform" << std::endl;
      break;
    }
  }
  if (i == platforms.size()) {
    std::cout << "ERROR: Failed to find Xilinx platform" << std::endl;
    exit(EXIT_FAILURE);
  }

  // Getting ACCELERATOR Devices and selecting 1st such device
  std::vector<cl::Device> devices;
  err = platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices);
  return devices;
}

char *read_binary_file(const std::string &xclbin_file_name, unsigned &nb) {
  if (access(xclbin_file_name.c_str(), R_OK) != 0) {
    printf("ERROR: %s xclbin not available please build\n",
           xclbin_file_name.c_str());
    exit(EXIT_FAILURE);
  }
  // Loading XCL Bin into char buffer
  std::cout << "INFO: Loading '" << xclbin_file_name << "'\n";
  std::ifstream bin_file(xclbin_file_name.c_str(), std::ifstream::binary);
  bin_file.seekg(0, bin_file.end);
  nb = bin_file.tellg();
  bin_file.seekg(0, bin_file.beg);
  char *buf = new char[nb];
  bin_file.read(buf, nb);
  return buf;
}
