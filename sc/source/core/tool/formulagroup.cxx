/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "formulagroup.hxx"
#include "document.hxx"
#include "formulacell.hxx"
#include "tokenarray.hxx"
#include "compiler.hxx"
#include "interpre.hxx"
#include "scmatrix.hxx"

#include "formula/vectortoken.hxx"
#include "config_features.h"

#include <vector>
#include <boost/unordered_map.hpp>

#define USE_DUMMY_INTERPRETER 1

#if USE_DUMMY_INTERPRETER
#include <cstdio>
#endif

namespace sc {

rtl_uString* FormulaGroupContext::intern( const OUString& rStr )
{
    StrHashType::iterator it = maStrPool.find(rStr);
    if (it == maStrPool.end())
    {
        // Not yet in the pool.
        std::pair<StrHashType::iterator, bool> r = maStrPool.insert(rStr.intern());
        if (!r.second)
            // Insertion failed.
            return NULL;

        it = r.first;
    }

    return it->pData;
}

namespace {

/**
 * Input double array consists of segments of NaN's and normal values.
 * Insert only the normal values into the matrix while skipping the NaN's.
 */
void fillMatrix( ScMatrix& rMat, size_t nCol, const double* pNums, size_t nLen )
{
    const double* p = pNums;
    const double* pEnd = p + nLen;
    const double* pHead = NULL;
    for (; p != pEnd; ++p)
    {
        if (!rtl::math::isNan(*p))
        {
            if (!pHead)
                // Store the first non-NaN position.
                pHead = p;

            continue;
        }

        if (pHead)
        {
            // Flush this non-NaN segment to the matrix.
            rMat.PutDouble(pHead, p - pHead, nCol, pHead - pNums);
            pHead = NULL;
        }
    }

    if (pHead)
    {
        // Flush last non-NaN segment to the matrix.
        rMat.PutDouble(pHead, p - pHead, nCol, pHead - pNums);
    }
}

void flushSegment(
    ScMatrix& rMat, size_t nCol, rtl_uString** pHead, rtl_uString** pCur, rtl_uString** pTop )
{
    size_t nOffset = pHead - pTop;
    std::vector<OUString> aStrs;
    aStrs.reserve(pCur - pHead);
    for (; pHead != pCur; ++pHead)
        aStrs.push_back(OUString(*pHead));

    rMat.PutString(&aStrs[0], aStrs.size(), nCol, nOffset);
}

void fillMatrix( ScMatrix& rMat, size_t nCol, rtl_uString** pStrs, size_t nLen )
{
    rtl_uString** p = pStrs;
    rtl_uString** pEnd = p + nLen;
    rtl_uString** pHead = NULL;
    for (; p != pEnd; ++p)
    {
        if (*p)
        {
            if (!pHead)
                // Store the first non-empty string position.
                pHead = p;

            continue;
        }

        if (pHead)
        {
            // Flush this non-empty segment to the matrix.
            flushSegment(rMat, nCol, pHead, p, pStrs);
            pHead = NULL;
        }
    }

    if (pHead)
    {
        // Flush last non-empty segment to the matrix.
        flushSegment(rMat, nCol, pHead, p, pStrs);
    }
}

}

ScMatrixRef FormulaGroupInterpreterSoftware::inverseMatrix(const ScMatrix& /*rMat*/)
{
    return ScMatrixRef();
}

bool FormulaGroupInterpreterSoftware::interpret(ScDocument& rDoc, const ScAddress& rTopPos,
                                                const ScFormulaCellGroupRef& xGroup,
                                                ScTokenArray& rCode)
{
    typedef boost::unordered_map<const formula::FormulaToken*, formula::FormulaTokenRef> CachedTokensType;

    // Decompose the group into individual cells and calculate them individually.

    // The caller must ensure that the top position is the start position of
    // the group.

    ScAddress aTmpPos = rTopPos;
    std::vector<double> aResults;
    aResults.reserve(xGroup->mnLength);
    CachedTokensType aCachedTokens;

    double fNan;
    rtl::math::setNan(&fNan);

    for (SCROW i = 0; i < xGroup->mnLength; ++i, aTmpPos.IncRow())
    {
        ScTokenArray aCode2;
        for (const formula::FormulaToken* p = rCode.First(); p; p = rCode.Next())
        {
            CachedTokensType::iterator it = aCachedTokens.find(p);
            if (it != aCachedTokens.end())
            {
                // This token is cached. Use the cached one.
                aCode2.AddToken(*it->second);
                continue;
            }

            switch (p->GetType())
            {
                case formula::svSingleVectorRef:
                {
                    const formula::SingleVectorRefToken* p2 = static_cast<const formula::SingleVectorRefToken*>(p);
                    const formula::VectorRefArray& rArray = p2->GetArray();
                    if (rArray.mbNumeric)
                    {
                        double fVal = fNan;
                        if (static_cast<size_t>(i) < p2->GetArrayLength())
                            fVal = rArray.mpNumericArray[i];

                        if (rtl::math::isNan(fVal))
                            aCode2.AddToken(ScEmptyCellToken(false, false));
                        else
                            aCode2.AddDouble(fVal);
                    }
                    else
                    {
                        rtl_uString* pStr = NULL;
                        if (static_cast<size_t>(i) < p2->GetArrayLength())
                            pStr = rArray.mpStringArray[i];

                        if (pStr)
                            aCode2.AddString(OUString(pStr));
                    }
                }
                break;
                case formula::svDoubleVectorRef:
                {
                    const formula::DoubleVectorRefToken* p2 = static_cast<const formula::DoubleVectorRefToken*>(p);
                    const std::vector<formula::VectorRefArray>& rArrays = p2->GetArrays();
                    size_t nColSize = rArrays.size();
                    size_t nRowStart = p2->IsStartFixed() ? 0 : i;
                    size_t nRowEnd = p2->GetRefRowSize() - 1;
                    if (!p2->IsEndFixed())
                        nRowEnd += i;
                    size_t nRowSize = nRowEnd - nRowStart + 1;
                    ScMatrixRef pMat(new ScMatrix(nColSize, nRowSize));

                    size_t nDataRowEnd = p2->GetArrayLength() - 1;
                    if (nRowStart > nDataRowEnd)
                        // Referenced rows are all empty.
                        nRowSize = 0;
                    else if (nRowEnd > nDataRowEnd)
                        // Data array is shorter than the row size of the reference. Truncate it to the data.
                        nRowSize -= nRowEnd - nDataRowEnd;

                    for (size_t nCol = 0; nCol < nColSize; ++nCol)
                    {
                        const formula::VectorRefArray& rArray = rArrays[nCol];
                        if (rArray.mbNumeric)
                        {
                            const double* pNums = rArray.mpNumericArray;
                            pNums += nRowStart;
                            fillMatrix(*pMat, nCol, pNums, nRowSize);
                        }
                        else
                        {
                            rtl_uString** pStrs = rArray.mpStringArray;
                            pStrs += nRowStart;
                            fillMatrix(*pMat, nCol, pStrs, nRowSize);
                        }
                    }

                    if (p2->IsStartFixed() && p2->IsEndFixed())
                    {
                        // Cached the converted token for absolute range referene.
                        ScComplexRefData aRef;
                        ScRange aRefRange = rTopPos;
                        aRefRange.aEnd.SetRow(rTopPos.Row() + nRowEnd);
                        aRef.InitRange(aRefRange);
                        formula::FormulaTokenRef xTok(new ScMatrixRangeToken(pMat, aRef));
                        aCachedTokens.insert(CachedTokensType::value_type(p, xTok));
                        aCode2.AddToken(*xTok);
                    }
                    else
                    {
                        ScMatrixToken aTok(pMat);
                        aCode2.AddToken(aTok);
                    }
                }
                break;
                default:
                    aCode2.AddToken(*p);
            }
        }

        ScFormulaCell* pDest = rDoc.GetFormulaCell(aTmpPos);
        if (!pDest)
            return false;

        generateRPNCode(rDoc, aTmpPos, aCode2);
        ScInterpreter aInterpreter(pDest, &rDoc, aTmpPos, aCode2);
        aInterpreter.Interpret();
        aResults.push_back(aInterpreter.GetResultToken()->GetDouble());
    } // for loop end (xGroup->mnLength)

    if (!aResults.empty())
        rDoc.SetFormulaResults(rTopPos, &aResults[0], aResults.size());

    return true;
}

// TODO: load module, hook symbol out, check it works, UI on failure etc.
namespace opencl {
    extern sc::FormulaGroupInterpreter *createFormulaGroupInterpreter();
}
FormulaGroupInterpreter *FormulaGroupInterpreter::msInstance = NULL;

#if USE_DUMMY_INTERPRETER
class FormulaGroupInterpreterDummy : public FormulaGroupInterpreter
{
    enum Mode {
        WRITE_OUTPUT = 0
    };
    Mode meMode;
public:
    FormulaGroupInterpreterDummy()
    {
        const char *pValue = getenv("FORMULA_GROUP_DUMMY");
        meMode = static_cast<Mode>(OString(pValue, strlen(pValue)).toInt32());
        fprintf(stderr, "Using Dummy Formula Group interpreter mode %d\n", (int)meMode);
    }

