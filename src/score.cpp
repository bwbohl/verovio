/////////////////////////////////////////////////////////////////////////////
// Name:        score.cpp
// Author:      Laurent Pugin
// Created:     29/08/2016
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "score.h"

//----------------------------------------------------------------------------

#include <assert.h>

//----------------------------------------------------------------------------

#include "doc.h"
#include "editorial.h"
#include "ending.h"
#include "functorparams.h"
#include "page.h"
#include "pb.h"
#include "sb.h"
#include "scoredef.h"
#include "section.h"
#include "system.h"
#include "vrv.h"

namespace vrv {

//----------------------------------------------------------------------------
// Score
//----------------------------------------------------------------------------

static const ClassRegistrar<Score> s_factory("score", SCORE);

Score::Score() : PageElement("score-"), PageElementStartInterface(), AttLabelled(), AttNNumberLike()
{
    RegisterAttClass(ATT_LABELLED);
    RegisterAttClass(ATT_NNUMBERLIKE);

    Reset();
}

Score::~Score() {}

void Score::Reset()
{
    Object::Reset();
    ResetLabelled();
    ResetNNumberLike();

    m_scoreDef.Reset();
}

bool Score::IsSupportedChild(Object *child)
{
    if (child->Is(SCOREDEF)) {
        assert(dynamic_cast<ScoreDef *>(child));
    }
    else if (child->Is(SB)) {
        assert(dynamic_cast<Sb *>(child));
    }
    else if (child->Is(SECTION)) {
        assert(dynamic_cast<Section *>(child));
    }
    else if (child->Is(ENDING)) {
        assert(dynamic_cast<Ending *>(child));
    }
    else if (child->Is(PB)) {
        assert(dynamic_cast<Pb *>(child));
    }
    else if (child->IsEditorialElement()) {
        assert(dynamic_cast<EditorialElement *>(child));
    }
    else {
        return false;
    }
    return true;
}

void Score::SetAsCurrent()
{
    Doc *doc = vrv_cast<Doc *>(this->GetFirstAncestor(DOC));
    // The doc can be NULL when doing the castoff and the pages are no attached to the doc
    // If such cases, it will not matter not to have the current scoreDef in the doc
    if (doc) {
        doc->SetCurrentScore(this);
    }
}

//----------------------------------------------------------------------------
// Functor methods
//----------------------------------------------------------------------------

int Score::AdjustDots(FunctorParams *functorParams)
{
    AdjustDotsParams *params = vrv_params_cast<AdjustDotsParams *>(functorParams);
    assert(params);

    params->m_staffNs = params->m_doc->GetCurrentScoreDef()->GetStaffNs();

    return FUNCTOR_CONTINUE;
}

int Score::AdjustLayers(FunctorParams *functorParams)
{
    AdjustLayersParams *params = vrv_params_cast<AdjustLayersParams *>(functorParams);
    assert(params);

    params->m_staffNs = params->m_doc->GetCurrentScoreDef()->GetStaffNs();

    return FUNCTOR_CONTINUE;
}

int Score::AdjustXPos(FunctorParams *functorParams)
{
    AdjustXPosParams *params = vrv_params_cast<AdjustXPosParams *>(functorParams);
    assert(params);

    params->m_staffNs = params->m_doc->GetCurrentScoreDef()->GetStaffNs();

    return FUNCTOR_CONTINUE;
}

int Score::AdjustGraceXPos(FunctorParams *functorParams)
{
    AdjustGraceXPosParams *params = vrv_params_cast<AdjustGraceXPosParams *>(functorParams);
    assert(params);

    params->m_staffNs = params->m_doc->GetCurrentScoreDef()->GetStaffNs();

    return FUNCTOR_CONTINUE;
}

int Score::ConvertToPageBased(FunctorParams *functorParams)
{
    ConvertToPageBasedParams *params = vrv_params_cast<ConvertToPageBasedParams *>(functorParams);
    assert(params);

    assert(!params->m_currentSystem);
    this->MoveItselfTo(params->m_page);
    System *system = new System();
    params->m_currentSystem = system;
    params->m_page->AddChild(system);

    return FUNCTOR_CONTINUE;
}

int Score::ConvertToPageBasedEnd(FunctorParams *functorParams)
{
    ConvertToPageBasedParams *params = vrv_params_cast<ConvertToPageBasedParams *>(functorParams);
    assert(params);

    ConvertToPageBasedBoundary(this, params->m_page);
    params->m_currentSystem = NULL;

    return FUNCTOR_CONTINUE;
}

int Score::UnCastOff(FunctorParams *functorParams)
{
    UnCastOffParams *params = vrv_params_cast<UnCastOffParams *>(functorParams);
    assert(params);

    PageElement::UnCastOff(functorParams);

    assert(!params->m_currentSystem);
    System *system = new System();
    params->m_currentSystem = system;
    params->m_page->AddChild(system);

    return FUNCTOR_CONTINUE;
}

int Score::ConvertToCastOffMensural(FunctorParams *functorParams)
{
    ConvertToCastOffMensuralParams *params = vrv_params_cast<ConvertToCastOffMensuralParams *>(functorParams);
    assert(params);

    PageElement::ConvertToCastOffMensural(functorParams);

    // assert(!params->m_targetSystem);
    System *system = new System();
    params->m_targetSystem = system;
    params->m_page->AddChild(system);

    return FUNCTOR_CONTINUE;
}

} // namespace vrv
