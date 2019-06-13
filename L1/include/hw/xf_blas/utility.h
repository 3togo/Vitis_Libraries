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

/**
 * @file utility.h
 * @brief common datatypes for L1 modules.
 * 
 * This file is part of XF BLAS Library.
 */

#ifndef XF_BLAS_UTILITY_H
#define XF_BLAS_UTILITY_H

#include <stdint.h>
#include <ostream>
#include <iomanip>
#include <iostream>
#include "hls_math.h"
#include "ap_int.h"
#include "ap_shift_reg.h"

namespace xf {
namespace linear_algebra {
namespace blas {
// Helper macros for renaming kernel
#define PASTER(x,y) x ## y
#define EVALUATOR(x,y)  PASTER(x,y)


#define FLOAT_WIDTH 7
#define CMP_WIDTH 11

constexpr size_t mylog2(size_t n) {
  return ( (n<2) ? 0 : 1+mylog2(n/2));
}

#ifdef __SYNTHESIS__
float abs(float x){
  return fabsf(x);
}
double abs(double x){
  return fabs(x);
}

#endif
template <typename t_DataType>
inline void cmpBigger(t_DataType p_in1, t_DataType p_in2, t_DataType &p_out) {
  p_out = (p_in1 > p_in2)? p_in1: p_in2;
}

template <typename t_DataType, unsigned int t_Width>
inline void wideCmpBigger(t_DataType p_in[t_Width], t_DataType &p_out) {
  #pragma HLS ARRAY_PARTITION variable=p_in dim=1 complete
  const int t_LogWidth = mylog2(t_Width);
  t_DataType l_inner[t_LogWidth][t_Width];
  #pragma HLS ARRAY_PARTITION variable=l_inner dim=1 complete
  #pragma HLS ARRAY_PARTITION variable=l_inner dim=2 complete
  for (int i=0; i<t_Width; ++i) {
  #pragma HLS UNROLL
    l_inner[0][i] = p_in[i];
  }
  for (int i=0; i<t_LogWidth-1; ++i) {
  #pragma HLS UNROLL
    for (int j=0; j<(t_Width / (2<<i)); ++j) {
      cmpBigger(l_inner[i-1][j*2], l_inner[i-1][j*2+1], l_inner[i][j]);
    }
  }
  p_out = l_inner[t_LogWidth-1][0]; 
}

template <typename t_DataType, typename t_IndexType>
inline void cmpBiggerIndex(
  t_DataType p_inData1, t_DataType p_inData2, 
  t_IndexType p_inIndex1, t_IndexType p_inIndex2, 
  t_DataType &p_outData, t_IndexType &p_outIndex) {
  if (p_inData1 >= p_inData2) {
    p_outData = p_inData1;
    p_outIndex = p_inIndex1;
  }
  else {
    p_outData = p_inData2;
    p_outIndex = p_inIndex2;
  }
}

template <typename t_DataType, typename t_IndexType, unsigned int t_Width>
inline void wideCmpBiggerIndex(
  t_DataType p_inData[t_Width],
  t_IndexType p_inIndex[t_Width],
  t_DataType &p_outData,
  t_IndexType &p_outIndex) {
  #pragma HLS ARRAY_PARTITION variable=p_inData dim=1 complete
  #pragma HLS ARRAY_PARTITION variable=p_inIndex dim=1 complete
  const int t_LogWidth = mylog2(t_Width);
  t_DataType l_innerData[t_LogWidth][t_Width];
  #pragma HLS ARRAY_PARTITION variable=l_innerData dim=1 complete
  #pragma HLS ARRAY_PARTITION variable=l_innerData dim=2 complete
  t_IndexType l_innerIndex[t_LogWidth][t_Width];
  #pragma HLS ARRAY_PARTITION variable=l_innerIndex dim=1 complete
  #pragma HLS ARRAY_PARTITION variable=l_innerIndex dim=2 complete
  for (int i=0; i<t_Width; ++i) {
  #pragma HLS UNROLL
    l_innerData[0][i] = p_inData[i];
    l_innerIndex[0][i] = p_inIndex[i];
  }
  for (int i=0; i<t_LogWidth-1; ++i) {
  #pragma HLS UNROLL
    for (int j=0; j<(t_Width / (2<<i)); ++j) {
      cmpBiggerIndex(l_innerData[i-1][j*2], l_innerData[i-1][j*2+1], l_innerIndex[i-1][j*2], l_innerIndex[i-1][j*2+1], l_innerData[i][j], l_innerIndex[i][j]);
    }
  }
  p_outData = l_innerData[t_LogWidth-1][0]; 
  p_outIndex = l_innerIndex[t_LogWidth-1][0];
}

template <typename T, unsigned int t_Width, unsigned int t_DataWidth = sizeof(T) * 8>
class WideType {
  private:
    T  m_Val[t_Width];
    static const unsigned int t_4k = 4096; 
  public:
    typedef T DataType;
    static const unsigned int t_WidthS = t_Width; 
    static const unsigned int t_per4k = t_4k / t_DataWidth / t_Width * 8; 
  public:
    T &getVal(unsigned int i) {return(m_Val[i]);}
    T &operator[](unsigned int p_Idx) {return(m_Val[p_Idx]);}
    T *getValAddr() {return(&m_Val[0]);}
    WideType() {}
    WideType(T p_initScalar) {
        #pragma HLS inline self
        for(int i = 0; i < t_Width; ++i) {
        #pragma HLS UNROLL
          getVal(i) = p_initScalar;
        }
      }
    T
    shift(T p_ValIn) {
        #pragma HLS inline self
        #pragma HLS data_pack variable=p_ValIn
        T l_valOut = m_Val[t_Width-1];
        WIDE_TYPE_SHIFT:for(int i = t_Width - 1; i > 0; --i) {
          T l_val = m_Val[i - 1];
          #pragma HLS data_pack variable=l_val
          m_Val[i] = l_val;
        }
        m_Val[0] = p_ValIn;
        return(l_valOut);
      }
    T
    shift() {
        #pragma HLS inline self
        T l_valOut = m_Val[t_Width-1];
        WIDE_TYPE_SHIFT:for(int i = t_Width - 1; i > 0; --i) {
          T l_val = m_Val[i - 1];
          #pragma HLS data_pack variable=l_val
          m_Val[i] = l_val;
        }
        return(l_valOut);
      }
    T
    unshift() {
        #pragma HLS inline self
        T l_valOut = m_Val[0];
        WIDE_TYPE_SHIFT:for(int i = 0; i < t_Width - 1; ++i) {
          T l_val = m_Val[i + 1];
          #pragma HLS data_pack variable=l_val
          m_Val[i] = l_val;
        }
        return(l_valOut);
      }
    static const WideType zero() {
        WideType l_zero;
        #pragma HLS data_pack variable=l_zero
        for(int i = 0; i < t_Width; ++i) {
          l_zero[i] = 0;
        }
        return(l_zero);
      }
    static
    unsigned int
    per4k() {
        return(t_per4k);
      }
    void
    print(std::ostream& os) {
        for(int i = 0; i < t_Width; ++i) {
          os << std::setw(FLOAT_WIDTH) << getVal(i) << " ";
        }
      }
    
};

template <typename T1, unsigned int T2>
std::ostream& operator<<(std::ostream& os, WideType<T1, T2>& p_Val) {
  p_Val.print(os);
  return(os);
}


template <
    typename t_FloatType,
    unsigned int t_MemWidth,     // In t_FloatType
    unsigned int t_MemWidthBits  // In bits; both must be defined and be consistent
  >
class MemUtil
{
  private:
  public:
    typedef WideType<t_FloatType, t_MemWidth> MemWideType;    
};
/////////////////////////    Control and helper types    /////////////////////////

template<class T, uint8_t t_NumCycles>
T
hlsReg(T p_In)
{
  #pragma HLS INLINE self off
  #pragma HLS INTERFACE ap_none port=return register
  if (t_NumCycles == 1) {
    return p_In;
  } else {
    return hlsReg<T, uint8_t(t_NumCycles - 1)> (p_In);
  }
}

template<class T>
T
hlsReg(T p_In)
{
  return hlsReg<T, 1> (p_In);
}

template <unsigned int W>
class BoolArr {
  private:
    bool m_Val[W];
  public:
    BoolArr(){}
    BoolArr(bool p_Init) {
        #pragma HLS inline self
        for(unsigned int i = 0; i < W; ++i) {
          #pragma HLS UNROLL
          m_Val[i] = p_Init;
        }
    }
    bool & operator[](unsigned int p_Idx) {
        #pragma HLS inline self
        return m_Val[p_Idx];
    }
    bool And() {
        #pragma HLS inline self
        bool l_ret = true;
        for(unsigned int i = 0; i < W; ++i) {
          #pragma HLS UNROLL
          #pragma HLS ARRAY_PARTITION variable=m_Val COMPLETE
          l_ret = l_ret && m_Val[i];
        }
        return(l_ret);
      }
    bool Or() {
        #pragma HLS inline self
        bool l_ret = false;
        for(unsigned int i = 0; i < W; ++i) {
          #pragma HLS UNROLL
          #pragma HLS ARRAY_PARTITION variable=m_Val COMPLETE
          l_ret = l_ret || m_Val[i];
        }
        return(l_ret);
      }
    void Reset() {
        #pragma HLS inline self
        for(unsigned int i = 0; i < W; ++i) {
          #pragma HLS UNROLL
          #pragma HLS ARRAY_PARTITION variable=m_Val COMPLETE
          m_Val[i] = false;
        }
      }
};

template <class S, int W>
bool
streamsAreEmpty(S p_Sin[W]) {
  #pragma HLS inline self
  bool l_allEmpty = true;
  LOOP_S_IDX:for (int w = 0; w < W; ++w) {
    #pragma HLS UNROLL
    l_allEmpty = l_allEmpty && p_Sin[w].empty();
  }
  return(l_allEmpty);
}

//  Bit converter
template <typename T>
class BitConv {
  public:
    static const unsigned int t_SizeOf = sizeof(T);
    static const unsigned int t_NumBits = 8 * sizeof(T);
    typedef ap_uint<t_NumBits> BitsType;
  public:
    BitsType
    toBits(T p_Val) {
      return p_Val;
    }
    T
    toType(BitsType p_Val) {
      return p_Val;
    }
 };
 
template<>
inline
BitConv<float>::BitsType
BitConv<float>::toBits(float p_Val) {
  union {
    float f;
    unsigned int i;
  } u;
  u.f = p_Val;
  return(u.i);
}

template<>
inline
float
BitConv<float>::toType(BitConv<float>::BitsType p_Val) {
  union {
    float f;
    unsigned int i;
  } u;
  u.i = p_Val;
  return(u.f);
}

template<>
inline
BitConv<double>::BitsType
BitConv<double>::toBits(double p_Val) {
  union {
    double f;
    int64_t i;
  } u;
  u.f = p_Val;
  return(u.i);
}

template<>
inline
double
BitConv<double>::toType(BitConv<double>::BitsType p_Val) {
  union {
    double f;
    int64_t i;
  } u;
  u.i = p_Val;
  return(u.f);
}

// Type converter - for vectors of different lengths and types
template <typename TS, typename TD>
class WideConv {
  private:
    static const unsigned int t_ws = TS::t_WidthS;
    static const unsigned int t_wd = TD::t_WidthS;
    typedef BitConv<typename TS::DataType> ConvSType;
    typedef BitConv<typename TD::DataType> ConvDType;
    static const unsigned int t_bs = ConvSType::t_NumBits;
    static const unsigned int t_bd = ConvDType::t_NumBits;
    static const unsigned int l_numBits = t_ws * t_bs;
  private:
    ap_uint<l_numBits> l_bits;
  public:
    inline
    TD
    convert(TS p_Src) {
      TD l_dst;
      ConvSType l_convS;
      assert(t_wd * t_bd == l_numBits);
      for(int ws = 0; ws < t_ws; ++ws) {
        l_bits.range(t_bs * (ws+1) - 1, t_bs * ws) = l_convS.toBits(p_Src[ws]);
      }
      ConvDType l_convD;
      for(int wd = 0; wd < t_wd; ++wd) {
        l_dst[wd] = l_convD.toType(l_bits.range(t_bd * (wd+1) - 1, t_bd * wd));
      }

      return(l_dst);
    }
};
}
}
}
#endif
