/////////////////////////////////////////////////////////////////////////////
// Name:        tabdursym.h
// Author:      Laurent Pugin
// Created:     2019
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_TABDURSYM_H__
#define __VRV_TABDURSYM_H__

#include "atts_shared.h"
#include "drawinginterface.h"
#include "layerelement.h"

namespace vrv {

//----------------------------------------------------------------------------
// TabDurSym
//----------------------------------------------------------------------------

/**
 * This class models the MEI <tabDurSym> element.
 */
class TabDurSym : public LayerElement, public StemmedDrawingInterface, public AttNNumberLike {
public:
    /**
     * @name Constructors, destructors, and other standard methods
     * Reset method reset all attribute classes
     */
    ///@{
    TabDurSym();
    virtual ~TabDurSym();
    void Reset() override;
    std::string GetClassName() const override { return "TabDurSym"; }
    ///@}

    /**
     * @name Getter to interfaces
     */
    ///@{
    StemmedDrawingInterface *GetStemmedDrawingInterface() override
    {
        return dynamic_cast<StemmedDrawingInterface *>(this);
    }
    ///@}

    /** Override the method since alignment is required */
    bool HasToBeAligned() const override { return true; }

    /**
     * Add an element to a element.
     */
    bool IsSupportedChild(Object *object) override;

    /**
     * Overwritten method for note
     */
    void AddChild(Object *object) override;

    /**
     * Get the stem up / stem down attachment point.
     * If necessary look at the glyph anchor (if any).
     */
    ///@{
    Point GetStemUpSE(Doc *doc, int staffSize, bool isCueSize) override;
    Point GetStemDownNW(Doc *doc, int staffSize, bool isCueSize) override;
    int CalcStemLenInThirdUnits(Staff *staff, data_STEMDIRECTION stemDir) override;
    ///@}

    //----------//
    // Functors //
    //----------//

    /**
     * See Object::PrepareLayerElementParts
     */
    int PrepareLayerElementParts(FunctorParams *functorParams) override;

    /**
     * See Object::CalcStem
     */
    int CalcStem(FunctorParams *functorParams) override;

protected:
    //
private:
    //
public:
    //
private:
    //
};

} // namespace vrv

#endif
