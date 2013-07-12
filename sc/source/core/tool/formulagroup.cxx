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
#include "scmatrix.hxx"

#include "formula/vectortoken.hxx"

#include <vector>
#include <boost/unordered_map.hpp>

#define USE_DUMMY_INTERPRETER 1

#if USE_DUMMY_INTERPRETER
#include <cstdio>
#endif

namespace sc {

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
                    for (size_t nCol = 0; nCol < nColSize; ++nCol)
                    {
                        const double* pArray = rArrays[nCol];
                        pArray += nRowStart;
                        pMat->PutDouble(pArray, nRowSize, nCol, 0);
                    }

                    if (p2->IsStartFixed() && p2->IsEndFixed())
                    {
                        // Cached the converted token for absolute range referene.
                        formula::FormulaTokenRef xTok(new ScMatrixToken(pMat));
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
            msInstance = new sc::FormulaGroupInterpreterSoftware();
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
