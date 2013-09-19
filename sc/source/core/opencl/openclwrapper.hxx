/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef SC_OPENCLWRAPPER_HXX
#define SC_OPENCLWRAPPER_HXX

#include <config_features.h>
#include <formula/opcode.hxx>
#include <sal/detail/log.h>
#include <cassert>
#include "platforminfo.hxx"

#include <rtl/string.hxx>

#include "clcc/clew.h"

// CL_MAP_WRITE_INVALIDATE_REGION is new in OpenCL 1.2.

// When compiling against an older OpenCL, use CL_MAP_WRITE.

// FIXME: But what if this code has been compiled against OpenCL 1.2
// headers but then runs against an OpenCL 1.1 implementation?
// Probably the code should check at run-time the version of the
// OpenCL implementation and choose which flag to use based on that.
#ifdef CL_MAP_WRITE_INVALIDATE_REGION
#define OPENCLWRAPPER_CL_MAP_WRITE_FLAG CL_MAP_WRITE_INVALIDATE_REGION
#else
#define OPENCLWRAPPER_CL_MAP_WRITE_FLAG CL_MAP_WRITE
#endif

#define MaxTextExtent 4096
//support AMD opencl
#define CL_QUEUE_THREAD_HANDLE_AMD 0x403E
#define CL_MAP_WRITE_INVALIDATE_REGION (1 << 2)

#define CHECK_OPENCL(status,name)    \
if( status != CL_SUCCESS )    \
{    \
    printf ("OpenCL error code is %d at " SAL_DETAIL_WHERE " when %s .\n", status, name);    \
    return false;    \
}

#define CHECK_OPENCL_PTR(status,name)    \
if( status != CL_SUCCESS )    \
{    \
    printf ("OpenCL error code is %d at " SAL_DETAIL_WHERE " when %s .\n", status, name);    \
    return NULL;    \
}

#define CHECK_OPENCL_VOID(status,name)    \
if( status != CL_SUCCESS )    \
{    \
    printf ("OpenCL error code is %d at " SAL_DETAIL_WHERE " when %s .\n", status, name);    \
}

#define CHECK_OPENCL_RELEASE(status,name)    \
if ( name != NULL )    \
    clReleaseMemObject( name );    \
