/***************************************************************************
Copyright (c) 2019, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ***************************************************************************/

#include "common/xf_headers.h"
#include "xf_autowhitebalance_config.h"

template <typename T>
void balanceWhiteSimple(std::vector<cv::Mat_<T> >& src,
                        cv::Mat& dst,
                        const float inputMin,
                        const float inputMax,
                        const float outputMin,
                        const float outputMax,
                        const float p) {
    /********************* Simple white balance *********************/
    const float s1 = p; // low quantile
    const float s2 = p; // high quantile

    int depth = 2; // depth of histogram tree
    if (src[0].depth() != CV_8U) ++depth;
    int bins = 16; // number of bins at each histogram level

    int nElements = int(pow((float)bins, (float)depth));

    int countval = 0;

    for (size_t i = 0; i < src.size(); ++i) {
        std::vector<int> hist(nElements, 0);

        typename cv::Mat_<T>::iterator beginIt = src[i].begin();
        typename cv::Mat_<T>::iterator endIt = src[i].end();

        for (typename cv::Mat_<T>::iterator it = beginIt; it != endIt; ++it) // histogram filling
        {
            int pos = 0;
            float minValue = inputMin - 0.5f;
            float maxValue = inputMax + 0.5f;
            T val = *it;

            float interval = float(maxValue - minValue) / bins;

            for (int j = 0; j < depth; ++j) {
                int currentBin = int((val - minValue + 1e-4f) / interval);
                ++hist[pos + currentBin];

                pos = (pos + currentBin) * bins;

                minValue = minValue + currentBin * interval;
                maxValue = minValue + interval;

                interval /= bins;
            }

            countval++;
        }

        int total = int(src[i].total());

        int p1 = 0, p2 = bins - 1;
        int n1 = 0, n2 = total;

        float minValue = inputMin - 0.5f;
        float maxValue = inputMax + 0.5f;

        float interval = (maxValue - minValue) / float(bins);

        for (int j = 0; j < depth; ++j)
        // searching for s1 and s2
        {
            while (n1 + hist[p1] < s1 * total / 100.0f) {
                n1 += hist[p1++];
                minValue += interval;

                // printf("ocv min vlaues:%f %d\n",minValue,n1);
            }
            p1 *= bins;

            while (n2 - hist[p2] > (100.0f - s2) * total / 100.0f) {
                n2 -= hist[p2--];
                maxValue -= interval;
            }
            p2 = (p2 + 1) * bins - 1;

            interval /= bins;
        }

        src[i] = (outputMax - outputMin) * (src[i] - minValue) / (maxValue - minValue) + outputMin;
    }

    /****************************************************************/

    dst.create(/**/ src[0].size(), CV_MAKETYPE(src[0].depth(), int(src.size())) /**/);
    cv::merge(src, dst);

    printf("\ncountvalue:%d\n", countval);
}

void calculateChannelSums(uint& sumB, uint& sumG, uint& sumR, uchar* src_data, int src_len, float thresh) {
    sumB = sumG = sumR = 0;
    ushort thresh255 = (ushort)cvRound(thresh * 255);
    int i = 0;
    unsigned int minRGB, maxRGB;
    for (; i < src_len; i += 3) {
        minRGB = std::min(src_data[i], std::min(src_data[i + 1], src_data[i + 2]));
        maxRGB = std::max(src_data[i], std::max(src_data[i + 1], src_data[i + 2]));
        if ((maxRGB - minRGB) * 255 > thresh255 * maxRGB) continue;
        sumB += src_data[i];
        sumG += src_data[i + 1];
        sumR += src_data[i + 2];
    }
}

