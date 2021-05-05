/**********
Copyright (c) 2018, Xilinx, Inc.
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/

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

// Forward declaration of utility functions included at the end of this file
std::vector<cl::Device> get_xilinx_devices();
char *read_binary_file(const std::string &xclbin_file_name, unsigned &nb);

// ------------------------------------------------------------------------------------
// Main program
// ------------------------------------------------------------------------------------
int main(int argc, char **argv) {
  if (argc < 4) {
    printf("Usage: %s <FMINDEXFILE> <XCLBIN> <TESTFILE>\n", argv[0]);
    return 1;
  }

  // Read FM-index from file.
  fm_index *index = FMIndexReadFromFile(argv[1], 1);
  if (!index) {
    printf("Could not read FM-index from file.\n");
    return 1;
  }

  unsigned pattern_count, pattern_sz, max_match_count;
  char *patterns;

  if (!(LoadTestData(argv[3], &patterns, &pattern_count, &pattern_sz,
                     &max_match_count))) {
    printf("Could not read test data file.\n");
    return 1;
  }

  // ------------------------------------------------------------------------------------
  // Step 1: Initialize the OpenCL environment
  // ------------------------------------------------------------------------------------
  cl_int err;
  std::string fmindexFile = argv[1];
  std::string binaryFile = argv[2];
  unsigned fileBufSize;
  std::vector<cl::Device> devices = get_xilinx_devices();
  devices.resize(1);
  cl::Device device = devices[0];
  cl::Context context(device, NULL, NULL, NULL, &err);
  char *fileBuf = read_binary_file(binaryFile, fileBufSize);
  cl::Program::Binaries bins{{fileBuf, fileBufSize}};
  cl::Program program(context, devices, bins, NULL, &err);
  cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
  cl::Kernel krnl_fmindex(program, "fmindex", &err);

  // ------------------------------------------------------------------------------------
  // Step 2: Create buffers and initialize test values
  // ------------------------------------------------------------------------------------
  // Create the buffers and allocate memory
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
                     sizeof(unsigned long) * max_match_count * pattern_count,
                     NULL, &err);

  // Map buffers to kernel arguments, thereby assigning them to specific device
  // memory banks
  krnl_fmindex.setArg(0, bwt_buf);
  krnl_fmindex.setArg(1, alphabet_buf);
  krnl_fmindex.setArg(2, ranks_buf);
  krnl_fmindex.setArg(3, sa_buf);
  krnl_fmindex.setArg(4, ranges_buf);
  krnl_fmindex.setArg(5, patterns_buf);
  krnl_fmindex.setArg(6, out_buf);

  // Map host-side buffer memory to user-space pointers
  size_t *out = (size_t *)q.enqueueMapBuffer(
      out_buf, CL_TRUE, CL_MAP_READ, 0,
      sizeof(unsigned long) * pattern_count * max_match_count);

  // ------------------------------------------------------------------------------------
  // Step 3: Run the kernel
  // ------------------------------------------------------------------------------------
  // Set kernel arguments
  // TODO: is this needed?
  krnl_fmindex.setArg(0, bwt_buf);
  krnl_fmindex.setArg(1, alphabet_buf);
  krnl_fmindex.setArg(2, ranks_buf);
  krnl_fmindex.setArg(3, sa_buf);
  krnl_fmindex.setArg(4, ranges_buf);
  krnl_fmindex.setArg(5, patterns_buf);
  krnl_fmindex.setArg(6, out_buf);
  krnl_fmindex.setArg(7, index->bwt_sz);
  krnl_fmindex.setArg(8, index->alphabet_sz);
  krnl_fmindex.setArg(9, pattern_count);
  krnl_fmindex.setArg(10, pattern_sz);
  krnl_fmindex.setArg(11, max_match_count);

  // Schedule transfer of inputs to device memory, execution of kernel, and
  // transfer of outputs back to host memory
  q.enqueueMigrateMemObjects(
      {bwt_buf, alphabet_buf, ranks_buf, sa_buf, ranges_buf, patterns_buf}, 0);
  q.enqueueTask(krnl_fmindex);
  q.enqueueMigrateMemObjects({out_buf}, CL_MIGRATE_MEM_OBJECT_HOST);

  // Wait for all scheduled operations to finish
  q.finish();

  for (unsigned i = 0; i < pattern_count; ++i) {
    printf("Pattern '%.*s' has indices: ", pattern_sz, &patterns[i * pattern_sz]);
    for (unsigned j = 0; j < max_match_count; ++j) {
      unsigned long match = out[i * max_match_count + j];
      if (match)
        printf("%lu ", match);
    }
    printf("\n");
  }

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
