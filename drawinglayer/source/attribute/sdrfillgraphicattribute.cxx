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

#include <drawinglayer/attribute/sdrfillgraphicattribute.hxx>
#include <drawinglayer/attribute/fillgraphicattribute.hxx>
#include <rtl/instance.hxx>
#include <vcl/graph.hxx>

//////////////////////////////////////////////////////////////////////////////

namespace drawinglayer
{
    namespace attribute
    {
        class ImpSdrFillGraphicAttribute
        {
        public:
            // data definitions
            Graphic                                 maFillGraphic;
            basegfx::B2DVector                      maSize;
            basegfx::B2DVector                      maOffset;
            basegfx::B2DVector                      maOffsetPosition;
            basegfx::B2DVector                      maRectPoint;

            // bitfield
            unsigned                                mbTiling : 1;
            unsigned                                mbStretch : 1;
            unsigned                                mbLogSize : 1;

            ImpSdrFillGraphicAttribute(
                const Graphic& rFillGraphic,
                const basegfx::B2DVector& rSize,
                const basegfx::B2DVector& rOffset,
                const basegfx::B2DVector& rOffsetPosition,
                const basegfx::B2DVector& rRectPoint,
                bool bTiling,
                bool bStretch,
                bool bLogSize)
            :   maFillGraphic(rFillGraphic),
                maSize(rSize),
                maOffset(rOffset),
                maOffsetPosition(rOffsetPosition),
                maRectPoint(rRectPoint),
                mbTiling(bTiling),
                mbStretch(bStretch),
                mbLogSize(bLogSize)
            {
            }

            ImpSdrFillGraphicAttribute()
            :   maFillGraphic(Graphic()),
                maSize(basegfx::B2DVector()),
                maOffset(basegfx::B2DVector()),
                maOffsetPosition(basegfx::B2DVector()),
                maRectPoint(basegfx::B2DVector()),
                mbTiling(false),
                mbStretch(false),
                mbLogSize(false)
            {
            }

            // data read access
            const Graphic& getFillGraphic() const { return maFillGraphic; }
            const basegfx::B2DVector& getSize() const { return maSize; }
            const basegfx::B2DVector& getOffset() const { return maOffset; }
            const basegfx::B2DVector& getOffsetPosition() const { return maOffsetPosition; }
            const basegfx::B2DVector& getRectPoint() const { return maRectPoint; }
            bool getTiling() const { return mbTiling; }
            bool getStretch() const { return mbStretch; }
            bool getLogSize() const { return mbLogSize; }

            bool operator==(const ImpSdrFillGraphicAttribute& rCandidate) const
            {
                return (getFillGraphic() == rCandidate.getFillGraphic()
                    && getSize() == rCandidate.getSize()
                    && getOffset() == rCandidate.getOffset()
                    && getOffsetPosition() == rCandidate.getOffsetPosition()
                    && getRectPoint() == rCandidate.getRectPoint()
                    && getTiling() == rCandidate.getTiling()
                    && getStretch() == rCandidate.getStretch()
                    && getLogSize() == rCandidate.getLogSize());
            }
        };

        namespace
        {
            struct theGlobalDefault :
                public rtl::Static< SdrFillGraphicAttribute::ImplType, theGlobalDefault > {};
        }

        SdrFillGraphicAttribute::SdrFillGraphicAttribute(
            const Graphic& rFillGraphic,
            const basegfx::B2DVector& rSize,
            const basegfx::B2DVector& rOffset,
            const basegfx::B2DVector& rOffsetPosition,
            const basegfx::B2DVector& rRectPoint,
            bool bTiling,
            bool bStretch,
            bool bLogSize)
        :   mpSdrFillGraphicAttribute(
                ImpSdrFillGraphicAttribute(
                    rFillGraphic,
                    rSize,
                    rOffset,
                    rOffsetPosition,
                    rRectPoint,
                    bTiling,
                    bStretch,
                    bLogSize))
        {
        }

        SdrFillGraphicAttribute::SdrFillGraphicAttribute()
        :   mpSdrFillGraphicAttribute(theGlobalDefault::get())
        {
        }

        SdrFillGraphicAttribute::SdrFillGraphicAttribute(const SdrFillGraphicAttribute& rCandidate)
        :   mpSdrFillGraphicAttribute(rCandidate.mpSdrFillGraphicAttribute)
        {
        }

        SdrFillGraphicAttribute::~SdrFillGraphicAttribute()
        {
        }

        bool SdrFillGraphicAttribute::isDefault() const
        {
            return mpSdrFillGraphicAttribute.same_object(theGlobalDefault::get());
        }

        SdrFillGraphicAttribute& SdrFillGraphicAttribute::operator=(const SdrFillGraphicAttribute& rCandidate)
        {
            mpSdrFillGraphicAttribute = rCandidate.mpSdrFillGraphicAttribute;
            return *this;
        }

        bool SdrFillGraphicAttribute::operator==(const SdrFillGraphicAttribute& rCandidate) const
        {
            return rCandidate.mpSdrFillGraphicAttribute == mpSdrFillGraphicAttribute;
        }

        const Graphic& SdrFillGraphicAttribute::getFillGraphic() const
        {
            return mpSdrFillGraphicAttribute->getFillGraphic();
        }

