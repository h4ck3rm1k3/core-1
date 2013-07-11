/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <config_features.h>
#include "formulagroup.hxx"
#include "document.hxx"
#include "formulacell.hxx"
#include "tokenarray.hxx"
#include "compiler.hxx"
#include "interpre.hxx"
#include "formula/vectortoken.hxx"
#include "scmatrix.hxx"

#include "openclwrapper.hxx"

namespace sc {

// A single public entry point for a factory function:
namespace opencl {
    extern sc::FormulaGroupInterpreter *createFormulaGroupInterpreter();
}

/////time test dbg
double getTimeDiff(const TimeValue& t1, const TimeValue& t2)
{
    double tv1 = t1.Seconds;
    double tv2 = t2.Seconds;
    tv1 += t1.Nanosec / 1000000000.0;
    tv2 += t2.Nanosec / 1000000000.0;

    return tv1 - tv2;
}//dbg-t
TimeValue aTimeBefore, aTimeAfter;
///////////////////////////////////////

class FormulaGroupInterpreterOpenCL : public FormulaGroupInterpreterSoftware
{
public:
    FormulaGroupInterpreterOpenCL() :
        FormulaGroupInterpreterSoftware()
    {
        OclCalc::InitEnv();
    }
    virtual ~FormulaGroupInterpreterOpenCL()
    {
        OclCalc::ReleaseOpenclRunEnv();
    }

