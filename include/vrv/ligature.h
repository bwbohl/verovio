/////////////////////////////////////////////////////////////////////////////
// Name:        ligature.h
// Author:      Donald Byrd
// Created:     2016
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_LIGATURE_H__
#define __VRV_LIGATURE_H__

#include "atts_mensural.h"
#include "drawinginterface.h"
#include "layerelement.h"

namespace vrv {

//----------------------------------------------------------------------------
// Ligature
//----------------------------------------------------------------------------

/**
 * This class represents a collection of notes in the same layer with successive
 * onset times, as used in mensural notation.
 * A ligature is contained in a layer.
 * It contains notes.
 */

class Ligature : public LayerElement, public ObjectListInterface, public AttLigatureVis {
public:
    /**
     * @name Constructors, destructors, reset and class name methods
     * Reset method resets all attribute classes
     */
    ///@{
    Ligature();
    virtual ~Ligature();
    Object *Clone() const override { return new Ligature(*this); }
    void Reset() override;
    std::string GetClassName() const override { return "Ligature"; }
    ///@}

    /** Override the method since alignment is required */
    bool HasToBeAligned() const override { return true; }

    /**
     * Add children (notes or editorial markup)
     */
    bool IsSupportedChild(Object *object) override;

    /**
     * Return the first or last note
     */
    ///@{
    Note *GetFirstNote();
    Note *GetLastNote();
    ///@}

    /**
     * @name Return shape information about the note ligature
     */
    ///@{
    int GetDrawingNoteShape(Note *note);
    int GetDrawingPreviousNoteShape(Note *note);
    ///@}

    //----------//
    // Functors //
    //----------//

    /**
     * See Object::CalcLigatureNotePos
     */
    int CalcLigatureNotePos(FunctorParams *functorParams) override;

    /**
     * See Object::ResetDrawing
     */
    int ResetDrawing(FunctorParams *functorParams) override;

protected:
    /**
     * Clear the m_clusters vector and delete all the objects.
     */
    void ClearClusters();

    /**
     * Filter the flat list and keep only Note elements.
     */
    void FilterList(ArrayOfObjects *childList) override;

public:
    /**
     *
     */
    std::vector<int> m_drawingShapes;
};

} // namespace vrv

#endif
