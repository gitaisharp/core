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

#include <svgtspannode.hxx>

namespace svgio::svgreader
{
        SvgTspanNode::SvgTspanNode(
            SvgDocument& rDocument,
            SvgNode* pParent)
        :   SvgNode(SVGToken::Tspan, rDocument, pParent),
            maSvgStyleAttributes(*this)
        {
        }

        SvgTspanNode::~SvgTspanNode()
        {
        }

        const SvgStyleAttributes* SvgTspanNode::getSvgStyleAttributes() const
        {
            // #i125293# Need to support CssStyles in tspan text sections
            return checkForCssStyle(maSvgStyleAttributes);
        }

        void SvgTspanNode::parseAttribute(const OUString& rTokenName, SVGToken aSVGToken, const OUString& aContent)
        {
            // call parent
            SvgNode::parseAttribute(rTokenName, aSVGToken, aContent);

            // read style attributes
            maSvgStyleAttributes.parseStyleAttribute(aSVGToken, aContent);

            // read text position attributes
            maSvgTextPositions.parseTextPositionAttributes(aSVGToken, aContent);

            // parse own
            switch(aSVGToken)
            {
                case SVGToken::Style:
                {
                    readLocalCssStyle(aContent);
                    break;
                }
                default:
                {
                    break;
                }
            }
        }

        double SvgTspanNode::getCurrentFontSize() const
        {
            return getCurrentFontSizeInherited();
        }

} // end of namespace svgio::svgreader

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
