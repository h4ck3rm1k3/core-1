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
#ifndef INCLUDED_SW_INC_FMTINFMT_HXX
#define INCLUDED_SW_INC_FMTINFMT_HXX

#include <svl/poolitem.hxx>
#include "swdllapi.h"

class SvxMacro;
class SvxMacroTableDtor;
class SwTxtINetFmt;
class IntlWrapper;

// ATT_INETFMT

class SW_DLLPUBLIC SwFmtINetFmt: public SfxPoolItem
{
    friend class SwTxtINetFmt;

    rtl::OUString aURL;                  ///< URL.
    rtl::OUString aTargetFrame;          ///< Target frame for URL.
    rtl::OUString aINetFmt;
    rtl::OUString aVisitedFmt;
    rtl::OUString aName;                 ///< Name of the link.
    SvxMacroTableDtor* pMacroTbl;
    SwTxtINetFmt* pTxtAttr;         ///< My TextAttribute.
    sal_uInt16 nINetId;
    sal_uInt16 nVisitedId;
public:
    SwFmtINetFmt( const rtl::OUString& rURL, const rtl::OUString& rTarget );
    SwFmtINetFmt( const SwFmtINetFmt& rAttr );
    SwFmtINetFmt();                     ///< For TypeInfo.
    virtual ~SwFmtINetFmt();

    TYPEINFO();

    /// "Pure virtual methods" of SfxPoolItem.
    virtual int             operator==( const SfxPoolItem& ) const;
    virtual SfxPoolItem*    Clone( SfxItemPool* pPool = 0 ) const;
    virtual SfxItemPresentation GetPresentation( SfxItemPresentation ePres,
                                    SfxMapUnit eCoreMetric,
                                    SfxMapUnit ePresMetric,
                                    rtl::OUString &rText,
                                    const IntlWrapper* pIntl = 0 ) const;

    virtual bool QueryValue( com::sun::star::uno::Any& rVal,
                                        sal_uInt8 nMemberId = 0 ) const;
    virtual bool PutValue( const com::sun::star::uno::Any& rVal,
                                        sal_uInt8 nMemberId = 0 );


    const SwTxtINetFmt* GetTxtINetFmt() const   { return pTxtAttr; }
    SwTxtINetFmt* GetTxtINetFmt()               { return pTxtAttr; }

    rtl::OUString GetValue() const               { return aURL; }

    rtl::OUString GetName() const                { return aName; }
    void SetName( const rtl::OUString& rNm )     { aName = rNm; }

    rtl::OUString GetTargetFrame() const         { return aTargetFrame; }

    rtl::OUString GetINetFmt() const             { return aINetFmt; }
    void SetINetFmt( const rtl::OUString& rNm )  { aINetFmt = rNm; }

    rtl::OUString GetVisitedFmt() const            { return aVisitedFmt; }
    void SetVisitedFmt( const rtl::OUString& rNm ) { aVisitedFmt = rNm; }

    sal_uInt16 GetINetFmtId() const             { return nINetId; }
    void SetINetFmtId( sal_uInt16 nNew )        { nINetId = nNew; }

    sal_uInt16 GetVisitedFmtId() const          { return nVisitedId; }
    void SetVisitedFmtId( sal_uInt16 nNew )     { nVisitedId = nNew; }

    /// Set a new MacroTable or clear the current one.
    void SetMacroTbl( const SvxMacroTableDtor* pTbl = 0 );
    const SvxMacroTableDtor* GetMacroTbl() const    { return pMacroTbl; }

    /// Macro getter and setter.
    void SetMacro( sal_uInt16 nEvent, const SvxMacro& rMacro );
    const SvxMacro* GetMacro( sal_uInt16 nEvent ) const;
};


#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
