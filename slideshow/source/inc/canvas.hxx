/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_SLIDESHOW_CANVAS_HXX
#define INCLUDED_SLIDESHOW_CANVAS_HXX

#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/rendering/XCanvas.hpp>
#include <com/sun/star/rendering/ViewState.hpp>
#include <drawinglayer/geometry/viewinformation2d.hxx>

namespace slideshow
{
    namespace internal
    {
        /// XCanvas and state parameters to render stuff
        struct Canvas
        {
            css::uno::Reference< css::rendering::XCanvas > mxCanvas;
            css::rendering::ViewState                      maViewState;
            ::drawinglayer::geometry::ViewInformation2D    maViewInfo;
        };
    }
}

#endif /* INCLUDED_SLIDESHOW_CANVAS_HXX */

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
