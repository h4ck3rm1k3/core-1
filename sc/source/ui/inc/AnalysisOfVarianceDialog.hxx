/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#ifndef ANALYSIS_OF_VARIANCE_DIALOG_HXX
#define ANALYSIS_OF_VARIANCE_DIALOG_HXX

#include "global.hxx"
#include "address.hxx"
#include "anyrefdg.hxx"

#include <vcl/fixed.hxx>
#include <vcl/group.hxx>
#include <vcl/lstbox.hxx>

#include "StatisticsInputOutputDialog.hxx"

class ScAnalysisOfVarianceDialog : public ScStatisticsInputOutputDialog
{
private:
    NumericField* mpAlpha;

public:
    ScAnalysisOfVarianceDialog(
        SfxBindings* pB, SfxChildWindow* pCW,
        Window* pParent, ScViewData* pViewData );

    virtual ~ScAnalysisOfVarianceDialog();

    virtual sal_Bool Close();

protected:
    virtual sal_Int16 GetUndoNameId();
    virtual ScRange ApplyOutput(ScDocShell* pDocShell);
};

#endif


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