    virtual ScMatrixRef inverseMatrix(const ScMatrix& rMat);
    virtual bool interpret(ScDocument& rDoc, const ScAddress& rTopPos,
                           const ScFormulaCellGroupRef& xGroup, ScTokenArray& rCode);
};

ScMatrixRef FormulaGroupInterpreterOpenCL::inverseMatrix(const ScMatrix& /* rMat */)
{
    return ScMatrixRef();
}

bool FormulaGroupInterpreterOpenCL::interpret(ScDocument& rDoc, const ScAddress& rTopPos,
                                              const ScFormulaCellGroupRef& xGroup, ScTokenArray& rCode)
{
    generateRPNCode(rDoc, rTopPos, rCode);

    size_t rowSize = xGroup->mnLength;
    fprintf(stderr,"rowSize at begin is ...%ld.\n",(long)rowSize);
    // The row quantity can be gotten from p2->GetArrayLength()
    int nCount1 = 0, nCount2 = 0, nCount3 = 0;
    int nOclOp = 0;
    double *rResult = NULL; // Point to the output data from GPU
    rResult = (double *)malloc(sizeof(double) * rowSize*2);// For 2 columns(B,C)
    if(NULL==rResult)
    {
        printf("malloc err\n");
        return false;
    }
    memset(rResult,0,rowSize);
    float * fpOclSrcData = NULL; // Point to the input data from CPU
    uint * npOclStartPos = NULL; // The first position for calculation,for example,the A1 in (=MAX(A1:A100))
    uint * npOclEndPos     = NULL; // The last position for calculation,for example, the A100 in (=MAX(A1:A100))
    float * fpLeftData     = NULL; // Left input for binary operator(+,-,*,/),for example,(=leftData+rightData)
    float * fpRightData  = NULL; // Right input for binary operator(+,-,*,/),for example,(=leftData/rightData)
                                 // The rightData can't be zero for "/"
    static OclCalc ocl_calc;
    // Don't know how large the size will be applied previously, so create them as the rowSize or 65536
    // Don't know which formulae will be used previously, so create buffers for different formulae used probably
    ocl_calc.CreateBuffer(fpOclSrcData,npOclStartPos,npOclEndPos,rowSize);
    ocl_calc.CreateBuffer(fpLeftData,fpRightData,rowSize);
    //printf("pptrr is %d,%d,%d\n",fpOclSrcData,npOclStartPos,npOclEndPos);
///////////////////////////////////////////////////////////////////////////////////////////

    // Until we implement group calculation for real, decompose the group into
    // individual formula token arrays for individual calculation.
    ScAddress aTmpPos = rTopPos;
    for (sal_Int32 i = 0; i < xGroup->mnLength; ++i)
    {
        aTmpPos.SetRow(xGroup->mnStart + i);
        ScTokenArray aCode2;
        for (const formula::FormulaToken* p = rCode.First(); p; p = rCode.Next())
        {
            switch (p->GetType())
            {
                case formula::svSingleVectorRef:
                {
                    const formula::SingleVectorRefToken* p2 = static_cast<const formula::SingleVectorRefToken*>(p);
                    const double* pArray = p2->GetArray();
                    aCode2.AddDouble(static_cast<size_t>(i) < p2->GetArrayLength() ? pArray[i] : 0.0);
                }
                break;
                case formula::svDoubleVectorRef:
                {
                    const formula::DoubleVectorRefToken* p2 = static_cast<const formula::DoubleVectorRefToken*>(p);
                    const std::vector<const double*>& rArrays = p2->GetArrays();
                    size_t nColSize = rArrays.size();
                    size_t nRowStart = p2->IsStartFixed() ? 0 : i;
                    size_t nRowEnd = p2->GetRefRowSize() - 1;
                    if (!p2->IsEndFixed())
                        nRowEnd += i;
                    size_t nRowSize = nRowEnd - nRowStart + 1;
                    ScMatrixRef pMat(new ScMatrix(nColSize, nRowSize, 0.0));

                    npOclStartPos[i] = nRowStart; // record the start position
                    npOclEndPos[i]     = nRowEnd;   // record the end position

                    for (size_t nCol = 0; nCol < nColSize; ++nCol)
                    {
                        const double* pArray = rArrays[nCol];
                        if( NULL==pArray )
                        {
                            fprintf(stderr,"Error: pArray is NULL!\n");
                            return false;
                        }

                        for( size_t u=0; u<rowSize; u++ )
                        {
                            // Many video cards can't support double type in kernel, so need transfer the double to float
                            fpOclSrcData[u] = (float)pArray[u];
                            //fprintf(stderr,"fpOclSrcData[%d] is %f.\n",u,fpOclSrcData[u]);
                        }

                        for (size_t nRow = 0; nRow < nRowSize; ++nRow)
                        {
                            if (nRowStart + nRow < p2->GetArrayLength())
                            {
                                double fVal = pArray[nRowStart+nRow];
                                pMat->PutDouble(fVal, nCol, nRow);
                            }
                        }
                    }

                    ScMatrixToken aTok(pMat);
                    aCode2.AddToken(aTok);
                }
                break;
                default:
                    aCode2.AddToken(*p);
            }
        }

        ScFormulaCell* pDest = rDoc.GetFormulaCell(aTmpPos);
        if (!pDest)
            return false;

        const formula::FormulaToken *pCur = aCode2.First();
        aCode2.Reset();
        while( ( pCur = aCode2.Next() ) != NULL )
        {
            OpCode eOp = pCur->GetOpCode();
            if(eOp==0)
            {
                 if(nCount3%2==0)
                     fpLeftData[nCount1++] = (float)pCur->GetDouble();
                 else
                     fpRightData[nCount2++] = (float)pCur->GetDouble();
                 nCount3++;
            }
            else if( eOp!=ocOpen && eOp!=ocClose )
                nOclOp = eOp;

//              if(count1>0){//dbg
//                  fprintf(stderr,"leftData is %f.\n",leftData[count1-1]);
//                  count1--;
//              }
//              if(count2>0){//dbg
//                  fprintf(stderr,"rightData is %f.\n",rightData[count2-1]);
//                  count2--;
//              }
        }

        if(!getenv("SC_GPU")||!ocl_calc.GetOpenclState())
        {
            fprintf(stderr,"ccCPU flow...\n\n");
            generateRPNCode(rDoc, aTmpPos, aCode2);
            ScInterpreter aInterpreter(pDest, &rDoc, aTmpPos, aCode2);
            aInterpreter.Interpret();
            pDest->SetResultToken(aInterpreter.GetResultToken().get());
            pDest->ResetDirty();
            pDest->SetChanged(true);
        }
    } // for loop end (xGroup->mnLength)

    // For GPU calculation
    if(getenv("SC_GPU")&&ocl_calc.GetOpenclState())
    {
            fprintf(stderr,"ggGPU flow...\n\n");
            printf(" oclOp is... %d\n",nOclOp);
            osl_getSystemTime(&aTimeBefore); //timer
            switch(nOclOp)
            {
                case ocAdd:
                    ocl_calc.OclHostSignedAdd32Bits(fpLeftData,fpRightData,rResult,nCount1);
                    break;
                case ocSub:
                    ocl_calc.OclHostSignedSub32Bits(fpLeftData,fpRightData,rResult,nCount1);
                    break;
                case ocMul:
                    ocl_calc.OclHostSignedMul32Bits(fpLeftData,fpRightData,rResult,nCount1);
                    break;
                case ocDiv:
                    ocl_calc.OclHostSignedDiv32Bits(fpLeftData,fpRightData,rResult,nCount1);
                    break;
                case ocMax:
                    ocl_calc.OclHostFormulaMax32Bits(fpOclSrcData,npOclStartPos,npOclEndPos,rResult,rowSize);
                    break;
                case ocMin:
                    ocl_calc.OclHostFormulaMin32Bits(fpOclSrcData,npOclStartPos,npOclEndPos,rResult,rowSize);
                    break;
                case ocAverage:
                    ocl_calc.OclHostFormulaAverage32Bits(fpOclSrcData,npOclStartPos,npOclEndPos,rResult,rowSize);
                    break;
                case ocSum:
                    //ocl_calc.OclHostFormulaSum(srcData,rangeStart,rangeEnd,rResult,rowSize);
                    break;
                case ocCount:
                    //ocl_calc.OclHostFormulaCount(rangeStart,rangeEnd,rResult,rowSize);
                    break;
                case ocSumProduct:
                    //ocl_calc.OclHostFormulaSumProduct(srcData,rangeStart,rangeEnd,rResult,rowSize);
                    break;
                default:
                    fprintf(stderr,"No OpenCL function for this calculation.\n");
                    break;
            }
            /////////////////////////////////////////////////////
            osl_getSystemTime(&aTimeAfter);
            double diff = getTimeDiff(aTimeAfter, aTimeBefore);
            //if (diff >= 1.0)
            {
                fprintf(stderr,"OpenCL,diff...%f.\n",diff);
            }
/////////////////////////////////////////////////////

//rResult[i];
//           for(sal_Int32 i = 0; i < rowSize; ++i){//dbg output results
//               fprintf(stderr,"After GPU,rRsults[%d] is ...%f\n",i,rResult[i]);
//           }

            // Insert the double data, in rResult[i] back into the document
            rDoc.SetFormulaResults(rTopPos, rResult, xGroup->mnLength);
        }

        if(rResult)
            free(rResult);

        if(getenv("SC_GPUSAMPLE")){
            //fprintf(stderr,"FormulaGroupInterpreter::interpret(),iniflag...%d\n",ocl_calc.GetOpenclState());
            //ocl_calc.OclTest();//opencl test sample for debug
        }

    return true;
}

/// Special case of formula compiler for groundwatering
class FormulaGroupInterpreterGroundwater : public FormulaGroupInterpreterSoftware
{
public:
    FormulaGroupInterpreterGroundwater() :
        FormulaGroupInterpreterSoftware()
    {
        fprintf(stderr,"\n\n ***** Groundwater Backend *****\n\n\n");
        OclCalc::InitEnv();
    }
    virtual ~FormulaGroupInterpreterGroundwater()
    {
        OclCalc::ReleaseOpenclRunEnv();
    }

