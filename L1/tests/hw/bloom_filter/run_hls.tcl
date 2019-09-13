#
# Copyright 2019 Xilinx, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

source settings.tcl
open_project -reset "bloom_filter.prj"

add_files bloom_filter_test.cpp -cflags "-I${XF_PROJ_ROOT}/L1/include/hw"
add_files -tb bloom_filter_test.cpp -cflags "-I${XF_PROJ_ROOT}/L1/include/hw"

open_solution -reset "bram_solution"
set_top syn_bloom_filter_bram
set_part xcku5p-sfvb784-2-i
create_clock -period 3.0

if {$CSIM == 1} {
  csim_design -argv "3"
}
if {$CSYNTH == 1} {
  csynth_design
}
if {$COSIM == 1} {
  cosim_design -argv "3"
}
if {$VIVADO_SYN == 1} {
  export_design -flow syn -rtl verilog
}

if {$VIVADO_IMPL == 1} {
  export_design -flow impl -rtl verilog
}

if {$QOR_CHECK == 1} {
  puts "QoR check not implemented yet"
}

open_solution -reset "bram_strm_solution"
set_top syn_bloom_filter_bram_and_strm
set_part xcku5p-sfvb784-2-i
create_clock -period 3.0

if {$CSIM == 1} {
  csim_design -argv "4"
}
if {$CSYNTH == 1} {
  csynth_design
}
if {$COSIM == 1} {
  cosim_design -argv "4"
}
if {$VIVADO_SYN == 1} {
  export_design -flow syn -rtl verilog
}

if {$VIVADO_IMPL == 1} {
  export_design -flow impl -rtl verilog
}

if {$QOR_CHECK == 1} {
  puts "QoR check not implemented yet"
}


open_solution -reset "uram_solution"
set_top syn_bloom_filter_uram
set_part xcku5p-sfvb784-2-i
create_clock -period 3.0

if {$CSIM == 1} {
  csim_design -argv "1"
}
if {$CSYNTH == 1} {
  csynth_design
}
if {$COSIM == 1} {
  cosim_design -argv "1"
}
if {$VIVADO_SYN == 1} {
  export_design -flow syn -rtl verilog
}

if {$VIVADO_IMPL == 1} {
  export_design -flow impl -rtl verilog
}

if {$QOR_CHECK == 1} {
  puts "QoR check not implemented yet"
}


open_solution -reset "uram_strm_solution"
set_top syn_bloom_filter_uram_and_strm
set_part xcku5p-sfvb784-2-i
create_clock -period 3.0

if {$CSIM == 1} {
  csim_design -argv "2"
}
if {$CSYNTH == 1} {
  csynth_design
}
if {$COSIM == 1} {
  cosim_design -argv "2"
}
if {$VIVADO_SYN == 1} {
  export_design -flow syn -rtl verilog
}

if {$VIVADO_IMPL == 1} {
  export_design -flow impl -rtl verilog
}

if {$QOR_CHECK == 1} {
  puts "QoR check not implemented yet"
}
exit
