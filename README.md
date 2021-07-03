# A Performance Analysis of High-Level Synthesis for FPGAs

This repository stores the code and latex of my bachelor's thesis. The full thesis PDF can be found in [hls_performance_thesis.pdf](hls_performance_thesis.pdf).

## Abstract

It is becoming increasingly popular to use Field-Programmable Gate Arrays (FPGAs)
as hardware accelerators in order to speed up certain parts of an algorithm. FPGAs
promise more energy efficiency and increased performance compared to CPUs. They are,
however, traditionally programmed with Hardware Description Languages (HDLs), which
are notoriously hard to use.
To accelerate their adoption in different contexts and domains, FPGAs can nowadays
also be programmed with high-level languages, such as C/C++ or OpenCL, in a process
called High-Level Synthesis (HLS). However, it can be a challenge to efficiently accelerate
algorithms using HLS.
In this thesis, we investigate the performance of HLS for a non-trivial case-study. To
this end, we devise a performance comparison between a sequential CPU algorithm and its
FPGA version, programmed with OpenCL for the Xilinx Vitis platform. Our case study is a
string-searching algorithm using the FM-index method, which is able to efficiently locate
substrings in arbitrarily long texts.
Our naive reference implementation is a simple port from CPU to OpenCL. In search
for more performance, on top of this naive OpenCL implementation, we also propose and
evaluate two FPGA-specific optimizations, suggested by literature.
Our empirical analysis shows that optimizations can greatly improve the performance of
algorithms programmed with HLS for FPGAs. However, the performance of our HLS-based
FPGA version for our case study could not match the performance of the CPU, despite
being more work efficient.