    virtual ScMatrixRef inverseMatrix(const ScMatrix& /* rMat */) { return ScMatrixRef(); }
    virtual bool interpret(ScDocument& rDoc, const ScAddress& rTopPos,
                           const ScFormulaCellGroupRef& xGroup, ScTokenArray& rCode);
};

#define RETURN_IF_FAIL(a,b) do { if (!(a)) { fprintf (stderr,b"\n"); return false; } } while (0)

#include "compiler.hxx"

// FIXME: really we should compile the formula and operate on the
// RPN representation which -should- be more compact and have no Open / Close
// or precedence issues; cf. rCode.FirstRPN() etc.
bool FormulaGroupInterpreterGroundwater::interpret(ScDocument& rDoc, const ScAddress& rTopPos,
                                                   const ScFormulaCellGroupRef& xGroup,
                                                   ScTokenArray& rCode)
{
    generateRPNCode(rDoc, rTopPos, rCode);

    // Inputs: both of length xGroup->mnLength
    OpCode eOp; // type of operation: ocAverage, ocMax, ocMin
    const double *pArrayToSubtractOneElementFrom;
    const double *pGroundWaterDataArray;

    const formula::FormulaToken* p = rCode.FirstRPN();
    RETURN_IF_FAIL(p != NULL && p->GetOpCode() == ocPush && p->GetType() == formula::svDoubleVectorRef, "double vector ref expected");

    // Get the range reference vector.
    const formula::DoubleVectorRefToken* pDvr = static_cast<const formula::DoubleVectorRefToken*>(p);
    const std::vector<const double*>& rArrays = pDvr->GetArrays();
    RETURN_IF_FAIL(rArrays.size() == 1, "unexpectedly large double ref array");
    RETURN_IF_FAIL(pDvr->GetArrayLength() == (size_t)xGroup->mnLength, "wrong double ref length");
    RETURN_IF_FAIL(pDvr->IsStartFixed() && pDvr->IsEndFixed(), "non-fixed ranges )");
    pGroundWaterDataArray = rArrays[0];

    // Function:
    p = rCode.NextRPN();
    RETURN_IF_FAIL(p != NULL, "no operator");
    eOp = p->GetOpCode();
    RETURN_IF_FAIL(eOp == ocAverage || eOp == ocMax || eOp == ocMin, "unexpected opcode - expected either average, max, or min");

    p = rCode.NextRPN();
    RETURN_IF_FAIL(p != NULL && p->GetOpCode() == ocPush && p->GetType() == formula::svSingleVectorRef, "single vector ref expected");

    // Get the single reference vector.
    const formula::SingleVectorRefToken* pSvr = static_cast<const formula::SingleVectorRefToken*>(p);
    pArrayToSubtractOneElementFrom = pSvr->GetArray();
    RETURN_IF_FAIL(pSvr->GetArrayLength() == (size_t)xGroup->mnLength, "wrong single ref length");

    p = rCode.NextRPN();
    RETURN_IF_FAIL(p != NULL && p->GetOpCode() == ocSub, "missing subtract opcode");

    p = rCode.NextRPN();
    RETURN_IF_FAIL(p == NULL, "there should be no more token");

    static OclCalc ocl_calc;

    // Here we have all the data we need to dispatch our openCL kernel [ I hope ]
    // so for:
    //   =AVERAGE(L$6:L$7701) - L6
    // we would get:
    //   eOp => ocAverage
    //   pGroundWaterDataArray => contains L$5:L$7701
    //   pArrayToSubtractOneElementFrom => contains L$5:L$7701 (ie. a copy)
    //   length of this array -> xGroup->mnLength

    fprintf (stderr, "Calculate !");

    double *pResult = ocl_calc.OclSimpleDeltaOperation( eOp, pGroundWaterDataArray,
                                                        pArrayToSubtractOneElementFrom,
                                                        (size_t) xGroup->mnLength );
    RETURN_IF_FAIL(pResult != NULL, "buffer alloc / calculaton failed");

    // Insert the double data, in rResult[i] back into the document
    rDoc.SetFormulaResults(rTopPos, pResult, xGroup->mnLength);

    delete [] pResult;

    SAL_DEBUG ("exit cleanly !");
    return true;
}

namespace opencl {
    sc::FormulaGroupInterpreter *createFormulaGroupInterpreter()
    {
        if (getenv("SC_GROUNDWATER"))
            return new sc::FormulaGroupInterpreterGroundwater();
        else
            return new sc::FormulaGroupInterpreterOpenCL();
    }
} // namespace opencl

} // namespace sc

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
