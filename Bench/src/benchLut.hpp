////////////////////////////////////////////////////////////////////////////////
//! @file	: benchLut.hpp
//! @date   : Jul 2013
//!
//! @brief  : Benchmark class for LUT
//! 
//! Copyright (C) 2013 - CRVI
//!
//! This file is part of OpenCLIPP.
//! 
//! OpenCLIPP is free software: you can redistribute it and/or modify
//! it under the terms of the GNU Lesser General Public License version 3
//! as published by the Free Software Foundation.
//! 
//! OpenCLIPP is distributed in the hope that it will be useful,
//! but WITHOUT ANY WARRANTY; without even the implied warranty of
//! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//! GNU Lesser General Public License for more details.
//! 
//! You should have received a copy of the GNU Lesser General Public License
//! along with OpenCLIPP.  If not, see <http://www.gnu.org/licenses/>.
//! 
////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <vector>

using namespace std;

template<typename DataType> class LutBench;

typedef LutBench<unsigned char>  LutBenchU8;


template<typename DataType>
class LutBench : public IBench1in1out
{
public:
   LutBench()
   :  IBench1in1out(USE_BUFFER)
   { }

   void Create(uint Width, uint Height);
   void Free();
   void RunIPP();
   void RunCUDA();
   void RunCL();
   void RunNPP();

   typedef DataType dataType;

   const static int Length = 5;

private:
   vector<uint> m_Levels;
   vector<uint> m_Values;

   vector<DataType> m_LevelsCUDA;
   vector<DataType> m_ValuesCUDA;

   DataType * m_LevelsPtrCUDA;
   DataType * m_ValuesPtrCUDA;

   NPP_CODE(
      Npp32s * m_NPPLevels;
      Npp32s * m_NPPValues;
      )
};
//-----------------------------------------------------------------------------------------------------------------------------
template<typename DataType>
void LutBench<DataType>::Create(uint Width, uint Height)
{
   IBench1in1out::Create<DataType, DataType>(Width, Height);

   m_Levels.assign(Length, 0);
   m_Values.assign(Length, 0);
   m_LevelsCUDA.assign(Length, 0);
   m_ValuesCUDA.assign(Length, 0);

   for (uint& e : m_Levels)
      e = rand() % 256;

   for (uint& e : m_Values)
      e = rand() % 256;

   sort(m_Levels.begin(), m_Levels.end());
   sort(m_Values.begin(), m_Values.end());

   for (int i = 0; i < Length; i++)
   {
      m_LevelsCUDA[i] = m_Levels[i];
      m_ValuesCUDA[i] = m_Values[i];
   }


   CUDA_CODE(
      CUDAPP(Malloc)(m_LevelsPtrCUDA, Length);
      CUDAPP(Malloc)(m_ValuesPtrCUDA, Length);

      CUDAPP(Upload)(m_LevelsCUDA.data(), m_LevelsPtrCUDA, Length);
      CUDAPP(Upload)(m_ValuesCUDA.data(), m_ValuesPtrCUDA, Length);
      )

   NPP_CODE(
      cudaMalloc((void**) &m_NPPLevels, Length * sizeof(Npp32s));
      cudaMalloc((void**) &m_NPPValues, Length * sizeof(Npp32s));

      cudaMemcpy(m_NPPLevels, m_Levels.data(), Length * sizeof(Npp32s), cudaMemcpyHostToDevice);
      cudaMemcpy(m_NPPValues, m_Values.data(), Length * sizeof(Npp32s), cudaMemcpyHostToDevice);
      )
}
//-----------------------------------------------------------------------------------------------------------------------------
template<typename DataType>
void LutBench<DataType>::Free()
{
   IBench1in1out::Free();

   CUDA_CODE(
      CUDAPP(Free)(m_LevelsPtrCUDA);
      CUDAPP(Free)(m_ValuesPtrCUDA);
      )

   NPP_CODE(
      cudaFree(m_NPPLevels);
      cudaFree(m_NPPValues);
      )
}
//-----------------------------------------------------------------------------------------------------------------------------
template<typename DataType>
void LutBench<DataType>::RunIPP()
{
   IPP_CODE(
      ippiLUT_8u_C1R(m_ImgSrc.Data(), m_ImgSrc.Step, m_ImgDstIPP.Data(), m_ImgDstIPP.Step,
         m_IPPRoi, (int*) m_Values.data(), (int*) m_Levels.data(), Length);
      )
}
//-----------------------------------------------------------------------------------------------------------------------------
template<typename DataType>
void LutBench<DataType>::RunCL()
{
   if (CLUsesBuffer())
      ocipLut_V(m_CLBufferSrc, m_CLBufferDst, m_Levels.data(), m_Values.data(), Length);
   else
      ocipLut(m_CLSrc, m_CLDst, m_Levels.data(), m_Values.data(), Length);
}
//-----------------------------------------------------------------------------------------------------------------------------
template<typename DataType>
void LutBench<DataType>::RunCUDA()
{
   // This library does only linear LUT, but it still is quite fast
   CUDA_CODE(
      CUDAPP(LUT_Linear<DataType>)((DataType*) m_CUDASrc, m_CUDASrcStep, (DataType*) m_CUDADst, m_CUDADstStep, m_ImgSrc.Width, m_ImgSrc.Height,
         m_LevelsPtrCUDA, m_ValuesPtrCUDA, Length);
      )
}
//-----------------------------------------------------------------------------------------------------------------------------
template<>
void LutBench<unsigned char>::RunNPP()
{
   NPP_CODE(
      nppiLUT_8u_C1R((Npp8u*) m_NPPSrc, m_NPPSrcStep, (Npp8u*) m_NPPDst, m_NPPDstStep, m_NPPRoi, m_NPPValues, m_NPPLevels, Length);
      )
}
