#include <sycl/sycl.hpp>
#include <sycl/ext/intel/fpga_extensions.hpp>

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

using write_iopipe = ext::intel::kernel_writeable_io_pipe<pipe_id<0>, block1056, 0>;

extern "C" {

  // submit the kernel for the single-kernel design
  event src(queue &q, block512 *in_ptr, size_t count, uint8_t dest_addr) {
    return q.submit([&](handler &h) {

      h.single_task<class src_test>([=]() [[intel::kernel_args_restrict]] {
        // using a host_ptr class tells the compiler that this pointer lives in
        // the hosts address space
        sycl::ext::intel::host_ptr<block512> in(in_ptr);

        block1056 packet;
        block512 data;

        packet.dest_addr = dest_addr;

        [[intel::initiation_interval(1)]]
        for (size_t i = 0; i < count; i++) {
          // do a simple copy - more complex computation can go here
          data = *(in + i);
          #pragma unroll
          for (int j = 0; j < 16; j++) {
            packet.data[j] = data.a[j];
          }
          write_iopipe::write(packet);

        }
      });

    });
  }
}
