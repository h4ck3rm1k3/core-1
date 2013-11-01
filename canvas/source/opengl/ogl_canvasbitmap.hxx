/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef OGL_CANVASBITMAP_HXX
#define OGL_CANVASBITMAP_HXX

#include <cppuhelper/compbase2.hxx>

#include <com/sun/star/rendering/XBitmapCanvas.hpp>
#include <com/sun/star/rendering/XIntegerBitmap.hpp>

#include <canvas/base/integerbitmapbase.hxx>
#include <canvas/base/disambiguationhelper.hxx>
#include <basegfx/vector/b2isize.hxx>

#include <boost/shared_ptr.hpp>

#include "ogl_bitmapcanvashelper.hxx"
#include "ogl_spritecanvas.hxx"


/* Definition of CanvasBitmap class */

namespace oglcanvas
{
    typedef ::cppu::WeakComponentImplHelper2< ::com::sun::star::rendering::XBitmapCanvas,
                                              ::com::sun::star::rendering::XIntegerBitmap > CanvasBitmapBase_Base;
    typedef ::canvas::IntegerBitmapBase<
        ::canvas::DisambiguationHelper< CanvasBitmapBase_Base >,
        BitmapCanvasHelper,
        ::osl::MutexGuard,
        ::cppu::OWeakObject >                          CanvasBitmapBaseT;

    class CanvasBitmap : public CanvasBitmapBaseT
    {
    public:
        /** Create a canvas bitmap for the given surface

            @param width
            Size of the bitmap

            @param height
            Size of the bitmap

            @param rDevice
            Reference device, with which bitmap should be compatible
         */
        CanvasBitmap( sal_Int32              width,
                      sal_Int32              height,
                      const SpriteCanvasRef& rDevice,
                      SpriteDeviceHelper&    rDeviceHelper,
                      bool                   bHasAlpha );

        /** Create verbatim copy (including all recorded actions)
         */
        CanvasBitmap( const CanvasBitmap& rSrc );

        /// Dispose all internal references
        virtual void disposeThis();

        /** Write out recorded actions
         */
        bool renderRecordedActions() const;

    private:
        /** MUST hold here, too, since CanvasHelper only contains a
            raw pointer (without refcounting)
        */
        SpriteCanvasRef mpDevice;
        bool            mbHasAlpha;
    };
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
