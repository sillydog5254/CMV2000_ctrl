# Usage with Vitis IDE:
# In Vitis IDE create a Single Application Debug launch configuration,
# change the debug type to 'Attach to running target' and provide this 
# tcl script in 'Execute Script' option.
# Path of this script: D:\Files\VivadoProjects\CMV2000_ctrl\vitis\CMV2000_ctrl_system\_ide\scripts\debugger_cmv2000_ctrl-default.tcl
# 
# 
# Usage with xsct:
# To debug using xsct, launch xsct and run below command
# source D:\Files\VivadoProjects\CMV2000_ctrl\vitis\CMV2000_ctrl_system\_ide\scripts\debugger_cmv2000_ctrl-default.tcl
# 
connect -url tcp:127.0.0.1:3121
targets -set -nocase -filter {name =~"APU*"}
rst -system
after 3000
targets -set -filter {jtag_cable_name =~ "Digilent JTAG-SMT2 210251A08870" && level==0 && jtag_device_ctx=="jsn-JTAG-SMT2-210251A08870-23727093-0"}
fpga -file D:/Files/VivadoProjects/CMV2000_ctrl/vitis/CMV2000_ctrl/_ide/bitstream/system_wrapper.bit
targets -set -nocase -filter {name =~"APU*"}
loadhw -hw D:/Files/VivadoProjects/CMV2000_ctrl/vitis/system_wrapper/export/system_wrapper/hw/system_wrapper.xsa -mem-ranges [list {0x40000000 0xbfffffff}] -regs
configparams force-mem-access 1
targets -set -nocase -filter {name =~"APU*"}
source D:/Files/VivadoProjects/CMV2000_ctrl/vitis/CMV2000_ctrl/_ide/psinit/ps7_init.tcl
ps7_init
ps7_post_config
targets -set -nocase -filter {name =~ "*A9*#0"}
dow D:/Files/VivadoProjects/CMV2000_ctrl/vitis/CMV2000_ctrl/Debug/CMV2000_ctrl.elf
configparams force-mem-access 0
targets -set -nocase -filter {name =~ "*A9*#0"}
con