        const basegfx::B2DVector& SdrFillGraphicAttribute::getSize() const
        {
            return mpSdrFillGraphicAttribute->getSize();
        }

        const basegfx::B2DVector& SdrFillGraphicAttribute::getOffset() const
        {
            return mpSdrFillGraphicAttribute->getOffset();
        }

        const basegfx::B2DVector& SdrFillGraphicAttribute::getOffsetPosition() const
        {
            return mpSdrFillGraphicAttribute->getOffsetPosition();
        }

        const basegfx::B2DVector& SdrFillGraphicAttribute::getRectPoint() const
        {
            return mpSdrFillGraphicAttribute->getRectPoint();
        }

        bool SdrFillGraphicAttribute::getTiling() const
        {
            return mpSdrFillGraphicAttribute->getTiling();
        }

        bool SdrFillGraphicAttribute::getStretch() const
        {
            return mpSdrFillGraphicAttribute->getStretch();
        }

        bool SdrFillGraphicAttribute::getLogSize() const
        {
            return mpSdrFillGraphicAttribute->getLogSize();
        }

        FillGraphicAttribute SdrFillGraphicAttribute::createFillGraphicAttribute(const basegfx::B2DRange& rRange) const
        {
            // get logical size of bitmap (before expanding eventually)
            Graphic aGraphic(getFillGraphic());
            const basegfx::B2DVector aLogicalSize(aGraphic.GetPrefSize().getWidth(), aGraphic.GetPrefSize().getHeight());

            // init values with defaults
            basegfx::B2DPoint aBitmapSize(1.0, 1.0);
            basegfx::B2DVector aBitmapTopLeft(0.0, 0.0);

            // are changes needed?
            if(getTiling() || !getStretch())
            {
                // init values with range sizes
                const double fRangeWidth(0.0 != rRange.getWidth() ? rRange.getWidth() : 1.0);
                const double fRangeHeight(0.0 != rRange.getHeight() ? rRange.getHeight() : 1.0);
                aBitmapSize = basegfx::B2DPoint(fRangeWidth, fRangeHeight);

                // size changes
                if(0.0 != getSize().getX())
                {
                    if(getSize().getX() < 0.0)
                    {
                        aBitmapSize.setX(aBitmapSize.getX() * (getSize().getX() * -0.01));
                    }
                    else
                    {
                        aBitmapSize.setX(getSize().getX());
                    }
                }
                else
                {
                    aBitmapSize.setX(aLogicalSize.getX());
                }

                if(0.0 != getSize().getY())
                {
                    if(getSize().getY() < 0.0)
                    {
                        aBitmapSize.setY(aBitmapSize.getY() * (getSize().getY() * -0.01));
                    }
                    else
                    {
                        aBitmapSize.setY(getSize().getY());
                    }
                }
                else
                {
                    aBitmapSize.setY(aLogicalSize.getY());
                }

                // get values, force to centered if necessary
                const basegfx::B2DVector aRectPoint(getTiling() ? getRectPoint() : basegfx::B2DVector(0.0, 0.0));

                // position changes X
                if(0.0 == aRectPoint.getX())
                {
                    aBitmapTopLeft.setX((fRangeWidth - aBitmapSize.getX()) * 0.5);
                }
                else if(1.0 == aRectPoint.getX())
                {
                    aBitmapTopLeft.setX(fRangeWidth - aBitmapSize.getX());
                }

                if(getTiling() && 0.0 != getOffsetPosition().getX())
                {
                    aBitmapTopLeft.setX(aBitmapTopLeft.getX() + (aBitmapSize.getX() * (getOffsetPosition().getX() * 0.01)));
                }

                // position changes Y
                if(0.0 == aRectPoint.getY())
                {
                    aBitmapTopLeft.setY((fRangeHeight - aBitmapSize.getY()) * 0.5);
                }
                else if(1.0 == aRectPoint.getY())
                {
                    aBitmapTopLeft.setY(fRangeHeight - aBitmapSize.getY());
                }

                if(getTiling() && 0.0 != getOffsetPosition().getY())
                {
                    aBitmapTopLeft.setY(aBitmapTopLeft.getY() + (aBitmapSize.getY() * (getOffsetPosition().getY() * 0.01)));
                }

                // apply bitmap size scaling to unit rectangle
                aBitmapTopLeft.setX(aBitmapTopLeft.getX() / fRangeWidth);
                aBitmapTopLeft.setY(aBitmapTopLeft.getY() / fRangeHeight);
                aBitmapSize.setX(aBitmapSize.getX() / fRangeWidth);
                aBitmapSize.setY(aBitmapSize.getY() / fRangeHeight);
            }

            // get offset in percent
            const double fOffsetX(basegfx::clamp(getOffset().getX() * 0.01, 0.0, 1.0));
            const double fOffsetY(basegfx::clamp(getOffset().getY() * 0.01, 0.0, 1.0));

            // create FillGraphicAttribute
            return FillGraphicAttribute(
                aGraphic,
                basegfx::B2DRange(aBitmapTopLeft, aBitmapTopLeft + aBitmapSize),
                getTiling(),
                fOffsetX,
                fOffsetY);
        }
    } // end of namespace attribute
} // end of namespace drawinglayer

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
