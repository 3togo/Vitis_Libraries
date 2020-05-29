/*
 * Copyright 2019 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "xf_reduce_config.h"

void reduce_accel(xf::cv::Mat<SRC_T, HEIGHT, WIDTH, NPIX>& _src,
                  xf::cv::Mat<DST_T, ONE_D_HEIGHT, ONE_D_WIDTH, XF_NPPC1>& _dst,
                  unsigned char dim) {
    xf::cv::reduce<REDUCTION_OP, SRC_T, DST_T, HEIGHT, WIDTH, ONE_D_HEIGHT, ONE_D_WIDTH, NPIX>(_src, _dst, dim);
}