    virtual ScMatrixRef inverseMatrix(const ScMatrix& /*rMat*/)
    {
        return ScMatrixRef();
    }

    virtual bool interpret(ScDocument& rDoc, const ScAddress& rTopPos,
                           const ScFormulaCellGroupRef& xGroup,
                           ScTokenArray& rCode)
    {
        (void)rCode;

        // Write simple data back into the sheet
        if (meMode == WRITE_OUTPUT)
        {
            double *pDoubles = new double[xGroup->mnLength];
            for (sal_Int32 i = 0; i < xGroup->mnLength; i++)
                pDoubles[i] = 42.0 + i;
            rDoc.SetFormulaResults(rTopPos, pDoubles, xGroup->mnLength);
            delete [] pDoubles;
        }
        return true;
    }
};
#endif

/// load and/or configure the correct formula group interpreter
FormulaGroupInterpreter *FormulaGroupInterpreter::getStatic()
{
    static bool bOpenCLEnabled = false;

#if USE_DUMMY_INTERPRETER
    if (getenv("FORMULA_GROUP_DUMMY"))
    {
        delete msInstance;
        return msInstance = new sc::FormulaGroupInterpreterDummy();
    }
#endif

    if ( msInstance &&
         bOpenCLEnabled != ScInterpreter::GetGlobalConfig().mbOpenCLEnabled )
    {
        bOpenCLEnabled = ScInterpreter::GetGlobalConfig().mbOpenCLEnabled;
        delete msInstance;
        msInstance = NULL;
    }

    if ( !msInstance )
    {
#if HAVE_FEATURE_OPENCL
        if ( ScInterpreter::GetGlobalConfig().mbOpenCLEnabled )
            msInstance = sc::opencl::createFormulaGroupInterpreter();
#endif
        if ( !msInstance ) // software fallback
        {
            fprintf(stderr, "Create S/W interp\n");
            msInstance = new sc::FormulaGroupInterpreterSoftware();
        }
    }

    return msInstance;
}

void FormulaGroupInterpreter::generateRPNCode(ScDocument& rDoc, const ScAddress& rPos, ScTokenArray& rCode)
{
    // First, generate an RPN (reverse polish notation) token array.
    ScCompiler aComp(&rDoc, rPos, rCode);
    aComp.SetGrammar(rDoc.GetGrammar());
    aComp.CompileTokenArray(); // Create RPN token array.
    // Now, calling FirstRPN() and NextRPN() will return tokens from the RPN token array.
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