if( status != CL_SUCCESS )    \
{    \
    printf ("OpenCL error code is %d at " SAL_DETAIL_WHERE " when clReleaseMemObject( %s ).\n", status, #name);    \
}

#define MAX_KERNEL_STRING_LEN 64
#define MAX_CLFILE_NUM 50
#define MAX_CLKERNEL_NUM 200
#define MAX_KERNEL_NAME_LEN 64

#if defined(_MSC_VER)
#ifndef strcasecmp
#define strcasecmp strcmp
#endif
#endif

#include <cstdio>
#include <vector>

typedef struct _KernelEnv
{
    cl_context mpkContext;
    cl_command_queue mpkCmdQueue;
    cl_program mpkProgram;
} KernelEnv;

extern "C" {

// user defined, this is function wrapper which is used to set the input
// parameters, launch kernel and copy data from GPU to CPU or CPU to GPU.
typedef int ( *cl_kernel_function )( void **userdata, KernelEnv *kenv );

}

namespace sc { namespace opencl {

typedef unsigned int uint;

struct OpenCLEnv
{
    cl_platform_id mpOclPlatformID;
    cl_context mpOclContext;
    cl_device_id mpOclDevsID;
    cl_command_queue mpOclCmdQueue;
};

struct Kernel
{
    const char* mpName;
    cl_kernel mpKernel;

    Kernel( const char* pName );
};

struct GPUEnv
{
    //share vb in all modules in hb library
    cl_platform_id mpPlatformID;
    cl_device_type mDevType;
    cl_context mpContext;
    cl_device_id *mpArryDevsID;
    cl_device_id mpDevID;
    cl_command_queue mpCmdQueue;
    cl_kernel mpArryKernels[MAX_CLFILE_NUM];
    cl_program mpArryPrograms[MAX_CLFILE_NUM]; //one program object maps one kernel source file
    char mArryKnelSrcFile[MAX_CLFILE_NUM][256]; //the max len of kernel file name is 256
    std::vector<Kernel> maKernels;
    int mnFileCount; // only one kernel file
    int mnIsUserCreated; // 1: created , 0:no create and needed to create by opencl wrapper
    int mnKhrFp64Flag;
    int mnAmdFp64Flag;
};

struct SingleVectorFormula
{
    const double *mdpInputLeftData;
    const double *mdpInputRightData;
    size_t mnInputLeftDataSize;
    size_t mnInputRightDataSize;
    uint mnInputLeftStartPosition;
    uint mnInputRightStartPosition;
    int mnInputLeftOffset;
    int mnInputRightOffset;
};

struct DoubleVectorFormula
{
    const double *mdpInputData;
    size_t mnInputDataSize;
    uint mnInputStartPosition;
    uint mnInputEndPosition;
    int mnInputStartOffset;
    int mnInputEndOffset;
};

class OpenclDevice
{
public:
    static GPUEnv gpuEnv;
    static int isInited;
    static OString maSourceHash;
    static int registOpenclKernel();
    static int releaseOpenclRunEnv();
    static int initOpenclRunEnv( GPUEnv *gpu );
    static int releaseOpenclEnv( GPUEnv *gpuInfo );
    static int compileKernelFile( GPUEnv *gpuInfo, const char *buildOption );
    static int initOpenclRunEnv( int argc );
    static int cachedOfKernerPrg( const GPUEnv *gpuEnvCached, const char * clFileName );
    static int generatBinFromKernelSource( cl_program program, const char * clFileName );
    static int writeBinaryToFile( const OString& rName, const char* birary, size_t numBytes );
    static int binaryGenerated( const char * clFileName, FILE ** fhandle );
    static int compileKernelFile( const char *filename, GPUEnv *gpuInfo, const char *buildOption );

    static int initOpenclAttr( OpenCLEnv * env );
    static int setKernelEnv( KernelEnv *envInfo );
    static Kernel* checkKernelName( const char *kernelName );

    static int getOpenclState();
    static void setOpenclState( int state );
};

class OclCalc: public OpenclDevice
{

public:
    KernelEnv kEnv;
    cl_mem mpClmemSrcData;
    cl_mem mpClmemStartPos;
    cl_mem mpClmemEndPos;
    cl_mem mpClmemLeftData;
    cl_mem mpClmemRightData;
    cl_mem mpClmemMergeLfData;
    cl_mem mpClmemMergeRtData;
    cl_mem mpClmemMatixSumSize;
    cl_mem mpClmemeOp;
    unsigned int nArithmeticLen;
    unsigned int nFormulaLen;
    unsigned int nClmemLen;
    unsigned int nFormulaColSize;
    unsigned int nFormulaRowSize;

    OclCalc();
    ~OclCalc();

// for 64bits double
    bool oclHostArithmeticOperator64Bits( const char* aKernelName,  double *&rResult, int nRowSize );
    bool oclMoreColHostArithmeticOperator64Bits( int nDataSize,int neOpSize,double *rResult, int nRowSize );
    bool oclHostFormulaStatistics64Bits( const char* aKernelName, double *&output, int outputSize);
    bool oclHostFormulaStash64Bits( const char* aKernelName, const double* dpSrcData, uint *nStartPos, uint *nEndPos, double *output, int nBufferSize, int size);
    bool oclHostFormulaCount64Bits( uint *npStartPos, uint *npEndPos, double *&dpOutput, int nSize );
    bool oclHostFormulaSumProduct64Bits( double *fpSumProMergeLfData, double *fpSumProMergeRrData, uint *npSumSize, double *&dpOutput, int nSize);
    bool oclHostMatrixInverse64Bits( const char* aKernelName, double *dpOclMatrixSrc, double *dpOclMatrixDst, std::vector<double>&dpResult, uint nDim );
// for 32bits float
    bool oclHostArithmeticOperator32Bits( const char* aKernelName, double *rResult, int nRowSize );
    bool oclMoreColHostArithmeticOperator32Bits( int nDataSize,int neOpSize,double *rResult, int nRowSize );
    bool oclHostFormulaStatistics32Bits( const char* aKernelName, double *output, int outputSize);
    bool oclHostFormulaCount32Bits( uint *npStartPos, uint *npEndPos, double *dpOutput, int nSize );
    bool oclHostArithmeticStash64Bits( const char* aKernelName, const double *dpLeftData, const double *dpRightData, double *rResult,int nRowSize );
    bool oclHostFormulaSumProduct32Bits( float *fpSumProMergeLfData, float *fpSumProMergeRrData, uint *npSumSize, double *dpOutput, int nSize );
    bool oclHostMatrixInverse32Bits( const char* aKernelName, float *fpOclMatrixSrc, float *fpOclMatrixDst, std::vector<double>& dpResult, uint nDim );
// for groundwater
    bool oclGroundWaterGroup( uint *eOp, uint eOpNum, const double *pOpArray, const double *pSubtractSingle,size_t nSrcDataSize, size_t nElements, double delta ,uint *nStartPos,uint *nEndPos,double *deResult);
    double *oclSimpleDeltaOperation( OpCode eOp, const double *pOpArray, const double *pSubtractSingle, size_t nElements, double delta );

    ///////////////////////////////////////////////////////////////
    bool createBuffer64Bits( double *&dpLeftData, double *&dpRightData, int nBufferSize );
    bool mapAndCopy64Bits(const double *dpTempLeftData,const double *dpTempRightData,int nBufferSize );
    bool mapAndCopy64Bits(const double *dpTempSrcData,unsigned int *unStartPos,unsigned int *unEndPos,int nBufferSize ,int nRowsize);
    bool mapAndCopyArithmetic64Bits( const double *dpMoreArithmetic,int nBufferSize );
    bool mapAndCopyMoreColArithmetic64Bits( const double *dpMoreColArithmetic,int nBufferSize ,uint *npeOp,uint neOpSize );
    bool createMoreColArithmeticBuf64Bits( int nBufferSize, int neOpSize );

    bool createFormulaBuf64Bits( int nBufferSize, int rowSize );
    bool createArithmeticOptBuf64Bits( int nBufferSize );

    bool createBuffer32Bits( float *&fpLeftData, float *&fpRightData, int nBufferSize );
    bool mapAndCopy32Bits(const double *dpTempLeftData,const double *dpTempRightData,int nBufferSize );
    bool mapAndCopy32Bits(const double *dpTempSrcData,unsigned int *unStartPos,unsigned int *unEndPos,int nBufferSize ,int nRowsize);
    bool mapAndCopyArithmetic32Bits( const double *dpMoreColArithmetic, int nBufferSize );
    bool mapAndCopyMoreColArithmetic32Bits( const double *dpMoreColArithmetic,int nBufferSize ,uint *npeOp,uint neOpSize );
    bool createMoreColArithmeticBuf32Bits( int nBufferSize, int neOpSize );
    bool createFormulaBuf32Bits( int nBufferSize, int rowSize );
    bool createArithmeticOptBuf32Bits( int nBufferSize );
    bool oclHostFormulaStash32Bits( const char* aKernelName, const double* dpSrcData, uint *nStartPos, uint *nEndPos, double *output, int nBufferSize, int size );
    bool oclHostArithmeticStash32Bits( const char* aKernelName, const double *dpLeftData, const double *dpRightData, double *rResult,int nRowSize );

    void releaseOclBuffer();

    friend class agency;
};

size_t getOpenCLPlatformCount();
const std::vector<OpenclPlatformInfo>& fillOpenCLInfo();

/**
 * Used to set or switch between OpenCL devices.
 *
 * @param pDeviceId the id of the opencl device of type cl_device_id, NULL means use software calculation
 * @param bAutoSelect use the algorithm to select the best OpenCL device
 *
 * @return returns true if there is a valid opencl device that has been set up
 */
bool switchOpenclDevice(const OUString* pDeviceId, bool bAutoSelect);

void compileKernels(const OUString* pDeviceId);

}}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
