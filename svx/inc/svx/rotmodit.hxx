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

#ifndef _SVX_ROTMODIT_HXX
#define _SVX_ROTMODIT_HXX

#include <svl/eitem.hxx>
#include "svx/svxdllapi.h"

//----------------------------------------------------------------------------
//  Alignment with rotated text

enum SvxRotateMode
{
    SVX_ROTATE_MODE_STANDARD,
    SVX_ROTATE_MODE_TOP,
    SVX_ROTATE_MODE_CENTER,
    SVX_ROTATE_MODE_BOTTOM
};

class SVX_DLLPUBLIC SvxRotateModeItem: public SfxEnumItem
{
public:
                TYPEINFO();

                SvxRotateModeItem( SvxRotateMode eMode=SVX_ROTATE_MODE_STANDARD, sal_uInt16 nWhich=0);
                SvxRotateModeItem( const SvxRotateModeItem& rItem );
                ~SvxRotateModeItem();

    virtual sal_uInt16              GetValueCount() const;
    virtual String              GetValueText( sal_uInt16 nVal ) const;
    virtual SfxPoolItem*        Clone( SfxItemPool *pPool = 0 ) const;
    virtual SfxPoolItem*        Create(SvStream &, sal_uInt16) const;
    virtual sal_uInt16              GetVersion( sal_uInt16 nFileVersion ) const;
    virtual SfxItemPresentation GetPresentation( SfxItemPresentation ePres,
                                                 SfxMapUnit eCoreMetric,
                                                 SfxMapUnit ePresMetric,
                                                 String& rText, const IntlWrapper * = 0 ) const;
    virtual bool                QueryValue( com::sun::star::uno::Any& rVal, sal_uInt8 nMemberId = 0 ) const;
    virtual bool                PutValue( const com::sun::star::uno::Any& rVal, sal_uInt8 nMemberId = 0 );
};


#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */