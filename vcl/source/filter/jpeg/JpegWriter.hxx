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

#ifndef INCLUDED_VCL_SOURCE_FILTER_JPEG_JPEGWRITER_HXX
#define INCLUDED_VCL_SOURCE_FILTER_JPEG_JPEGWRITER_HXX

#include <vcl/graph.hxx>
#include <vcl/fltcall.hxx>
#include <com/sun/star/uno/Sequence.h>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/task/XStatusIndicator.hpp>

class JPEGWriter
{
    SvStream&           mrStream;
    Bitmap              maBitmap;
    BitmapReadAccess*   mpReadAccess;
    sal_uInt8*          mpBuffer;
    bool                mbNative;
    sal_Bool            mbGreys;
    sal_Int32           mnQuality;
    sal_Int32           maChromaSubsampling;

    bool*               mpExpWasGrey;

    com::sun::star::uno::Reference< com::sun::star::task::XStatusIndicator > mxStatusIndicator;

public:
    JPEGWriter( SvStream& rStream,
                const ::com::sun::star::uno::Sequence< ::com::sun::star::beans::PropertyValue >* pFilterData,
                bool* pExportWasGrey = NULL );

    virtual ~JPEGWriter() {};

    void*       GetScanline( long nY );
    sal_Bool    Write( const Graphic& rGraphic );

};

#endif // INCLUDED_VCL_SOURCE_FILTER_JPEG_JPEGWRITER_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
