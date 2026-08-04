TARGET = $(OFS_ASP_ROOT):ofs_iseries-dk_usm_noc

MHZ = 600MHz

EARLY = -fsycl-link=early 

SEED = 0

FLAGS = -fsycl -fPIC -fintelfpga -Xstarget=$(TARGET) -shared -Xshardware -Xsclock=$(MHZ) -Xsseed=$(SEED) -Xsv

COMMON = -I ./include

source: 	
	icpx $(FLAGS) $(COMMON) src/src.cpp -o src/src.so
	icpx $(FLAGS) $(COMMON) src/sink.cpp -o src/sink.so

source_early: 
	icpx $(FLAGS) $(EARLY) $(COMMON) src/src.cpp -o src/src_early.so
	icpx $(FLAGS) $(EARLY) $(COMMON) src/sink.cpp -o src/sink_early.so

s2: 
	icpx $(FLAGS) $(COMMON) src/src.cpp -o slot2/src.so
	icpx $(FLAGS) $(COMMON) src/sink.cpp -o slot2/sink.so

s3: 
	icpx $(FLAGS) $(COMMON) src/src.cpp -o slot3/src.so
	icpx $(FLAGS) $(COMMON) src/sink.cpp -o slot3/sink.so

s2_src:
	icpx $(FLAGS) $(COMMON) src/src.cpp -o slot2/src.so

s2_sink:
	icpx $(FLAGS) $(COMMON) src/sink.cpp -o slot2/sink.so

s3_src:
	icpx $(FLAGS) $(COMMON) src/src.cpp    -o slot3/src.so

s3_sink:
	icpx $(FLAGS) $(COMMON) src/sink.cpp    -o slot3/sink.so

s2_early:
	icpx $(FLAGS) $(EARLY) $(COMMON) src/src.cpp -o slot2/src_early.so
	icpx $(FLAGS) $(EARLY) $(COMMON) src/sink.cpp -o slot2/sink_early.so

s3_early:
	icpx $(FLAGS) $(EARLY) $(COMMON) src/src.cpp		-o slot3/src_early.so
	icpx $(FLAGS) $(EARLY) $(COMMON) src/sink.cpp		-o slot3/sink_early.so

all_early:
	make s2_early
	make s3_early

main: main.cpp
	icpx -fsycl -o main main.cpp

clean: 
	rm -rf slot2/*.so*
	rm -rf slot3/*.so*
  