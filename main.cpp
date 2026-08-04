#include <sycl/sycl.hpp>
#include <array>
#include <iostream>
#include <dlfcn.h>
#include <thread> // For std::this_thread::sleep_for
#include <chrono> // For std::chrono::seconds, milliseconds, etc.

#include <sycl/ext/intel/fpga_extensions.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <iomanip>
#include <functional>
#include <numeric>
#include <queue>
#include <random>
#include <string>
#include <type_traits>
#include <utility>
using namespace sycl;
using namespace std::chrono;


// Create an exception handler for asynchronous SYCL exceptions
static auto exception_handler = [](sycl::exception_list e_list) {
  for (std::exception_ptr const &e : e_list) {
    try {
      std::rethrow_exception(e);
    }
    catch (std::exception const &e) {
#if _DEBUG
      std::cout << "Failure" << std::endl;
#endif
      std::terminate();
    }
  }
};

struct alignas(64)block512 { // 512 bits 
  int a[16];
};

auto slot2_src_lib = dlopen("/home/jcheung2/ofs_fourslot/2024.1/iseries_apps/bringup/usm_iopipes_test/slot2/src.so", RTLD_NOW);
auto slot2_src = (event (*) (queue &q, block512*, size_t, uint8_t)) dlsym(slot2_src_lib, "src");

auto slot3_sink_lib = dlopen("/home/jcheung2/ofs_fourslot/2024.1/iseries_apps/bringup/usm_iopipes_test/slot3/sink.so", RTLD_NOW);
auto slot3_sink = (event (*) (queue &q, block512*, size_t)) dlsym(slot3_sink_lib, "sink");

// auto slot2_sink_lib = dlopen("/home/jcheung2/ofs_fourslot/2024.1/iseries_apps/bringup/usm_iopipes_test/slot2/sink.so", RTLD_NOW);
// auto slot2_sink = (event (*) (queue &q, block512*, size_t)) dlsym(slot2_sink_lib, "sink");

// auto slot3_src_lib = dlopen("/home/jcheung2/ofs_fourslot/2024.1/iseries_apps/bringup/usm_iopipes_test/slot3/src.so", RTLD_NOW);
// auto slot3_src = (event (*) (queue &q, block512*, size_t, uint8_t)) dlsym(slot3_src_lib, "src");





void PrintPerformanceInfo(std::string print_prefix, size_t count,
                          std::vector<double>& latency_ms,
                          std::vector<double>& process_time_ms) {
  // compute the input size in MB
  double input_size_megabytes = (sizeof(block512) * count) * 1e-6;

  // compute the average latency and processing time
  double iterations = latency_ms.size() - 1;
  double avg_latency_ms = std::accumulate(latency_ms.begin() + 1,
                                          latency_ms.end(),
                                          0.0) / iterations;
  double avg_processing_time_ms = std::accumulate(process_time_ms.begin() + 1,
                                                  process_time_ms.end(),
                                                  0.0) / iterations;

  // compute the throughput
  double avg_tp_mb_s = input_size_megabytes / (avg_processing_time_ms * 1e-3);

  // print info
  std::cout << std::fixed << std::setprecision(4);
  std::cout << print_prefix
            << " average latency:           " << avg_latency_ms << " ms\n";
  std::cout << print_prefix
            << " average throughput:        " << avg_tp_mb_s  << " MB/s\n";
}

// 3 -> 2
// void slot2sink_DoWorkSingleKernel(sycl::queue& q_src, sycl::queue& q_sink, block512* in, block512* out,
//                         size_t chunks, size_t chunk_count, size_t total_count,
//                         size_t inflight_kernels, size_t iterations) {
//   // timing data
//   std::vector<double> latency_ms(iterations);
//   std::vector<double> process_time_ms(iterations);

//   // count the number of chunks for which kernels have been started
//   size_t in_chunk = 0;

//   // count the number of chunks for which kernels have finished 
//   size_t out_chunk = 0;

//   // use a queue to track the kernels in flight
//   // By queueing multiple kernels before waiting on the oldest to finish
//   // (inflight_kernels) we still have kernels in the SYCL queue and ready to
//   // launch while we call event.wait() on the oldest kernel in the queue.
//   // However, if we set 'inflight_kernels' too high, then the time to launch
//   // the first set of kernels will be longer than the time for the first kernel
//   // to finish and our latency and throughput will be negatively affected.
//   std::queue<event> event_q;

//     for (size_t i = 0; i < iterations; i++) {
//     // reset the output data to catch any untouched data
//     // std::fill_n(out, total_count, -1);
//     for (size_t j = 0; j < total_count; j++) {
//       for (uint8_t k = 0; k < 8; k++) { 
//         out[j].a[k] = 0;
//       }
//     }

//     // reset counters
//     in_chunk = 0;
//     out_chunk = 0;

//     // clear the queue
//     std::queue<event> clear_q;
//     std::swap(event_q, clear_q);

//     // latency timers
//     high_resolution_clock::time_point first_data_in, first_data_out;

//     auto start = high_resolution_clock::now();

