/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#ifndef SC_XMLSTYLESIMPORTHELPER_HXX
#define SC_XMLSTYLESIMPORTHELPER_HXX

#include "rangelst.hxx"
#include "simplerangelist.hxx"
#include <rtl/ustring.hxx>

#include <set>
#include <vector>
#include <list>
#include <boost/shared_ptr.hpp>

class ScXMLImport;

struct ScMyStyleNumberFormat
{
    OUString       sStyleName;
    sal_Int32           nNumberFormat;

    ScMyStyleNumberFormat() : nNumberFormat(-1) {}
    ScMyStyleNumberFormat(const OUString& rStyleName) :
        sStyleName(rStyleName), nNumberFormat(-1) {}
    ScMyStyleNumberFormat(const OUString& rStyleName, const sal_Int32 nFormat) :
        sStyleName(rStyleName), nNumberFormat(nFormat) {}
};

struct LessStyleNumberFormat
{
    bool operator() (const ScMyStyleNumberFormat& rValue1, const ScMyStyleNumberFormat& rValue2) const
    {
        return rValue1.sStyleName < rValue2.sStyleName;
    }
};

typedef std::set< ScMyStyleNumberFormat, LessStyleNumberFormat >    ScMyStyleNumberFormatSet;

class ScMyStyleNumberFormats
{
    ScMyStyleNumberFormatSet    aSet;

public:
    void AddStyleNumberFormat(const OUString& rStyleName, const sal_Int32 nNumberFormat);
    sal_Int32 GetStyleNumberFormat(const OUString& rStyleName);
};

struct ScMyCurrencyStyle
{
    OUString       sCurrency;
    ::boost::shared_ptr<ScSimpleRangeList> mpRanges;

    ScMyCurrencyStyle() :
        mpRanges(new ScSimpleRangeList)
    {}
    ~ScMyCurrencyStyle() {}
};

struct LessCurrencyStyle
{
    bool operator() (const ScMyCurrencyStyle& rValue1, const ScMyCurrencyStyle& rValue2) const
    {
        return rValue1.sCurrency < rValue2.sCurrency;
    }
};

typedef std::set<ScMyCurrencyStyle, LessCurrencyStyle>  ScMyCurrencyStylesSet;

class ScMyStyleRanges : public SvRefBase
{
    ::boost::shared_ptr<ScSimpleRangeList> mpTextList;
    ::boost::shared_ptr<ScSimpleRangeList> mpNumberList;
    ::boost::shared_ptr<ScSimpleRangeList> mpTimeList;
    ::boost::shared_ptr<ScSimpleRangeList> mpDateTimeList;
    ::boost::shared_ptr<ScSimpleRangeList> mpPercentList;
    ::boost::shared_ptr<ScSimpleRangeList> mpLogicalList;
    ::boost::shared_ptr<ScSimpleRangeList> mpUndefinedList;
    ScMyCurrencyStylesSet*  pCurrencyList;

    void SetStylesToRanges(const ::std::list<ScRange>& rList,
        const OUString* pStyleName, const sal_Int16 nCellType,
        const OUString* pCurrency, ScXMLImport& rImport);
public:
    ScMyStyleRanges();
    ~ScMyStyleRanges();
    void AddRange(const ScRange& rRange, const sal_Int16 nType);
    void AddCurrencyRange(const ScRange& rRange, const OUString* pCurrency);
    void InsertCol(const sal_Int32 nCol, const sal_Int32 nTab, ScDocument* pDoc);
    void SetStylesToRanges(const OUString* pStyleName, ScXMLImport& rImport);
};
SV_DECL_IMPL_REF( ScMyStyleRanges );

struct ScMyStyle
{
    OUString       sStyleName;
    ScMyStyleRangesRef  xRanges;

    ScMyStyle() : xRanges(new ScMyStyleRanges()) {}
    ~ScMyStyle() {}
};

struct LessStyle
{
    bool operator() (const ScMyStyle& rValue1, const ScMyStyle& rValue2) const
    {
        return rValue1.sStyleName < rValue2.sStyleName;
    }
};

typedef std::set<ScMyStyle, LessStyle>  ScMyStylesSet;
typedef std::vector<ScMyStylesSet::iterator> ScMyStyles;

class ScMyStylesImportHelper
{
    ScMyStylesSet       aCellStyles;
    ScMyStyles          aColDefaultStyles;
    ScMyStylesSet::iterator aRowDefaultStyle;
    ScXMLImport&        rImport;
    OUString*      pStyleName;
    OUString*      pPrevStyleName;
    OUString*      pCurrency;
    OUString*      pPrevCurrency;
    ScRange             aPrevRange;
    sal_Int16           nCellType;
    sal_Int16           nPrevCellType;
    bool                bPrevRangeAdded;

    void ResetAttributes();
    ScMyStylesSet::iterator GetIterator(const OUString* pStyleName);
    void AddDefaultRange(const ScRange& rRange);
    void AddSingleRange(const ScRange& rRange);
    void AddRange();
    bool IsEqual(const OUString* pFirst, const OUString* pSecond)
    {
        return ((pFirst && pSecond && pFirst->equals(*pSecond)) ||
                (!pFirst && !pSecond) ||
                (!pFirst && pSecond && pSecond->isEmpty()) ||
                (!pSecond && pFirst && pFirst->isEmpty()));
    }
public:
    ScMyStylesImportHelper(ScXMLImport& rImport);
    ~ScMyStylesImportHelper();
    void AddColumnStyle(const OUString& rStyleName, const sal_Int32 nColumn, const sal_Int32 nRepeat);
    void SetRowStyle(const OUString& rStyleName);
    void SetAttributes(OUString* pStyleName,
        OUString* pCurrency, const sal_Int16 nCellType);
    void AddRange(const ScRange& rRange);
    void AddCell(const ScAddress& rAddress);
    void InsertCol(const sal_Int32 nCol, const sal_Int32 nTab, ScDocument* pDoc); // a col is inserted before nCol
    void EndTable();
    void SetStylesToRanges();
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
