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

VXXFLAGS := -t ${TARGET} --log_dir $(TARGET) --report_dir $(TARGET) --temp_dir $(TARGET) -I/usr/include/x86_64-linux-gnu -Wno-unused-label
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

ndrange: $(TARGET)/ndrange.xclbin

ndrange2: $(TARGET)/ndrange2.xclbin

memory: $(TARGET)/memory.xclbin

final: $(TARGET)/final.xclbin

run-verify: verify $(TARGET)/$(KERNEL).xclbin
	@test -n "$(FMFILE)" || (echo "FMFILE undefined" ; exit 1)
	@test -n "$(TESTFILE)" || (echo "TESTFILE undefined" ; exit 1)
	@test -n "$(KERNEL)" || (echo "KERNEL undefined" ; exit 1)
	@test -n "$(NDRANGE)" || (echo "NDRANGE undefined" ; exit 1)
	@test -n "$(LOCALSIZE)" || (echo "LOCALSIZE undefined" ; exit 1)
	cd $(TARGET) && $(EMULATION_FLAG) ../verify ../$(FMFILE) ../$(TARGET)/$(KERNEL).xclbin ../$(TESTFILE) $(NDRANGE) $(LOCALSIZE)

$(TARGET):
	mkdir $(TARGET)

%.o: %.cpp $(PROJ_HEADERS)
	g++ -c -o $@ $< $(GXXFLAGS)

verify: verify.o
	g++ -o $@ $(PROJ_OBJS) $< $(GXXFLAGS)

benchmark: benchmark.o
	g++ -o $@ $(PROJ_OBJS) $< $(GXXFLAGS)

$(TARGET)/unopt.xo: unopt.cl no_ndrange.cfg
	v++ -c -k fmindex $< $(VXXFLAGS) --config no_ndrange.cfg -o $@

$(TARGET)/unopt.xclbin: $(TARGET)/unopt.xo emconfig.json
	v++ -l $< $(VXXFLAGS) --config no_ndrange.cfg -o $@
	mv -t $(TARGET) xrc.log xcd.log

$(TARGET)/memory.xo: memory.cl no_ndrange.cfg
	v++ -c -k fmindex $< $(VXXFLAGS) --config no_ndrange.cfg -o $@

$(TARGET)/memory.xclbin: $(TARGET)/memory.xo emconfig.json
	v++ -l $< $(VXXFLAGS) --config no_ndrange.cfg -o $@
	mv -t $(TARGET) xrc.log xcd.log

$(TARGET)/ndrange.xo: ndrange.cl ndrange.cfg
	v++ -c -k fmindex $< $(VXXFLAGS) --config ndrange.cfg -o $@

$(TARGET)/ndrange.xclbin: $(TARGET)/ndrange.xo emconfig.json
	v++ -l $< $(VXXFLAGS) --config ndrange.cfg -o $@
	mv -t $(TARGET) xrc.log xcd.log

$(TARGET)/ndrange2.xo: ndrange2.cl ndrange2.cfg
	v++ -c -k fmindex $< $(VXXFLAGS) --config ndrange2.cfg -o $@

$(TARGET)/ndrange2.xclbin: $(TARGET)/ndrange2.xo emconfig.json
	v++ -l $< $(VXXFLAGS) --config ndrange2.cfg -o $@
	mv -t $(TARGET) xrc.log xcd.log

$(TARGET)/final.xo: final.cl ndrange.cfg
	v++ -c -k fmindex $< $(VXXFLAGS) --config ndrange.cfg -o $@

$(TARGET)/final.xclbin: $(TARGET)/final.xo emconfig.json
	v++ -l $< $(VXXFLAGS) --config ndrange.cfg -o $@
	mv -t $(TARGET) xrc.log xcd.log

emconfig.json:
	emconfigutil --platform $(PLATFORM) --nd 1

clean:
	rm -rf emconfig.json *.info *.link_summary *.compile_summary \
	    *.xclbin *.xo *.log _x $(TARGET) verify TempConfig *.csv *.run_summary \
	    *.o .Xil .run .ipcache

cleanall: clean
	rm -rf $(TARGETS)
