ifndef PLATFORM
    $(error PLATFORM is not set.)
endif

ifndef TARGET
    $(info TARGET is not set; using sw_emu.)
endif

TARGET ?= sw_emu

TARGETS := sw_emu hw_emu hw

ifeq ($(filter $(TARGET),$(TARGETS)),)
    $(error TARGET can only be one of: "$(TARGETS)")
endif

VXXFLAGS := -t ${TARGET} --log_dir $(TARGET) --report_dir $(TARGET) --temp_dir $(TARGET) --config fmindex.cfg -I/usr/include/x86_64-linux-gnu -Wno-unused-label
GXXFLAGS := -Wall -g -std=c++11 -I${XILINX_XRT}/include/ -L${XILINX_XRT}/lib/ -lOpenCL -lpthread -lrt -lstdc++ -I..
PROJ_HEADERS := ../fmindex.h ../util.h
PROJ_OBJS := ../fmindex.o ../util.o

ifeq ($(TARGET), hw)
	EMULATION_FLAG :=
else
	EMULATION_FLAG := XCL_EMULATION_MODE=$(TARGET)
endif

.PHONY: all run-verify clean cleanall unopt opt

all: $(TARGET) verify benchmark unopt opt

unopt: $(TARGET)/unopt.xclbin

opt: $(TARGET)/opt.xclbin

run-verify: verify $(TARGET)/$(KERNEL).xclbin
	@test -n "$(FMFILE)" || (echo "FMFILE undefined" ; exit 1)
	@test -n "$(TESTFILE)" || (echo "TESTFILE undefined" ; exit 1)
	@test -n "$(KERNEL)" || (echo "KERNEL undefined" ; exit 1)
	cd $(TARGET) && $(EMULATION_FLAG) ../verify ../$(FMFILE) ../$(TARGET)/$(KERNEL).xclbin ../$(TESTFILE)

$(TARGET):
	mkdir $(TARGET)

%.o: %.cpp $(PROJ_HEADERS)
	g++ -c -o $@ $< $(GXXFLAGS)

verify: verify.o
	g++ -o $@ $(PROJ_OBJS) $< $(GXXFLAGS)

benchmark: benchmark.o
	g++ -o $@ $(PROJ_OBJS) $< $(GXXFLAGS)
	
$(TARGET)/%.xo: %.cl
	v++ -c -k fmindex $< $(VXXFLAGS) -o $@

$(TARGET)/%.xclbin: $(TARGET)/%.xo emconfig.json
	v++ -l $< $(VXXFLAGS) -o $@
	mv -t $(TARGET) xrc.log xcd.log

emconfig.json:
	emconfigutil --platform $(PLATFORM) --nd 1

clean:
	rm -rf emconfig.json *.info *.link_summary *.compile_summary \
	    *.xclbin *.xo *.log _x $(TARGET) verify TempConfig *.csv *.run_summary \
	    *.o .Xil .run .ipcache

cleanall: clean
	rm -rf $(TARGETS)
