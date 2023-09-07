vlib modelsim_lib/work
vlib modelsim_lib/msim

vlib modelsim_lib/msim/xil_defaultlib

vmap xil_defaultlib modelsim_lib/msim/xil_defaultlib

vlog -work xil_defaultlib  -incr -mfcu  "+incdir+../../../../CMV2000_ctrl.gen/sources_1/bd/system/ipshared/ec67/hdl" "+incdir+../../../../CMV2000_ctrl.gen/sources_1/bd/system/ipshared/ee60/hdl" "+incdir+D:/ProgramFiles/Xilinx/Vivado/2022.2/data/xilinx_vip/include" \
"../../../bd/system/ip/system_processing_system7_0_0/sim/system_processing_system7_0_0.v" \
"../../../bd/system/ip/system_assert_0_0/sim/system_assert_0_0.v" \
"../../../bd/system/sim/system.v" \


vlog -work xil_defaultlib \
"glbl.v"