void applyChannelGains(cv::Mat& src, cv::Mat& dst, float gainB, float gainG, float gainR) {
    int N3 = 3 * src.cols * src.rows;
    int i = 0;

    // Scale gains by their maximum (fixed point approximation works only when all gains are <=1)
    float gain_max = std::max(gainB, std::max(gainG, gainR));
    if (gain_max > 0) {
        gainB /= gain_max;
        gainG /= gain_max;
        gainR /= gain_max;
    }

    if (src.type() == CV_8UC3) {
        // Fixed point arithmetic, mul by 2^8 then shift back 8 bits
        int i_gainB = cvRound(gainB * (1 << 8)), i_gainG = cvRound(gainG * (1 << 8)),
            i_gainR = cvRound(gainR * (1 << 8));
        const uchar* src_data = src.ptr<uchar>();
        uchar* dst_data = dst.ptr<uchar>();
        for (; i < N3; i += 3) {
            dst_data[i] = (uchar)((src_data[i] * i_gainB) >> 8);
            dst_data[i + 1] = (uchar)((src_data[i + 1] * i_gainG) >> 8);
            dst_data[i + 2] = (uchar)((src_data[i + 2] * i_gainR) >> 8);
        }
    }
}

void balanceWhiteGW(cv::Mat& src, cv::Mat& dst) {
    int N = src.cols * src.rows, N3 = N * 3;

    double dsumB = 0.0, dsumG = 0.0, dsumR = 0.0;
    float thresh = 0.9f;

    uint sumB = 0, sumG = 0, sumR = 0;
    calculateChannelSums(sumB, sumG, sumR, src.ptr<uchar>(), N3, thresh);
    dsumB = (double)sumB;
    dsumG = (double)sumG;
    dsumR = (double)sumR;

    // Find inverse of averages
    double max_sum = std::max(dsumB, std::max(dsumR, dsumG));
    const double eps = 0.1;
    float dinvB = dsumB < eps ? 0.f : (float)(max_sum / dsumB), dinvG = dsumG < eps ? 0.f : (float)(max_sum / dsumG),
          dinvR = dsumR < eps ? 0.f : (float)(max_sum / dsumR);

    // Use the inverse of averages as channel gains:
    applyChannelGains(src, dst, dinvB, dinvG, dinvR);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path1> \n");
        return -1;
    }

    cv::Mat in_gray, in_gray1, ocv_ref, out_gray, diff, ocv_ref_in1, ocv_ref_in2, inout_gray1, ocv_ref_gw;
    in_gray = cv::imread(argv[1], 1); // read image
    if (in_gray.data == NULL) {
        fprintf(stderr, "Cannot open image %s\n", argv[1]);
        return -1;
    }

    ocv_ref.create(in_gray.rows, in_gray.cols, CV_8UC3);
    ocv_ref_gw.create(in_gray.rows, in_gray.cols, CV_8UC3);
    out_gray.create(in_gray.rows, in_gray.cols, CV_8UC3);
    diff.create(in_gray.rows, in_gray.cols, CV_8UC3);

    /// simple white balancing cref code
    float inputMin = 0.0f;
    float inputMax = 255.0f;
    float outputMin = 0.0f;
    float outputMax = 255.0f;
    float p = 2.0f;

    float thresh = 0.9;

    static xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC1> imgInput1(in_gray.rows, in_gray.cols);
    static xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC1> imgOutput(out_gray.rows, out_gray.cols);

    imgInput1.copyTo(in_gray.data);
    xf::cv::imwrite("in_hls.jpg", imgInput1);

    autowhitebalance_accel(imgInput1, imgInput1, imgOutput, thresh);

    out_gray.data = imgOutput.copyFrom();

    imwrite("out_hls.jpg", out_gray);

    std::vector<cv::Mat_<uchar> > mv;
    split(in_gray, mv);

    // simple white balancing algorithm
    balanceWhiteSimple(mv, ocv_ref, inputMin, inputMax, outputMin, outputMax, p);

    // gray world white balancing algorithm
    balanceWhiteGW(in_gray, ocv_ref_gw);

    imwrite("ocv_gw.png", ocv_ref_gw);

    imwrite("ocv_simple.png", ocv_ref);

    if (WB_TYPE == 0) {
        // Compute absolute difference image
        cv::absdiff(ocv_ref_gw, out_gray, diff);
    } else {
        // Compute absolute difference image
        cv::absdiff(ocv_ref, out_gray, diff);
    }

    imwrite("error.png", diff); // Save the difference image for debugging purpose

    float err_per;
    xf::cv::analyzeDiff(diff, 1, err_per);

    return 0;
}