//     do {
//       // if we still have kernels to launch, launch them in here
//       if (in_chunk < chunks) {
//         // launch the kernel
//         size_t chunk_offset = in_chunk*chunk_count; 
//         // this function is defined in 'single_kernel.hpp'
//         auto e = slot2_sink(q_sink, out + chunk_offset, chunk_count);
//         uint8_t slot2_address = 2;
//         slot3_src(q_src, in + chunk_offset, chunk_count, slot2_address);

//         // push the kernel event into the queue
//         event_q.push(e);

//         // if this is the first chunk, track the time
//         if (in_chunk == 0) first_data_in = high_resolution_clock::now();
//         in_chunk++;
//       }

//       // wait on the earliest kernel to finish if either condition is met:
//       //    1) there are a certain number kernels in flight
//       //    2) all of the kernels have been launched
//       if ((event_q.size() >= inflight_kernels) || (in_chunk >= chunks)) {
//         // pop the earliest kernel event we are waiting on
//         auto e = event_q.front();
//         event_q.pop();

//         // wait on it to finish
//         e.wait();

//         // track the time if this is the first producer/consumer pair
//         if (out_chunk == 0) first_data_out = high_resolution_clock::now();

//         // The synchronization of the kernels ending tells us that, at this 
//         // point, the first 'out_chunk' chunks are valid on the host.
//         // NOTE: This is the point where you would consume the output data
//         // at (out + out_chunk*chunk_size).
//         out_chunk++;
//       }
//     } while (out_chunk < chunks);

//     auto end = high_resolution_clock::now();

//     // compute latency and processing time
//     duration<double, std::milli> latency = first_data_out - first_data_in;
//     duration<double, std::milli> process_time = end - start;
//     latency_ms[i] = latency.count();
//     process_time_ms[i] = process_time.count();
//   }

//   // compute and print timing information
//   PrintPerformanceInfo("3->2 Single-kernel",
//                           total_count, latency_ms, process_time_ms);
// }

// 2 -> 3
void slot3sink_DoWorkSingleKernel(sycl::queue& q_src, sycl::queue& q_sink, block512* in, block512* out,
                        size_t chunks, size_t chunk_count, size_t total_count,
                        size_t inflight_kernels, size_t iterations) {
  // timing data
  std::vector<double> latency_ms(iterations);
  std::vector<double> process_time_ms(iterations);

  // count the number of chunks for which kernels have been started
  size_t in_chunk = 0;

  // count the number of chunks for which kernels have finished 
  size_t out_chunk = 0;

  // use a queue to track the kernels in flight
  // By queueing multiple kernels before waiting on the oldest to finish
  // (inflight_kernels) we still have kernels in the SYCL queue and ready to
  // launch while we call event.wait() on the oldest kernel in the queue.
  // However, if we set 'inflight_kernels' too high, then the time to launch
  // the first set of kernels will be longer than the time for the first kernel
  // to finish and our latency and throughput will be negatively affected.
  std::queue<event> event_q;

    for (size_t i = 0; i < iterations; i++) {
    // reset the output data to catch any untouched data
    // std::fill_n(out, total_count, -1);
    for (size_t j = 0; j < total_count; j++) {
      for (uint8_t k = 0; k < 8; k++) { 
        out[j].a[k] = 0;
      }
    }

    // reset counters
    in_chunk = 0;
    out_chunk = 0;

    // clear the queue
    std::queue<event> clear_q;
    std::swap(event_q, clear_q);

    // latency timers
    high_resolution_clock::time_point first_data_in, first_data_out;

    auto start = high_resolution_clock::now();

    do {
      // if we still have kernels to launch, launch them in here
      if (in_chunk < chunks) {
        // launch the kernel
        size_t chunk_offset = in_chunk*chunk_count; 
        // this function is defined in 'single_kernel.hpp'
        auto e = slot3_sink(q_sink, out + chunk_offset, chunk_count);
        uint8_t slot3_address = 3;
        slot2_src(q_src, in + chunk_offset, chunk_count, slot3_address);

        // push the kernel event into the queue
        event_q.push(e);

        // if this is the first chunk, track the time
        if (in_chunk == 0) first_data_in = high_resolution_clock::now();
        in_chunk++;
      }

      // wait on the earliest kernel to finish if either condition is met:
      //    1) there are a certain number kernels in flight
      //    2) all of the kernels have been launched
      if ((event_q.size() >= inflight_kernels) || (in_chunk >= chunks)) {
        // pop the earliest kernel event we are waiting on
        auto e = event_q.front();
        event_q.pop();

        // wait on it to finish
        e.wait();

        // track the time if this is the first producer/consumer pair
        if (out_chunk == 0) first_data_out = high_resolution_clock::now();

        // The synchronization of the kernels ending tells us that, at this 
        // point, the first 'out_chunk' chunks are valid on the host.
        // NOTE: This is the point where you would consume the output data
        // at (out + out_chunk*chunk_size).
        out_chunk++;
      }
    } while (out_chunk < chunks);

    auto end = high_resolution_clock::now();

    // compute latency and processing time
    duration<double, std::milli> latency = first_data_out - first_data_in;
    duration<double, std::milli> process_time = end - start;
    latency_ms[i] = latency.count();
    process_time_ms[i] = process_time.count();
  }

  // compute and print timing information
  PrintPerformanceInfo("2->3 Single-kernel",
                          total_count, latency_ms, process_time_ms);
}

