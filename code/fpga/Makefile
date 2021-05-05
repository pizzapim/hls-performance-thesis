HOSTCODE := host.cpp
KERNELCODE := fmindex.cpp
FPGACONFIG := u250.cfg

ifndef TARGET
$(info TARGET is not set; using sw_emu.)
endif

TARGET ?= sw_emu

TARGETS := sw_emu hw_emu hw

ifeq ($(filter $(TARGET),$(TARGETS)),)
    $(error TARGET can only be one of: "$(TARGETS)")
endif

VXXFLAGS := --log_dir $(TARGET) --report_dir $(TARGET) --temp_dir $(TARGET)
GXXFLAGS := -Wall -g -std=c++11 -I${XILINX_XRT}/include/ -L${XILINX_XRT}/lib/ -lOpenCL -lpthread -lrt -lstdc++ -I..
XCLBIN := $(TARGET)/fmindex.xclbin

all: $(TARGET) host emconfig.json $(TARGET)/fmindex.xclbin

run: all
	test -n "$(FMINDEX)" || (echo "FMINDEX undefined" ; exit 1)
	test -n "$(TESTS)" || (echo "TESTS undefined" ; exit 1)
	cd $(TARGET) && XCL_EMULATION_MODE=$(TARGET) ../host ../$(FMINDEX) ../$(XCLBIN) ../$(TESTS)

$(TARGET):
	mkdir $(TARGET)

host.o: host.cpp ../fmindex.h
	g++ -c -o $@ $< $(GXXFLAGS)

host: host.o
	g++ -o $@ ../fmindex.o $^ $(GXXFLAGS)
	
$(TARGET)/fmindex.xo: $(KERNELCODE)
	v++ -c -t ${TARGET} --config $(FPGACONFIG) -k fmindex $(KERNELCODE) $(VXXFLAGS) -o $@ 

$(TARGET)/fmindex.xclbin: $(TARGET)/fmindex.xo
	v++ -l -t ${TARGET} --config $(FPGACONFIG) $(TARGET)/fmindex.xo $(VXXFLAGS) -o $@
	mv -t $(TARGET) xrc.log xcd.log

emconfig.json:
	emconfigutil --platform xilinx_u200_xdma_201830_2 --nd 1

clean:
	rm -rf emconfig.json *.info *.link_summary *.compile_summary \
	    *.xclbin *.xo *.log _x $(TARGET) host TempConfig *.csv *.run_summary \
	    *.o
