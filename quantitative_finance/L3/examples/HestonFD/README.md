
# Setup Environment

source /opt/xilinx/xrt/setup.csh

source /*path to xf_fintech*/L3/src/env.csh

# Build Xilinx Fintech Library

cd  /*path to xf_fintech*/L3/src

**make clean all**


# Build Instuctions

To build the command line executable (heston_cl) from this directory

**make clean all**

> Note this requires the xilinx fintech library to already to built


# Run Instuctions

Copy the prebuild kernel files to this directory

**fd.xclbin**


To run the command line exe and generate the interpolated NPV

>output/hestonfd_cl.exe kappa eta sigma rho rd T K S V N m1 m2

or with default arguments

**make run**