int main() {

  std::cout << "Starting...\n";
  auto platforms = sycl::platform::get_platforms();
  
  for (auto platform : sycl::platform::get_platforms())
  {
      std::cout << "\n\n\n\nPlatform: "
                << platform.get_info<sycl::info::platform::name>()
                << std::endl;

      for (auto device : platform.get_devices())
      {
          std::cout << "\n\n\n\n\t****************Device: "
                    << device.get_info<sycl::info::device::name>()
                    << std::endl;
      }
  }


  size_t chunks = 1 << 2;         // 512
  size_t chunk_count = 1 << 21;   // 32768
  size_t iterations = 10;
  size_t inflight_kernels = 2;

  // compute the total number of elements
  size_t total_count = chunks * chunk_count;
  // size_t total_count = 4;

  bool passed = true;

  std::cout << "Creating queues...\n";

  // queue properties to enable profiling
  property_list prop_list { property::queue::enable_profiling() };
  queue q2(platforms[1].get_devices()[2], exception_handler, prop_list);  
  queue q3(platforms[1].get_devices()[3], exception_handler, prop_list);  

  auto device2 = q2.get_device();
  auto device3 = q3.get_device();

  if (!device2.get_info<info::device::usm_host_allocations>()) {
    std::cerr << "ERROR: The device does not support USM host"
              << " allocations\n";
    std::terminate();
  }

  if (!device3.get_info<info::device::usm_host_allocations>()) {
    std::cerr << "ERROR: The device does not support USM host"
              << " allocations\n";
    std::terminate();
  }

  block512 *in_2, *out_2;
  block512 *in_3, *out_3;

  if ((in_2 = malloc_host<block512>(total_count, q2)) == nullptr) {
    std::cerr << "ERROR: could not allocate space for 'in_2'\n";
    std::terminate();
  }
  if ((out_2 = malloc_host<block512>(total_count, q2)) == nullptr) {
    std::cerr << "ERROR: could not allocate space for 'out_2'\n";
    std::terminate();
  }
  if ((in_3 = malloc_host<block512>(total_count, q3)) == nullptr) {
    std::cerr << "ERROR: could not allocate space for 'in_3'\n";
    std::terminate();
  }
  if ((out_3 = malloc_host<block512>(total_count, q3)) == nullptr) {
    std::cerr << "ERROR: could not allocate space for 'out_3'\n";
    std::terminate();
  }


  for (size_t i = 0; i < total_count; i++) {

    for (uint8_t j = 0; j < 8; j++) { 
      in_2[i].a[j] = j;
      out_2[i].a[j] = 0;

      in_3[i].a[j] = j;
      out_3[i].a[j] = 0;
    }
  }


  std::cout << "# Chunks:             " << chunks << "\n";
  std::cout << "Chunk count:          " << chunk_count << "\n";
  std::cout << "Total count:          " << total_count << "\n";
  std::cout << "Iterations:           " << iterations-1 << "\n";
  std::cout << "\n";

  ////////////////////////////////////////////////

  // 2-> 3

  slot3sink_DoWorkSingleKernel(q2, q3, in_2, out_3, chunks, chunk_count, total_count,
                      inflight_kernels, iterations);

  for (size_t i = 0; i < total_count; i++) {

    for (uint8_t j = 0; j < 8; j++) { 
      int a = in_2[i].a[j];
      int b = out_3[i].a[j];
      if (b != a) {
        std::cout << "2->3 ERROR: Values do not match, "
                  << "in_2[" << i << "].a[" << j << "]:" << a
                  << " != out_3[" << i << "].a[" << j << "]:" << b
                  << "\n";
        passed = false;
        // break;
      }
      // else {
      //         std::cout << "2->3 Match, "
      //             << "in_2[" << i << "].a[" << j << "]:" << a
      //             << " == out_3[" << i << "].a[" << j << "]:" << b
      //             << "\n";
      // }
    }

  }

  ////////////////////////////////////////////////
  // 3->2

  // slot2sink_DoWorkSingleKernel(q3, q2, in_3, out_2, chunks, chunk_count, total_count,
  //                     inflight_kernels, iterations);

  // for (size_t i = 0; i < total_count; i++) {

  //   for (uint8_t j = 0; j < 8; j++) { 
  //     int a = in_3[i].a[j];
  //     int b = out_2[i].a[j];
  //     if (b != a) {
  //       std::cout << "3->2 ERROR: Values do not match, "
  //                 << "in_3[" << i << "].a[" << j << "]:" << a
  //                 << " != out_2[" << i << "].a[" << j << "]:" << b
  //                 << "\n";
  //       passed = false;
  //       // break;
  //     }
  //   }
  // }

  ////////////////////////////////////////////////


  if (!passed) {
    std::cout << "Test failed.\n";
  } else {
    std::cout << "Test passed.\n";
  }


  std::cout << "Successfully completed on device.\n";        


  return 0;
}

