#include <sycl/sycl.hpp>
#include <sycl/ext/intel/fpga_extensions.hpp>

#define PIPELINE_STAGES 1

using namespace sycl;
using namespace std;

struct alignas(64) block512 { // 512 bits     
  int a[16];
};

struct block1056 { // 1056 bits 
  uint8_t dest_addr; // dest_addr is expected to be the first field
  uint8_t src_addr;
  uint8_t user;
  uint8_t byte_pad;
  int pad[16];
  int data[16];
};

template <unsigned ID>
struct pipe_id {
  static constexpr unsigned id = ID;
};

using read_iopipe = ext::intel::kernel_readable_io_pipe<pipe_id<1>, block1056, 0>;

extern "C" {

  // submit the kernel for the single-kernel design
  event sink(queue &q, block512 *out_ptr, size_t count) {
    return q.submit([&](handler &h) {

      h.single_task<class sink_test>([=]() [[intel::kernel_args_restrict]] {
        // using a host_ptr class tells the compiler that this pointer lives in
        // the hosts address space
        sycl::ext::intel::host_ptr<block512> out(out_ptr);

        // block512 regs[PIPELINE_STAGES];
        block1056 packet;
        block512 data;

        [[intel::initiation_interval(1)]]
        // for (size_t i = 0; i < count + PIPELINE_STAGES; i++) {
        for (size_t i = 0; i < count; i++) {
          // do a simple copy - more complex computation can go here
          packet = read_iopipe::read(); 
          #pragma unroll
          for (int j = 0; j < 16; j++) {
            data.a[j] = packet.data[j];
          }
          *(out + i) = data;

          // if (i < count) {
          //   packet = read_iopipe::read();
          // }

          // if (i >= PIPELINE_STAGES) {
          //   *(out + i - PIPELINE_STAGES) = regs[PIPELINE_STAGES - 1];
          // }

          // #pragma unroll
          // for (uint16_t stage = PIPELINE_STAGES-1; stage > 0; stage--) {
          //   regs[stage] = regs[stage - 1];
          // }

          // regs[0] = packet.data;
        
        }
      });

    });
  }
  
}
