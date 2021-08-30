/////////////////////////////////////////////////////////////////////////////
// Name:        iopae.cpp
// Author:      Rodolfo Zitellini // this is the important stuff!
// Created:     2012
// Copyright (c) Laurent Pugin. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "iopae.h"

//----------------------------------------------------------------------------

#include <assert.h>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>

#ifndef NO_PAE_SUPPORT
#include <regex>
#endif /* NO_PAE_SUPPORT */

//----------------------------------------------------------------------------

#include "beam.h"
#include "chord.h"
#include "clef.h"
#include "comparison.h"
#include "doc.h"
#include "dot.h"
#include "fermata.h"
#include "gracegrp.h"
#include "keyaccid.h"
#include "keysig.h"
#include "layer.h"
#include "mdiv.h"
#include "measure.h"
#include "metersig.h"
#include "mrest.h"
#include "multirest.h"
#include "note.h"
#include "rest.h"
#include "score.h"
#include "scoredef.h"
#include "section.h"
#include "space.h"
#include "staff.h"
#include "staffdef.h"
#include "staffgrp.h"
#include "tie.h"
#include "trill.h"
#include "tuplet.h"
#include "vrv.h"

//----------------------------------------------------------------------------

namespace vrv {

//----------------------------------------------------------------------------
// PAEOutput
//----------------------------------------------------------------------------

PAEOutput::PAEOutput(Doc *doc) : Output(doc) {}

PAEOutput::~PAEOutput() {}

bool PAEOutput::Export(std::string &output)
{
    m_docScoreDef = true;
    m_mensural = false;
    m_skip = false;
    m_layerN = -1;
    m_staffN = -1;
    m_currentOct = -1;
    m_currentDur = -1;
    m_currentDots = -1;
    m_grace = false;

    m_doc->GetCurrentScoreDef()->Save(this);

    m_docScoreDef = false;

    m_doc->Save(this);

    output = m_streamStringOutput.str();

    return true;
}

bool PAEOutput::WriteObject(Object *object)
{
    if (object->Is(MDIV)) {
        WriteMdiv(dynamic_cast<Mdiv *>(object));
    }
    if (object->Is(SCOREDEF)) {
        WriteScoreDef(dynamic_cast<ScoreDef *>(object));
    }
    else if (object->Is(STAFFDEF)) {
        WriteStaffDef(dynamic_cast<StaffDef *>(object));
    }
    else if (object->Is(MEASURE)) {
        WriteMeasure(dynamic_cast<Measure *>(object));
    }
    else if (object->Is(STAFF)) {
        WriteStaff(dynamic_cast<Staff *>(object));
    }
    else if (object->Is(LAYER)) {
        WriteLayer(dynamic_cast<Layer *>(object));
    }

    // Measure elements

    // Layer elements
    else if (object->Is(BARLINE)) {
        WriteBarLine(dynamic_cast<BarLine *>(object));
    }
    else if (object->Is(BEAM)) {
        WriteBeam(dynamic_cast<Beam *>(object));
    }
    else if (object->Is(CHORD)) {
        WriteChord(dynamic_cast<Chord *>(object));
    }
    else if (object->Is(CLEF)) {
        WriteClef(dynamic_cast<Clef *>(object));
    }
    else if (object->Is(GRACEGRP)) {
        WriteGraceGrp(dynamic_cast<GraceGrp *>(object));
    }
    else if (object->Is(KEYACCID)) {
        WriteKeyAccid(dynamic_cast<KeyAccid *>(object));
    }
    else if (object->Is(KEYSIG)) {
        WriteKeySig(dynamic_cast<KeySig *>(object));
    }
    else if (object->Is(MENSUR)) {
        WriteMensur(dynamic_cast<Mensur *>(object));
    }
    else if (object->Is(METERSIG)) {
        WriteMeterSig(dynamic_cast<MeterSig *>(object));
    }
    else if (object->Is(MREST)) {
        WriteMRest(dynamic_cast<MRest *>(object));
    }
    else if (object->Is(MULTIREST)) {
        WriteMultiRest(dynamic_cast<MultiRest *>(object));
    }
    else if (object->Is(NOTE)) {
        WriteNote(dynamic_cast<Note *>(object));
    }
    else if (object->Is(REST)) {
        WriteRest(dynamic_cast<Rest *>(object));
    }
    else if (object->Is(SPACE)) {
        WriteSpace(dynamic_cast<Space *>(object));
    }
    else if (object->Is(TUPLET)) {
        WriteTuplet(dynamic_cast<Tuplet *>(object));
    }
    else {
        // Log something?
    }

    return true;
}

bool PAEOutput::WriteObjectEnd(Object *object)
{
    if (object->Is(MEASURE)) {
        WriteMeasureEnd(dynamic_cast<Measure *>(object));
    }
    else if (object->Is(BEAM)) {
        WriteBeamEnd(dynamic_cast<Beam *>(object));
    }
    else if (object->Is(TUPLET)) {
        WriteTupletEnd(dynamic_cast<Tuplet *>(object));
    }

    return true;
}

void PAEOutput::WriteMdiv(Mdiv *mdiv)
{
    assert(mdiv);

    m_streamStringOutput << "@data:";
}

void PAEOutput::WriteScoreDef(ScoreDef *scoreDef) {}

void PAEOutput::WriteStaffDef(StaffDef *staffDef)
{
    assert(staffDef);

    if (m_staffN != -1) return;

    m_staffN = staffDef->GetN();

    if (staffDef->HasNotationtype() && staffDef->GetNotationtype() == NOTATIONTYPE_mensural) {
        m_mensural = true;
    }
}

void PAEOutput::WriteMeasure(Measure *measure)
{
    assert(measure);

    m_currentMeasure = measure;
}

void PAEOutput::WriteMeasureEnd(Measure *measure)
{
    assert(measure);

    std::string barLine;
    switch (measure->GetRight()) {
        case (BARRENDITION_rptend): barLine = "://"; break;
        case (BARRENDITION_rptboth): barLine = "://:"; break;
        case (BARRENDITION_rptstart): barLine = "//:"; break;
        case (BARRENDITION_dbl): barLine = "//"; break;
        case (BARRENDITION_invis): barLine = ""; break;
        default: barLine = "/";
    }

    m_streamStringOutput << barLine;
}

void PAEOutput::WriteStaff(Staff *staff)
{
    assert(staff);

    if (m_staffN == staff->GetN()) {
        m_skip = false;
        return;
    }

    m_skip = true;
}

void PAEOutput::WriteLayer(Layer *layer)
{
    assert(layer);

    if (m_layerN == -1) {
        m_layerN = layer->GetN();
        m_skip = false;
        return;
    }

    // second layer in the staff
    if (!m_skip && (m_layerN != layer->GetN())) {
        m_skip = true;
        return;
    }
}

void PAEOutput::WriteBarLine(BarLine *barLine)
{
    assert(barLine);

    if (m_skip) return;

    // We should look more precisely at its appearance ...
    m_streamStringOutput << "/";
}

void PAEOutput::WriteBeam(Beam *beam)
{
    assert(beam);

    if (m_skip) return;

    m_grace = false;

    ClassIdsComparison matchType({ NOTE, CHORD });
    ArrayOfObjects children;
    LayerElement *child = dynamic_cast<LayerElement *>(beam->FindDescendantByComparison(&matchType));
    if (child && child->IsGraceNote()) {
        m_streamStringOutput << "qq";
        m_grace = true;
    }

    m_streamStringOutput << "{";
}

void PAEOutput::WriteBeamEnd(Beam *beam)
{
    assert(beam);

    if (m_skip) return;

    m_streamStringOutput << "}";

    if (m_grace) {
        m_streamStringOutput << "r";
        m_grace = false;
    }
}

void PAEOutput::WriteChord(Chord *chord)
{
    assert(chord);

    if (m_skip) return;

    std::string oct;

    WriteDur(chord);
    WriteGrace(chord);
}

void PAEOutput::WriteClef(Clef *clef)
{
    assert(clef);

    if (m_skip) return;

    std::string outStart = (m_docScoreDef) ? "@clef:" : " %";
    std::string outEnd = (m_docScoreDef) ? "\n" : " ";

    std::string shape;
    if ((clef->GetShape() == CLEFSHAPE_G) && (clef->GetDis() == OCTAVE_DIS_8)) {
        shape = "g";
    }
    else {
        switch (clef->GetShape()) {
            case (CLEFSHAPE_C): shape = "C"; break;
            case (CLEFSHAPE_F): shape = "F"; break;
            case (CLEFSHAPE_G): shape = "G"; break;
            default: shape = "G";
        }
    }
    std::string sign = (m_mensural) ? "+" : "-";
    std::string line = StringFormat("%d", clef->GetLine());

    m_streamStringOutput << outStart << shape << sign << line << outEnd;
}

void PAEOutput::WriteGraceGrp(GraceGrp *graceGrp) {}

void PAEOutput::WriteKeyAccid(KeyAccid *keyAccid)
{
    // To do for cases when key signature have []
}

void PAEOutput::WriteKeySig(KeySig *keySig)
{
    assert(keySig);

    if (m_skip) return;

    std::string outStart = (m_docScoreDef) ? "@keysig:" : " $";
    std::string outEnd = (m_docScoreDef) ? "\n" : " ";

    data_ACCIDENTAL_WRITTEN accidType = keySig->GetSig().second;
    std::string sig;
    sig.push_back((accidType == ACCIDENTAL_WRITTEN_s) ? 'x' : 'b');
    for (int i = 0; i < keySig->GetSig().first; ++i) {
        data_PITCHNAME pname = KeySig::GetAccidPnameAt(accidType, i);
        std::string pnameStr = keySig->AttTyped::PitchnameToStr(pname);
        sig.push_back(std::toupper(pnameStr.at(0)));
    }

    m_streamStringOutput << outStart << sig << outEnd;
}

void PAEOutput::WriteMensur(Mensur *mensur)
{
    assert(mensur);

    if (m_skip) return;

    std::string outStart = (m_docScoreDef) ? "@timesig:" : " @";
    std::string outEnd = (m_docScoreDef) ? "\n" : " ";

    m_streamStringOutput << outStart << outEnd;
}

void PAEOutput::WriteMeterSig(MeterSig *meterSig)
{
    assert(meterSig);

    if (m_skip) return;

    std::string outStart = (m_docScoreDef) ? "@timesig:" : " @";
    std::string outEnd = (m_docScoreDef) ? "\n" : " ";

    std::string sig;
    if (meterSig->HasSym()) {
        sig = (meterSig->GetSym() == METERSIGN_common) ? "c" : "c/";
    }
    else if (meterSig->GetForm() == METERFORM_num) {
        sig = StringFormat("%d", meterSig->GetTotalCount());
    }
    else if (meterSig->HasCount() && meterSig->GetUnit()) {
        sig = StringFormat("%d/%d", meterSig->GetTotalCount(), meterSig->GetUnit());
    }

    m_streamStringOutput << outStart << sig << outEnd;
}

void PAEOutput::WriteMRest(MRest *mRest)
{
    assert(mRest);

    if (m_skip) return;

    m_streamStringOutput << "=";
}

void PAEOutput::WriteMultiRest(MultiRest *multiRest)
{
    assert(multiRest);

    if (m_skip) return;

    m_streamStringOutput << "=" << multiRest->GetNum();
}

void PAEOutput::WriteNote(Note *note)
{
    assert(note);
    assert(m_currentMeasure);

    if (m_skip) return;

    std::string oct;

    // For chords, only output the top note
    Chord *chord = note->IsChordTone();
    if (chord) {
        if (note != chord->GetTopNote()) return;
    }
    else {
        WriteDur(note);
        WriteGrace(note);
    }

    if (note->GetOct() != m_currentOct) {
        m_currentOct = note->GetOct();
        char octSign = (m_currentOct > 3) ? '\'' : ',';
        int signCount = (m_currentOct > 3) ? (m_currentOct - 3) : (4 - m_currentOct);
        m_streamStringOutput << std::string(signCount, octSign);
    }

    Accid *noteAccid = dynamic_cast<Accid *>(note->FindDescendantByType(ACCID));
    if (noteAccid) {
        std::string accid;
        switch (noteAccid->GetAccid()) {
            case (ACCIDENTAL_WRITTEN_s): accid = "x"; break;
            case (ACCIDENTAL_WRITTEN_f): accid = "b"; break;
            case (ACCIDENTAL_WRITTEN_ss): accid = "xx"; break;
            case (ACCIDENTAL_WRITTEN_x): accid = "xx"; break;
            case (ACCIDENTAL_WRITTEN_ff): accid = "bb"; break;
            case (ACCIDENTAL_WRITTEN_n): accid = "n"; break;
            default: accid = "";
        }
        m_streamStringOutput << accid;
    }

    PointingToComparison pointingToComparisonFermata(FERMATA, note);
    Fermata *fermata
        = dynamic_cast<Fermata *>(m_currentMeasure->FindDescendantByComparison(&pointingToComparisonFermata, 1));
    if (fermata) m_streamStringOutput << "(";

    std::string pname = note->AttPitch::PitchnameToStr(note->GetPname());
    std::transform(pname.begin(), pname.end(), pname.begin(), ::toupper);
    m_streamStringOutput << pname;

    if (fermata) m_streamStringOutput << ")";

    PointingToComparison pointingToComparisonTrill(TRILL, note);
    Trill *trill = dynamic_cast<Trill *>(m_currentMeasure->FindDescendantByComparison(&pointingToComparisonTrill, 1));
    if (trill) m_streamStringOutput << "t";

    PointingToComparison pointingToComparisonTie(TIE, note);
    Tie *tie = dynamic_cast<Tie *>(m_currentMeasure->FindDescendantByComparison(&pointingToComparisonTie, 1));
    if (tie) m_streamStringOutput << "+";
}

void PAEOutput::WriteRest(Rest *rest)
{
    assert(rest);

    if (m_skip) return;

    WriteDur(rest);
    m_streamStringOutput << "-";
}

void PAEOutput::WriteSpace(Space *space)
{
    assert(space);

    if (m_skip) return;

    WriteDur(space);
    // Simply make it a rest - no other option in PAE
    m_streamStringOutput << "-";
}

void PAEOutput::WriteTuplet(Tuplet *tuplet)
{
    assert(tuplet);

    Staff *staff = vrv_cast<Staff *>(tuplet->GetFirstAncestor(STAFF));
    assert(staff);

    double content = tuplet->GetContentAlignmentDuration(NULL, NULL, true, staff->m_drawingNotationType);
    // content = DUR_MAX / 2^(dur - 2)
    int tupletDur = (content != 0.0) ? log2(DUR_MAX / content) + 2 : 4;
    // We should be looking for dotted values

    std::string dur;
    switch (tupletDur) {
        case (DUR_LG): dur = "0"; break;
        case (DUR_BR): dur = "9"; break;
        case (DUR_1): dur = "1"; break;
        case (DUR_2): dur = "2"; break;
        case (DUR_4): dur = "4"; break;
        case (DUR_8): dur = "8"; break;
        case (DUR_16): dur = "6"; break;
        case (DUR_32): dur = "3"; break;
        case (DUR_64): dur = "5"; break;
        case (DUR_128): dur = "7"; break;
        default: LogWarning("Unsupported tuplet duration"); dur = "4";
    }

    // For duration to be written within the tuplet
    m_currentDur = -1;
    m_streamStringOutput << dur << "(";
}

void PAEOutput::WriteTupletEnd(Tuplet *tuplet)
{
    assert(tuplet);

    m_streamStringOutput << ";" << tuplet->GetNum() << ")";
}

void PAEOutput::WriteDur(DurationInterface *interface)
{
    assert(interface);

    if ((interface->GetDur() != m_currentDur) || (interface->GetDots() != m_currentDots)) {
        m_currentDur = interface->GetDur();
        m_currentDots = (interface->HasDots()) ? interface->GetDots() : 0;
        std::string dur;
        switch (m_currentDur) {
            case (DURATION_long): dur = "0"; break;
            case (DURATION_breve): dur = "9"; break;
            case (DURATION_1): dur = "1"; break;
            case (DURATION_2): dur = "2"; break;
            case (DURATION_4): dur = "4"; break;
            case (DURATION_8): dur = "8"; break;
            case (DURATION_16): dur = "6"; break;
            case (DURATION_32): dur = "3"; break;
            case (DURATION_64): dur = "5"; break;
            case (DURATION_128): dur = "7"; break;
            case (DURATION_maxima): dur = "0"; break;
            case (DURATION_longa): dur = "0"; break;
            case (DURATION_brevis): dur = "9"; break;
            case (DURATION_semibrevis): dur = "1"; break;
            case (DURATION_minima): dur = "2"; break;
            case (DURATION_semiminima): dur = "4"; break;
            case (DURATION_fusa): dur = "8"; break;
            case (DURATION_semifusa): dur = "6"; break;
            default: LogWarning("Unsupported duration"); dur = "4";
        }
        m_streamStringOutput << dur;
        m_streamStringOutput << std::string(m_currentDots, '.');
    }
}

void PAEOutput::WriteGrace(AttGraced *attGraced)
{
    assert(attGraced);

    // We are in a beam of grace notes;
    if (m_grace) return;

    if (attGraced->GetGrace() == GRACE_unacc) {
        m_streamStringOutput << "g";
    }
    else if (attGraced->HasGrace()) {
        m_streamStringOutput << "q";
    }
}

#ifndef NO_PAE_SUPPORT

#define BEAM_INITIAL 0x01
#define BEAM_MEDIAL 0x02
#define BEAM_TUPLET 0x03
#define BEAM_TERMINAL 0x04

// User interface variables:
int debugQ = 0; // used with --debug option
int stdoutQ = 0;
char outdir[1024] = { 0 }; // used with -d option
char extension[1024] = { 0 }; // used with -e option
char hum2abc[1024] = { 0 }; // used with -a option
int quietQ = 0; // used with -q option
int quiet2Q = 0; // used with -Q option

// Global variables:
char data_line[10001] = { 0 };
#define MAX_DATA_LEN 1024 // One line of the pae file would not be that long!
char data_key[MAX_DATA_LEN];
char data_value[MAX_DATA_LEN]; // ditto as above

#endif /* NO_PAE_SUPPORT */

//----------------------------------------------------------------------------
// PAEInput
//----------------------------------------------------------------------------

PAEInput::PAEInput(Doc *doc)
    : // This is pretty bad. We open a bad fileoinputstream as we don't use it
    Input(doc)
{
    m_staff = NULL;
    m_measure = NULL;
    m_layer = NULL;
    m_tie = NULL;
    m_is_in_chord = false;
    m_is_mensural = false;

    m_currentKeySig = NULL;
    m_tieAccid.first = PITCHNAME_NONE;
    m_tieAccid.second = ACCIDENTAL_WRITTEN_NONE;
}

PAEInput::~PAEInput() {}

#ifndef NO_PAE_SUPPORT

//////////////////////////////////////////////////////////////////////////

bool PAEInput::Import(const std::string &pae)
{
    std::istringstream in_stream(pae);
    parsePlainAndEasy(in_stream);
    return true;
}

//////////////////////////////
//
// parsePlainAndEasy --
//

void PAEInput::parsePlainAndEasy(std::istream &infile)
{
    // buffers
    char c_clef[1024] = { 0 };
    char c_key[1024] = { 0 };
    char c_keysig[1024] = { 0 };
    char c_timesig[1024] = { 0 };
    char c_alttimesig[1024] = { 0 };
    char incipit[10001] = { 0 };
    int in_beam = 0;

    std::string s_key;
    pae::Measure current_measure;
    pae::Note current_note;
    Clef *staffDefClef = NULL;
    MeterSig *scoreDefMeterSig = NULL;
    Mensur *scoreDefMensur = NULL;
    KeySig *scoreDefKeySig = NULL;

    std::vector<pae::Measure> staff;

    m_doc->Reset();
    m_doc->SetType(Raw);
    // The mdiv
    Mdiv *mdiv = new Mdiv();
    mdiv->m_visibility = Visible;
    m_doc->AddChild(mdiv);
    // The score
    Score *score = new Score();
    mdiv->AddChild(score);
    // the section
    Section *section = new Section();
    score->AddChild(section);

    // add minimal scoreDef
    StaffGrp *staffGrp = new StaffGrp();
    StaffDef *staffDef = new StaffDef();
    staffDef->SetN(1);
    staffDef->SetLines(5);
    staffGrp->AddChild(staffDef);

    // read values
    while (!infile.eof()) {
        infile.getline(data_line, 10000);
        if (infile.eof()) {
            // LogDebug("Truncated file or ending tag missing");
            // exit(1);
        }
        getAtRecordKeyValue(data_key, data_value, data_line);
        if (strcmp(data_key, "end") == 0) {
            break;
        }
        else if (strcmp(data_key, "clef") == 0) {
            strcpy(c_clef, data_value);
        }
        else if (strcmp(data_key, "key") == 0) {
            strcpy(c_key, data_value);
        }
        else if (strcmp(data_key, "keysig") == 0) {
            strcpy(c_keysig, data_value);
        }
        else if (strcmp(data_key, "timesig") == 0) {
            strcpy(c_timesig, data_value);
        }
        else if (strcmp(data_key, "alttimesig") == 0) {
            strcpy(c_alttimesig, data_value);
        }
        else if (strcmp(data_key, "data") == 0) {
            strcpy(incipit, data_value);
        }
        else {
            LogWarning("Unknown row '%s' in incipit data", data_line);
        }
    }

    if (strlen(c_clef)) {
        Clef *c = new Clef;
        getClefInfo(c_clef, c); // do we need to put a default clef?
        if (!staffDefClef)
            staffDefClef = c;
        else
            current_measure.clef = c;
    }

    if (strlen(c_keysig)) {
        KeySig *k = new KeySig();
        // Make it an attribute for now
        k->IsAttribute(true);
        getKeyInfo(c_keysig, k);
        if (!scoreDefKeySig) {
            scoreDefKeySig = k;
        }
        else {
            if (current_measure.key) {
                delete current_measure.key;
            }
            current_measure.key = k;
        }
    }
    if (strlen(c_timesig)) {
        if (m_is_mensural) {
            Mensur *mensur = new Mensur();
            getTimeInfo(c_timesig, NULL, mensur);
            if (!scoreDefMensur) {
                scoreDefMensur = mensur;
            }
            else {
                if (current_measure.mensur) {
                    delete current_measure.mensur;
                }
                current_measure.mensur = mensur;
            }
        }
        else {
            MeterSig *meter = new MeterSig;
            getTimeInfo(c_timesig, meter, NULL);
            if (!scoreDefMeterSig) {
                scoreDefMeterSig = meter;
            }
            else {
                if (current_measure.meter) {
                    delete current_measure.meter;
                }
                current_measure.meter = meter;
            }
        }
    }

    // read the incipit string
    int length = (int)strlen(incipit);
    int i = 0;
    while (i < length) {
        // eat the input...

        if (incipit[i] == ' ') {
            // just skip
            i++;
        }

        // octaves
        if ((incipit[i] == '\'') || (incipit[i] == ',')) {
            i += getOctave(incipit, &current_note.octave, i);
        }

        // rhythmic values
        else if ((isdigit(incipit[i]) != 0) && ((i <= 0) || incipit[i - 1] != ';')) {
            i += getDurations(incipit, &current_measure, i);
        }

        // accidentals (1 = n; 2 = x; 3 = xx; 4 = b; 5 = bb)
        else if (incipit[i] == 'n' || incipit[i] == 'x' || incipit[i] == 'b') {
            i += getAccidental(incipit, &current_note.accidental, i);
        }

        //
        // beaming starts
        else if (incipit[i] == '{') {
            // current_note.beam = 1;
            if (current_note.tuplet_note > 0)
                current_note.beam = BEAM_TUPLET;
            else
                current_note.beam = BEAM_INITIAL;
            in_beam++;
        }

        // beaming ends
        else if (incipit[i] == '}' && in_beam > 0) {
            if (!current_measure.notes.empty()) {
                current_measure.notes.at(current_measure.notes.size() - 1).beam = BEAM_TERMINAL;
            }
            current_note.beam = 0;
            in_beam--;
        }

        // ties are read when adding the note
        else if (incipit[i] == '+') {
        }

        // beginning tuplets & fermatas
        else if (incipit[i] == '(') {
            i += getTupletFermata(incipit, &current_note, i);
        }

        // end of tuplets
        else if ((incipit[i] == ';') || (incipit[i] == ')')) {
            i += getTupletFermataEnd(incipit, &current_note, i);
        }

        // trills are read when adding the note
        else if (incipit[i] == 't') {
        }

        // grace notes
        else if ((incipit[i] == 'g') || (incipit[i] == 'q')) {
            i += getGraceNote(incipit, &current_note, i);
        }

        // end of appogiatura
        else if (incipit[i] == 'r') {
            current_note.appoggiatura = 0; // should not have to be done, but just in case
        }

        // note and rest
        // getNote also creates a new note object
        else if (((incipit[i] - 'A' >= 0) && (incipit[i] - 'A' < 7)) || (incipit[i] == '-')) {
            i += getNote(incipit, &current_note, &current_measure, i);
        }

        // whole rest
        else if (incipit[i] == '=') {
            i += getWholeRest(incipit, &current_measure.wholerest, i);
        }

        // abbreviation
        else if (incipit[i] == '!') {
            i += getAbbreviation(incipit, &current_measure, i);
        }

        // measure repetition
        else if ((incipit[i] == 'i') && staff.size() > 0) {
            pae::Measure last_measure = staff[staff.size() - 1];
            current_measure.notes = last_measure.notes;
            current_measure.wholerest = last_measure.wholerest;
        }

        // barLine
        else if ((incipit[i] == ':') || (incipit[i] == '/')) {
            i += getBarLine(incipit, &current_measure.barLine, i);
            current_measure.abbreviation_offset = 0; // just in case...
            staff.push_back(current_measure);
            current_measure.reset();
            if (m_currentKeySig) m_currentKeySig->FillMap(m_currentAccids);
        }

        // clef change
        else if ((incipit[i] == '%') && (i + 1 < length)) {
            Clef *c = new Clef;
            i += getClefInfo(incipit, c, i + 1);
            //
            if (!staffDefClef) {
                staffDefClef = c;
            }
            // If there are no notes yet in the measure
            // attach this clef change to the measure
            else if (current_measure.notes.size() == 0) {
                // If a clef was already assigned, remove it
                if (current_measure.clef) delete current_measure.clef;

                current_measure.clef = c;
            }
            else {
                // as above
                if (current_note.clef) delete current_note.clef;

                current_note.clef = c;
            }
        }

        // time signature change
        else if ((incipit[i] == '@') && (i + 1 < length)) {
            if (m_is_mensural) {
                Mensur *mensur = new Mensur();
                i += getTimeInfo(incipit, NULL, mensur, i + 1);
                if (current_note.mensur) {
                    delete current_note.mensur;
                }
                current_note.mensur = mensur;
            }
            else {
                MeterSig *meter = new MeterSig;
                i += getTimeInfo(incipit, meter, NULL, i + 1);
                if (current_measure.notes.size() == 0) {
                    if (current_measure.meter) {
                        delete current_measure.meter;
                    }
                    // When will this be deleted? Potential memory leak? LP
                    current_measure.meter = meter;
                }
                else {
                    if (current_note.meter) {
                        delete current_note.meter;
                    }
                    current_note.meter = meter;
                }
            }
        }

        // key signature change
        else if ((incipit[i] == '$') && (i + 1 < length)) {
            KeySig *k = new KeySig();
            // Make it an attribute for now
            k->IsAttribute(true);
            i += getKeyInfo(incipit, k, i + 1);
            if (current_measure.notes.size() == 0) {
                if (current_measure.key) delete current_measure.key;
                current_measure.key = k;
            }
            else {
                if (current_note.key) delete current_note.key;
                current_note.key = k;
            }
        }

        i++;
    }

    // we need to add the last measure if it has no barLine at the end
    if (current_measure.notes.size() != 0) {
        // current_measure.barLine = "=-";
        staff.push_back(current_measure);
        current_measure.notes.clear();
    }

    int measure_count = 1;

    std::vector<pae::Measure>::iterator it;
    for (it = staff.begin(); it < staff.end(); ++it) {

        m_staff = new Staff(1);
        m_measure = new Measure(true, measure_count);
        m_layer = new Layer();
        m_layer->SetN(1);

        m_staff->AddChild(m_layer);
        m_measure->AddChild(m_staff);

        pae::Measure obj = *it;

        // Add a score def if we have a new key sig or meter sig
        if (obj.key || obj.meter) {
            ScoreDef *scoreDef = new ScoreDef();
            if (obj.key) {
                scoreDef->AddChild(obj.key);
                obj.key = NULL;
            }
            if (obj.meter) {
                obj.meter->IsAttribute(true);
                scoreDef->AddChild(obj.meter);
                obj.meter = NULL;
            }
            section->AddChild(scoreDef);
        }

        section->AddChild(m_measure);

        convertMeasure(&obj);
        measure_count++;
    }

    if (m_is_mensural) {
        staffDef->SetNotationtype(NOTATIONTYPE_mensural);
    }
    if (staffDefClef) {
        // Make it an attribute for now
        staffDefClef->IsAttribute(true);
        staffDef->AddChild(staffDefClef);
    }
    if (scoreDefKeySig) {
        m_doc->GetCurrentScoreDef()->AddChild(scoreDefKeySig);
    }
    if (scoreDefMeterSig) {
        // Make it an attribute for now
        scoreDefMeterSig->IsAttribute(true);
        m_doc->GetCurrentScoreDef()->AddChild(scoreDefMeterSig);
    }
    if (scoreDefMensur) {
        // Make it an attribute for now
        scoreDefMensur->IsAttribute(true);
        m_doc->GetCurrentScoreDef()->AddChild(scoreDefMensur);
    }

    m_doc->GetCurrentScoreDef()->AddChild(staffGrp);

    if (m_tie != NULL) {
        LogWarning("Open tie will not render because tstamp2 is missing");
    }

    m_doc->ConvertToPageBasedDoc();
}

//////////////////////////////
//
// getOctave --
//
#define BASE_OCT 4
int PAEInput::getOctave(const char *incipit, char *octave, int index)
{
    int i = index;
    int length = (int)strlen(incipit);
    if (incipit[i] == '\'') {
        *octave = BASE_OCT;
        while ((i + 1 < length) && (incipit[i + 1] == '\'')) {
            (*octave)++;
            i++;
        }
    }
    else if (incipit[i] == ',') {
        // negative octave
        *octave = BASE_OCT - 1;
        while ((i + 1 < length) && (incipit[i + 1] == ',')) {
            (*octave)--;
            i++;
        }
    }

    return i - index;
}

//////////////////////////////
//
// getDuration --
//

int PAEInput::getDuration(const char *incipit, data_DURATION *duration, int *dot, int index)
{

    int i = index;
    int length = (int)strlen(incipit);

    if (m_is_mensural) {
        switch (incipit[i]) {
            case '0': *duration = DURATION_longa; break;
            case '1': *duration = DURATION_semibrevis; break;
            case '2': *duration = DURATION_minima; break;
            case '3':
                *duration = DURATION_breve;
                LogWarning("Duration 3 unsupported with mensural notation, using breve instead");
                break;
            case '4': *duration = DURATION_semiminima; break;
            case '5':
                *duration = DURATION_breve;
                LogWarning("Duration 5 unsupported with mensural notation, using breve instead");
                break;
            case '6': *duration = DURATION_semifusa; break;
            case '7': *duration = DURATION_breve; break;
            case '8': *duration = DURATION_fusa; break;
            case '9': *duration = DURATION_brevis; break;
        }
    }
    else {
        switch (incipit[i]) {
            case '0': *duration = DURATION_long; break;
            case '1': *duration = DURATION_1; break;
            case '2': *duration = DURATION_2; break;
            case '3': *duration = DURATION_32; break;
            case '4': *duration = DURATION_4; break;
            case '5': *duration = DURATION_64; break;
            case '6': *duration = DURATION_16; break;
            case '7': *duration = DURATION_128; break;
            case '8': *duration = DURATION_8; break;
            case '9': *duration = DURATION_breve; break;
        }
    }

    *dot = 0;
    while ((i + 1 < length) && (incipit[i + 1] == '.')) {
        (*dot)++;
        i++;
    }
    if ((*dot == 1) && (incipit[i] == 7)) {
        // neumatic notation
        *duration = DURATION_breve;
        *dot = 0;
        LogWarning("Plaine & Easie import: neumatic notation unsupported, using breve instead");
    }

    return i - index;
}

//////////////////////////////
//
// getDurations --
//

int PAEInput::getDurations(const char *incipit, pae::Measure *measure, int index)
{
    int i = index;
    int length = (int)strlen(incipit);

    measure->durations_offset = 0;
    measure->durations.clear();
    measure->dots.clear();

    // int j = 0;
    do {
        int dot;
        data_DURATION dur = DURATION_4;
        // measure->dots.setSize(j+1);
        i += getDuration(incipit, &dur, &dot, i);
        measure->durations.push_back(dur);
        measure->dots.push_back(dot);
        // j++;
        if ((i + 1 < length) && isdigit(incipit[i + 1])) {
            i++;
        }
        else {
            break;
        }
    } while (1);

    return i - index;
}

//////////////////////////////
//
// getAccidental --
//

int PAEInput::getAccidental(const char *incipit, data_ACCIDENTAL_WRITTEN *accident, int index)
{
    int i = index;
    int length = (int)strlen(incipit);

    if (incipit[i] == 'n') {
        *accident = ACCIDENTAL_WRITTEN_n;
    }
    else if (incipit[i] == 'x') {
        *accident = ACCIDENTAL_WRITTEN_s;
        if ((i + 1 < length) && (incipit[i + 1] == 'x')) {
            *accident = ACCIDENTAL_WRITTEN_x;
            i++;
        }
    }
    else if (incipit[i] == 'b') {
        *accident = ACCIDENTAL_WRITTEN_f;
        if ((i + 1 < length) && (incipit[i + 1] == 'b')) {
            *accident = ACCIDENTAL_WRITTEN_ff;
            i++;
        }
    }
    return i - index;
}

//////////////////////////////
//
// getTupletOrFermata --
//

int PAEInput::getTupletFermata(const char *incipit, pae::Note *note, int index)
{
    int i = index;
    int length = (int)strlen(incipit);

    // Detect if it is a fermata or a tuplet.
    //
    // std::regex_constants::ECMAScript is the default syntax, so optional.
    // Previously these were extended regex syntax, but this case
    // is the same in ECMAScript syntax.
    std::regex exp("^([^)]*[ABCDEFG-][^)]*[ABCDEFG-][^)]*)", std::regex_constants::ECMAScript);
    bool is_tuplet = regex_search(incipit + i, exp);

    if (is_tuplet) {
        int t = i;
        int tuplet_val = 3; // triplets are default
        int tuplet_notes = 0;
        char *buf;

        // create the buffer so we can convert the tuplet nr to int
        buf = (char *)malloc(length + 1); // allocate it with space for 0x00
        memset(buf, 0x00, length + 1); // wipe it up

        // move until we find the ;
        while ((t < length) && (incipit[t] != ';')) {

            // Triplets are sometimes codes without ';' values
            if (incipit[t] == ')') {
                break;
            }

            // count the notes
            if ((incipit[t] - 'A' >= 0) && (incipit[t] - 'A' < 7)) {
                tuplet_notes++;
            }
            // count the rests
            if (incipit[t] == '-') {
                tuplet_notes++;
            }

            t++;
        }

        // We detected a ';', get the value of the tuplet (otherwise 3 by default)
        if ((t < length) && incipit[t] != ')') {
            int t2 = 0;
            // t + 1 should point to the number
            t++; // move one char to the number
            while (((t + t2) < length) && (incipit[t + t2] != ')')) {

                // If we have extraneous chars, exit here
                if (!isdigit(incipit[t + t2])) {
                    LogDebug("Plaine & Easie import: non-number in tuplet number found");
                    free(buf);
                    return i - index;
                }

                // copy the number char-by-char
                buf[t2] = incipit[t + t2];
                t2++;
            }

            tuplet_val = atoi(buf);
        }

        // this is the first note, the total number of notes = tuplet_val
        note->tuplet_notes = tuplet_notes;
        // but also the note counter
        note->tuplet_note = tuplet_notes;
        // the tuplet val (3 or after ;)
        note->tuplet_val = tuplet_val;
        free(buf); // dispose of the buffer
    }
    else {
        if (note->tuplet_notes > 0) {
            LogWarning("Plaine & Easie import: fermatas within tuplets won't be handled correctly");
        }
        note->fermata = true;
    }

    return i - index;
}

//////////////////////////////
//
// getTupletFermataEnd --
//
// this can be deleted in the future?
int PAEInput::getTupletFermataEnd(const char *incipit, pae::Note *note, int index)
{
    int i = index;
    // int length = strlen(incipit);

    // TODO: fermatas inside tuplets won't be currently handled correctly
    if (note->tuplet_notes != 1) {
        note->fermata = false;
    }

    return i - index;
}

//////////////////////////////
//
// getGraceNote --
//

int PAEInput::getGraceNote(const char *incipit, pae::Note *note, int index)
{
    int i = index;
    int length = (int)strlen(incipit);

    // acciaccatura
    if (incipit[i] == 'g') {
        note->acciaccatura = true;
    }

    // appoggiatura
    else if (incipit[i] == 'q') {
        note->appoggiatura = 1;
        if ((i + 1 < length) && (incipit[i + 1] == 'q')) {
            i++;
            int r = i;
            while ((r < length) && (incipit[r] != 'r')) {
                if ((incipit[r] - 'A' >= 0) && (incipit[r] - 'A' < 7)) {
                    note->appoggiatura++;
                }
                r++;
            }
        }
    }
    return i - index;
}

//////////////////////////////
//
// getPitch --
//

data_PITCHNAME PAEInput::getPitch(char c_note)
{
    data_PITCHNAME pitch = PITCHNAME_c;

    switch (c_note) {
        case 'A': pitch = PITCHNAME_a; break;
        case 'B': pitch = PITCHNAME_b; break;
        case 'C': pitch = PITCHNAME_c; break;
        case 'D': pitch = PITCHNAME_d; break;
        case 'E': pitch = PITCHNAME_e; break;
        case 'F': pitch = PITCHNAME_f; break;
        case 'G': pitch = PITCHNAME_g; break;
        case '-': pitch = PITCHNAME_NONE; break;
        default: break;
    }
    return pitch;
}

//////////////////////////////
//
// getTimeInfo -- read the time signature.
//

int PAEInput::getTimeInfo(const char *incipit, MeterSig *meter, Mensur *mensur, int index)
{
    int i = index;
    int length = (int)strlen(incipit);

    if (!meter && !mensur) {
        return 0;
    }

    if (!isdigit(incipit[i]) && (incipit[i] != 'c') && (incipit[i] != 'o')) {
        return 0;
    }

    // find the end of time signature
    i++; // the time signature length is a least 1
    while (i < length) {
        if (!isdigit(incipit[i]) && (incipit[i] != '/') && (incipit[i] != '.')) {
            break;
        }
        i++;
    }

    // use a substring for the time signature
    char timesig_str[1024];
    memset(timesig_str, 0, 1024);
    // strncpy not always put the \0 in the end!
    strncpy(timesig_str, incipit + index, i - index);

    std::ostringstream sout;

    // regex_match matches to the entire input string (regex_search does
    // partial matches.  In this case cmatch is used to store the submatches
    // (enclosed in parentheses) for later reference.  Use std::smatch when
    // dealing with strings, or std::wmatch with wstrings.
    std::cmatch matches;
    if (meter) {
        if (regex_match(timesig_str, matches, std::regex("(\\d+)/(\\d+)"))) {
            meter->SetCount({ std::stoi(matches[1]) });
            meter->SetUnit(std::stoi(matches[2]));
        }
        else if (regex_match(timesig_str, matches, std::regex("\\d+"))) {
            meter->SetCount({ std::stoi(timesig_str) });
            meter->SetUnit(1);
            meter->SetForm(METERFORM_num);
        }
        else if (strcmp(timesig_str, "c") == 0) {
            // C
            meter->SetSym(METERSIGN_common);
        }
        else if (strcmp(timesig_str, "c/") == 0) {
            // C|
            meter->SetSym(METERSIGN_cut);
        }
        else if (strcmp(timesig_str, "c3") == 0) {
            // C3
            meter->SetSym(METERSIGN_common);
            meter->SetCount({ 3 });
        }
        else if (strcmp(timesig_str, "c3/2") == 0) {
            // C3/2
            meter->SetSym(METERSIGN_common); // ??
            meter->SetCount({ 3 });
            meter->SetUnit(2);
        }
        else {
            LogWarning("Plaine & Easie import: unsupported time signature: %s", timesig_str);
        }
    }
    else {
        if (regex_match(timesig_str, matches, std::regex("(\\d+)/(\\d+)"))) {
            mensur->SetNum(std::stoi(matches[1]));
            mensur->SetNumbase(std::stoi(matches[2]));
        }
        else if (regex_match(timesig_str, matches, std::regex("\\d+"))) {
            mensur->SetNum(std::stoi(timesig_str));
        }
        else if (regex_match(timesig_str, matches, std::regex("([co])([\\./]?)([\\./]?)(\\d*)/?(\\d*)"))) {
            // C
            if (matches[1] == "c") {
                mensur->SetSign(MENSURATIONSIGN_C);
            }
            // O
            else {
                mensur->SetSign(MENSURATIONSIGN_O);
            }
            // Dot (second or third match since order between . and / is not defined in PAE)
            if ((matches[2] == ".") || (matches[3] == ".")) {
                mensur->SetDot(BOOLEAN_true);
            }
            // Slash (second or third match, ditto)
            if ((matches[2] == "/") || (matches[3] == "/")) {
                mensur->SetSlash(1);
            }
            // Num
            if (matches[4] != "") {
                mensur->SetNum(std::stoi(matches[4]));
            }
            // Numbase (but only if Num is given)
            if ((matches[4] != "") && (matches[5] != "")) {
                mensur->SetNumbase(std::stoi(matches[5]));
            }
        }
        else {
            LogWarning("Plaine & Easie import: unsupported time signature: %s", timesig_str);
        }
    }

    return i - index;
}

//////////////////////////////
//
// getClefInfo -- read the clef.
//

int PAEInput::getClefInfo(const char *incipit, Clef *mclef, int index)
{
    // a clef is maximum 3 character length
    // go through the 3 character and retrieve the letter (clef) and the line
    int length = (int)strlen(incipit);
    int i = 0;
    char clef = 'G';
    char line = '2';
    while ((index < length) && (i < 3)) {
        if (i == 0) {
            clef = incipit[index];
        }
        else if (i == 2) {
            line = incipit[index];
        }
        if (incipit[index] == '+') {
            m_is_mensural = true;
        }
        i++;
        index++;
    }

    if (clef == 'G') {
        mclef->SetShape(CLEFSHAPE_G);
        mclef->SetLine(line - 48);
    }
    else if (clef == 'C') {
        mclef->SetShape(CLEFSHAPE_C);
        mclef->SetLine(line - 48);
    }
    else if (clef == 'F') {
        mclef->SetShape(CLEFSHAPE_F);
        mclef->SetLine(line - 48);
    }
    else if (clef == 'g') {
        mclef->SetShape(CLEFSHAPE_G);
        mclef->SetLine(line - 48);
        mclef->SetDis(OCTAVE_DIS_8);
        mclef->SetDisPlace(STAFFREL_basic_below);
    }
    else {
        LogDebug("Plaine & Easie import: undefined clef %c", clef);
    }

    // measure->clef = mclef;

    return i;
}

//////////////////////////////
//
// getWholeRest -- read the getWholeRest.
//

int PAEInput::getWholeRest(const char *incipit, int *wholerest, int index)
{
    int length = (int)strlen(incipit);
    int i = index;

    *wholerest = 1;
    if ((i + 1 < length) && isdigit(incipit[i + 1])) {
        sscanf(&(incipit[i + 1]), "%d", wholerest);
        char buf[10];
        memset(buf, 0, 10);
        sprintf(buf, "%d", *wholerest);
        i += strlen(buf);
    }
    return i - index;
}

/**********************************
 *
 * getBarLine -- read the barLine.
 * Translation from PAE to verovio representation:
 *
 BARRENDITION_single     /
 BARRENDITION_end        does not exist
 BARRENDITION_rptboth    ://:
 BARRENDITION_rptend     ://
 BARRENDITION_rptstart   //:
 BARRENDITION_dbl        //
 */

int PAEInput::getBarLine(const char *incipit, data_BARRENDITION *output, int index)
{

    bool is_barline_rptboth = false;
    bool is_barline_rptend = false;
    bool is_barline_rptstart = false;
    bool is_barline_dbl = false;

    if (strncmp(incipit + index, "://:", 4) == 0) {
        is_barline_rptboth = true;
    }

    if (strncmp(incipit + index, "://", 3) == 0) {
        is_barline_rptend = true;
    }

    if (strncmp(incipit + index, "//:", 3) == 0) {
        is_barline_rptstart = true;
    }

    if (strncmp(incipit + index, "//", 2) == 0) {
        is_barline_dbl = true;
    }

    int i = 0; // number of characters
    if (is_barline_rptboth) {
        *output = BARRENDITION_rptboth;
        i = 3;
    }
    else if (is_barline_rptstart) {
        *output = BARRENDITION_rptstart;
        i = 2;
    }
    else if (is_barline_rptend) {
        *output = BARRENDITION_rptend;
        i = 2;
    }
    else if (is_barline_dbl) {
        *output = BARRENDITION_dbl;
        i = 1;
    }
    else {
        *output = BARRENDITION_single;
        i = 0;
    }
    return i;
}

//////////////////////////////
//
// getAbbreviation -- read abbreviation
//

int PAEInput::getAbbreviation(const char *incipit, pae::Measure *measure, int index)
{
    int length = (int)strlen(incipit);
    int i = index;

    if (measure->abbreviation_offset == -1) { // start
        measure->abbreviation_offset = (int)measure->notes.size();
    }
    else { //
        int abbreviation_stop = (int)measure->notes.size();
        while ((i + 1 < length) && (incipit[i + 1] == 'f')) {
            i++;
            for (int j = measure->abbreviation_offset; j < abbreviation_stop; ++j) {
                measure->notes.push_back(measure->notes.at(j));
                // With abbreviation, repeat clefs but do not copy keySig, meterSig and mensur
                if (measure->notes.back().clef)
                    measure->notes.back().clef = vrv_cast<Clef *>(measure->notes.back().clef->Clone());
                measure->notes.back().meter = NULL;
                measure->notes.back().key = NULL;
                measure->notes.back().mensur = NULL;
            }
        }
        measure->abbreviation_offset = -1;
    }

    return i - index;
}

//////////////////////////////
//
// getKeyInfo -- read the key signature.
//

int PAEInput::getKeyInfo(const char *incipit, KeySig *key, int index)
{
    key->Reset();

    int alt_nr = 0;
    std::string m_keySigString = "";

    // at the key information line, extract data
    int length = (int)strlen(incipit);
    int i = index;
    bool end_of_keysig = false;
    bool enclosed = false;
    bool has_enclosed = false;
    std::vector<bool> enclosedAccids;
    enclosedAccids.resize(7);
    bool cancel = false;
    data_ACCIDENTAL_WRITTEN alterationType = ACCIDENTAL_WRITTEN_NONE;
    while ((i < length) && (!end_of_keysig)) {
        switch (incipit[i]) {
            case 'b':
                alt_nr = 0;
                alterationType = ACCIDENTAL_WRITTEN_f;
                break;
            case 'x':
                alt_nr = 0;
                alterationType = ACCIDENTAL_WRITTEN_s;
                break;
            case 'n':
                alt_nr = 0;
                cancel = true;
                break;
            case '[':
                enclosed = true;
                has_enclosed = true;
                break;
            case ']': enclosed = false; break;
            case 'F':
            case 'C':
            case 'G':
            case 'D':
            case 'A':
            case 'E':
            case 'B': alt_nr++; break;
            default: end_of_keysig = true; break;
        }
        if (!end_of_keysig) {
            if (alt_nr < 7) {
                enclosedAccids.at(alt_nr) = enclosed;
            }
            i++;
        }
    }

    // Just in case
    alt_nr = std::min(6, alt_nr);

    if (alterationType != ACCIDENTAL_WRITTEN_NONE) {
        if (has_enclosed == true) {
            key->IsAttribute(false);
            for (int i = 0; i < alt_nr; ++i) {
                KeyAccid *keyAccid = new KeyAccid();
                data_PITCHNAME pname = (alterationType == ACCIDENTAL_WRITTEN_f) ? KeySig::s_pnameForFlats[i]
                                                                                : KeySig::s_pnameForSharps[i];
                keyAccid->SetPname(pname);
                keyAccid->SetAccid(alterationType);
                key->AddChild(keyAccid);
                if (enclosedAccids.at(i)) {
                    keyAccid->SetEnclose(ENCLOSURE_brack);
                }
            }
        }
        else {
            key->SetSig(std::make_pair(alt_nr, alterationType));
        }
        if (cancel) {
            key->SetSigShowchange(BOOLEAN_true);
        }
    }
    else {
        key->SetSig(std::make_pair(0, ACCIDENTAL_WRITTEN_n));
    }

    m_currentKeySig = key;
    key->FillMap(m_currentAccids);

    return i - index;
}

//////////////////////////////
//
// getNote --
//

int PAEInput::getNote(const char *incipit, pae::Note *note, pae::Measure *measure, int index)
{
    int oct;
    int i = index;
    int app;
    int tuplet_num;

    if (note->acciaccatura) {
        // acciaccaturas are always eights regardless
        // and have no dots
        note->duration = DURATION_8;
    }
    else {
        if (measure->durations.size() == 0) {
            note->duration = DURATION_4;
            note->dots = 0;
            LogWarning("Plaine & Easie import: found note before duration was specified");
        }
        else {
            note->duration = measure->durations.at(measure->durations_offset);
            note->dots = measure->dots.at(measure->durations_offset);
        }
    }
    note->pitch = getPitch(incipit[i]);

    // If necessary pass the accid from the tied note (could be from the previous measure)
    if (m_tieAccid.first != PITCHNAME_NONE) {
        if (m_tieAccid.second == ACCIDENTAL_WRITTEN_n) {
            if (m_currentAccids.count(m_tieAccid.first) != 0) {
                m_currentAccids.erase(m_tieAccid.first);
            }
        }
        else if (m_tieAccid.second != ACCIDENTAL_WRITTEN_NONE) {
            m_currentAccids[m_tieAccid.first] = m_tieAccid.second;
        }
    }
    m_tieAccid.first = PITCHNAME_NONE;
    m_tieAccid.second = ACCIDENTAL_WRITTEN_NONE;

    // Natural in front of the note, remove it from the current list
    if (note->accidental == ACCIDENTAL_WRITTEN_n) {
        if (m_currentAccids.count(note->pitch) != 0) {
            m_currentAccids.erase(note->pitch);
        }
    }
    // Not a natural in front of the note, add it to the current list
    else if (note->accidental != ACCIDENTAL_WRITTEN_NONE) {
        m_currentAccids[note->pitch] = note->accidental;
    }

    // Nothing in front of the note, but something in the list - make it an accid.ges
    if ((note->accidental == ACCIDENTAL_WRITTEN_NONE) && (m_currentAccids.count(note->pitch) != 0)) {
        note->accidental = m_currentAccids.at(note->pitch);
        note->accidGes = true;
    }

    // lookout, hack. If a rest (PITCHNAME_NONE val) then create rest object.
    // it will be added instead of the note
    if (note->pitch == PITCHNAME_NONE) {
        note->rest = true;
    }

    // chord
    if (regex_search(incipit + i + 1, std::regex("^[^A-G]*\\^"))) {
        note->chord = true;
    }

    // tie
    if (regex_search(incipit + i + 1, std::regex("^[^A-G]*\\+"))) {
        note->tie = true;
        if (note->accidental) {
            m_tieAccid.first = note->pitch;
            m_tieAccid.second = note->accidental;
        }
    }
    else {
        m_tieAccid.first = PITCHNAME_NONE;
        m_tieAccid.second = ACCIDENTAL_WRITTEN_NONE;
    }

    // trills
    if (regex_search(incipit + i + 1, std::regex("^[^A-G]*t"))) {
        note->trill = true;
    }

    oct = note->octave;
    measure->notes.push_back(*note);

    app = note->appoggiatura;
    tuplet_num = note->tuplet_note;

    // Reset note to defaults
    note->clear();

    // write back the values we need to save
    note->octave = oct; // save octave

    // tuplets. Decrease the current number if we are in a tuplet
    // i.e. tuplet_num > 0
    // al the other values just need to be in the first note
    if (tuplet_num > 0) {
        note->tuplet_note = --tuplet_num;
    }

    // grace notes
    note->acciaccatura = false;
    if (app > 0) {
        note->appoggiatura = --app;
    }
    // durations
    if (measure->durations.size() > 0) {
        measure->durations_offset++;
        if (measure->durations_offset >= (int)measure->durations.size()) {
            measure->durations_offset = 0;
        }
    }

    note->fermata = false; // only one note per fermata;
    note->trill = false;

    return i - index;
}

//////////////////////////////
//
// convertMeasure --
//

void PAEInput::convertMeasure(pae::Measure *measure)
{
    if (measure->clef != NULL) {
        m_layer->AddChild(measure->clef);
    }

    if (measure->wholerest > 0) {
        if (measure->wholerest == 1) {
            MRest *mRest = new MRest;
            m_layer->AddChild(mRest);
        }
        else {
            MultiRest *multiRest = new MultiRest();
            multiRest->SetNum(measure->wholerest);
            m_layer->AddChild(multiRest);
        }
    }

    m_nested_objects.clear();

    for (unsigned int i = 0; i < measure->notes.size(); ++i) {
        pae::Note *note = &measure->notes.at(i);
        parseNote(note);
    }

    // Set barLine
    m_measure->SetRight(measure->barLine);
}

void PAEInput::parseNote(pae::Note *note)
{

    LayerElement *element;

    if (note->rest) {
        Rest *rest = new Rest();

        rest->SetDur(note->duration);

        if (!m_is_mensural && note->dots != 0) {
            rest->SetDots(note->dots);
        }

        if (note->fermata) {
            Fermata *fermata = new Fermata();
            fermata->SetStartid("#" + rest->GetUuid());
            m_measure->AddChild(fermata);
        }

        element = rest;
    }
    else {
        Note *mnote = new Note();

        mnote->SetPname(note->pitch);
        mnote->SetOct(note->octave);
        if (note->accidental != ACCIDENTAL_WRITTEN_NONE) {
            Accid *accid = new Accid();
            if (note->accidGes) {
                accid->SetAccidGes(Att::AccidentalWrittenToGestural(note->accidental));
            }
            else {
                accid->SetAccid(note->accidental);
            }
            mnote->AddChild(accid);
        }

        mnote->SetDur(note->duration);

        if (!m_is_mensural && note->dots != 0) {
            mnote->SetDots(note->dots);
        }

        // pseudo chant notation with 7. in PAE - make quater notes without stem
        if ((mnote->GetDur() == DURATION_128) && (mnote->GetDots() == 1)) {
            mnote->SetDur(DURATION_4);
            mnote->SetDots(0);
            mnote->SetStemLen(0);
            mnote->SetStemVisible(BOOLEAN_false);
        }

        if (note->fermata) {
            Fermata *fermata = new Fermata();
            fermata->SetStartid("#" + mnote->GetUuid());
            m_measure->AddChild(fermata);
        }

        if (note->trill) {
            Trill *trill = new Trill();
            trill->SetStartid("#" + mnote->GetUuid());
            m_measure->AddChild(trill);
        }

        if (m_tie != NULL) {
            m_tie->SetEndid("#" + mnote->GetUuid());
            m_tie = NULL;
        }

        if (note->tie) {
            m_tie = new Tie();
            m_tie->SetStartid("#" + mnote->GetUuid());
            m_measure->AddChild(m_tie);
        }

        element = mnote;
    }

    // Does this note have a clef change? push it before everything else
    if (note->clef) {
        addLayerElement(note->clef);
        note->clef = NULL;
    }

    // Same thing for time changes
    // You can find this sometimes
    if (note->meter) {
        addLayerElement(note->meter);
        note->meter = NULL;
    }
    if (note->mensur) {
        addLayerElement(note->mensur);
        note->mensur = NULL;
    }

    // Handle key change. Evil if done in a beam
    if (note->key) {
        addLayerElement(note->key);
        note->key = NULL;
    }

    // Acciaccaturas are similar but do not get beamed (do they)
    // this case is simpler. NOTE a note can not be acciacctura AND appoggiatura
    // Acciaccatura rests do not exist
    if (note->acciaccatura && (element->Is(NOTE))) {
        Note *mnote = vrv_cast<Note *>(element);
        assert(mnote);
        mnote->SetDur(DURATION_8);
        mnote->SetGrace(GRACE_unacc);
        mnote->SetStemDir(STEMDIRECTION_up);
    }

    if ((note->appoggiatura > 0) && (element->Is(NOTE))) {
        Note *mnote = vrv_cast<Note *>(element);
        assert(mnote);
        mnote->SetGrace(GRACE_acc);
        mnote->SetStemDir(STEMDIRECTION_up);
    }

    if ((note->beam == BEAM_INITIAL) && !m_is_mensural) {
        pushContainer(new Beam());
    }

    // we have a tuplet, the tuplet_note is > 0
    // which means we are counting a tuplet
    if (note->tuplet_note > 0 && note->tuplet_notes == note->tuplet_note) { // first elem in tuplet
        Tuplet *newTuplet = new Tuplet();
        newTuplet->SetNum(note->tuplet_val);
        newTuplet->SetNumbase(2);
        pushContainer(newTuplet);
    }

    if ((note->beam == BEAM_TUPLET) && !m_is_mensural) {
        pushContainer(new Beam());
    }

    // note in a chord
    if (note->chord && element->Is(NOTE)) {
        Note *mnote = dynamic_cast<Note *>(element);
        assert(mnote);
        // first note?
        if (!m_is_in_chord) {
            Chord *chord = new Chord();
            chord->SetDots(mnote->GetDots());
            chord->SetDur(mnote->GetDur());
            pushContainer(chord);
            m_is_in_chord = true;
        }
        mnote->ResetAugmentDots();
        mnote->ResetDurationLogical();
    }

    // Add the note to the current container
    addLayerElement(element);
    element = NULL;

    // Add mensural dot
    if (m_is_mensural && note->dots > 0) {
        addLayerElement(new Dot());
    }

    // the last note counts always '1'
    // insert the tuplet elem
    // and reset the tuplet counter
    if (note->tuplet_note == 1) {
        popContainer();
    }

    if ((note->beam == BEAM_TERMINAL) && !m_is_mensural) {
        popContainer();
    }

    // last note of a chord
    if (!note->chord && m_is_in_chord) {
        Note *mnote = dynamic_cast<Note *>(element);
        if (mnote) {
            mnote->ResetAugmentDots();
            mnote->ResetDurationLogical();
        }
        popContainer();
        m_is_in_chord = false;
    }
}

void PAEInput::pushContainer(LayerElement *container)
{
    addLayerElement(container);
    m_nested_objects.push_back(container);
}

void PAEInput::popContainer()
{
    // assert(m_nested_objects.size() > 0);
    if (m_nested_objects.size() == 0) {
        LogError("Plaine & Easie import: tried to pop an object from empty stack. "
                 "Cross-measure objects (tuplets, beams) are not supported.");
    }
    else {
        m_nested_objects.pop_back();
    }
}

void PAEInput::addLayerElement(LayerElement *element)
{
    if (m_nested_objects.size() > 0) {
        if (!m_nested_objects.back()->IsSupportedChild(element)) {
            delete element;
            element = NULL;
        }
        else {
            m_nested_objects.back()->AddChild(element);
        }
    }
    else {
        m_layer->AddChild(element);
    }
}

//////////////////////////////
//
// getAtRecordKeyValue --
//   Formats: @key: value
//            @key:value
//            @key :value
//            @ key :value
//            @ key : value
//   only one per line
//

void PAEInput::getAtRecordKeyValue(char *key, char *value, const char *input)
{

#define SKIPSPACE                                                                                                      \
    while ((index < length) && isspace(input[index])) {                                                                \
        index++;                                                                                                       \
    }

    char MARKER = '@';
    char SEPARATOR = ':';
    char EMPTY = '\0';

    int length = (int)strlen(input);
    int count = 0;

    // zero out strings
    memset(key, EMPTY, MAX_DATA_LEN);
    memset(value, EMPTY, MAX_DATA_LEN);

    if (length == 0) {
        return;
    }

    char ch;
    int index = 0;

    // find starting @ symbol (ignoring any starting space)
    SKIPSPACE
    if (input[index] != MARKER) {
        // invalid record format since it does not start with @
        return;
    }
    index++;
    SKIPSPACE

    // start storing the key value:
    while ((index < length) && (input[index] != SEPARATOR)) {
        if (isspace(input[index])) {
            index++;
            continue;
        }
        ch = input[index];

        // Should never happen
        if (count >= MAX_DATA_LEN) return;

        key[count] = ch;
        count++;
        index++;
    }
    // check to see if valid format: (:) must be the current character
    if (input[index] != SEPARATOR) {
        key[0] = EMPTY;
        return;
    }
    index++;
    SKIPSPACE

    // Also should never happen
    if (strlen(&input[index]) > MAX_DATA_LEN) return;

    strcpy(value, &input[index]);

    // Truncate string to first space
    size_t i;
    for (i = strlen(value) - 2; i > 0; i--) {
        if (isspace(value[i])) {
            value[i] = EMPTY;
            continue;
        }
        break;
    }
}

#endif // NO_PAE_SUPPORT

//----------------------------------------------------------------------------
// PAEInput2
//----------------------------------------------------------------------------

namespace pae {

    static const char CONTAINER_END = '~';
    static const char VOID = '_';
    static const std::string INTERNAL_CHARS = "QXY";
    static const char OCTAVEUP = '\'';
    static const char OCTAVEDOWN = ',';
    static const char KEYSIG_START = '$';
    static const std::string KEYSIG = "xnb[]ABCDEFG";
    static const char CLEF_START = '%';
    static const std::string CLEF = "GCFg-+12345";
    static const char METERSIG_START = '@';
    static const std::string METERSIG = "/o.c0123456789";
    static const std::string GRACE = "qg";
    static const std::string NOTENAME = "ABCDEFG";
    static const std::string DURATION = "0123456789.";
    // preprocessing of the data replaces xx with X and bb with Y
    static const std::string ACCIDENTAL_INTERNAL = "xbnXY";
    static const std::string MEASURE = ":/";

    Token::Token(char c, int position, Object *object)
    {
        m_char = c;
        m_inputChar = c;
        m_position = position;
        m_object = object;
        m_isError = false;
    }

    Token::~Token() {}

    bool Token::Is(ClassId classId) { return (m_object && m_object->Is(classId)); }

    bool Token::IsContainerEnd() { return (m_object && (m_char == pae::CONTAINER_END)); }

    bool Token::IsEnd() { return (!m_object && (m_char == pae::CONTAINER_END)); }

    bool Token::IsSpace() { return (m_char == ' '); }

    bool Token::IsVoid() { return (m_char == pae::VOID); }

    std::string Token::GetName()
    {
        if (!m_object) return "?";
        std::string name = m_object->GetClassName();
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        return name;
    }

} // namespace pae

PAEInput2::PAEInput2(Doc *doc) : Input(doc) {}

PAEInput2::~PAEInput2()
{
    this->ClearTokenObjects();
}

void PAEInput2::ClearTokenObjects()
{
    // Before we clear the pae list of tokens, we need to delete all token objects.
    // Normally, they should be none because they are passed to the doc.
    for (auto &token : m_pae) {
        if (!token.m_object || token.IsContainerEnd()) continue;
        LogDebug("Delete token %s", token.m_object->GetClassName().c_str());
        delete token.m_object;
        token.m_object = NULL;
    }
    m_pae.clear();
}

void PAEInput2::LogPAE(std::string msg, pae::Token &token)
{
    m_hasErrors = true;
    token.m_isError = true;
    std::string posStr;
    if (token.m_position == -1) {
        posStr = "(unknown position)";
    }
    else {
        posStr = StringFormat("(character %d)", token.m_position);
    }
    std::string fullMsg = StringFormat("PAE: %s %s", msg.c_str(), posStr.c_str());

    if (m_pedanticMode) {
        LogError(fullMsg.c_str());
    }
    else {
        LogWarning(fullMsg.c_str());
    }
}

void PAEInput2::LogDebugTokens(bool vertical)
{
    // For long incipits or to see full class name
    if (vertical) {
        for (auto &token : m_pae) {
            char c1 = (token.m_char) ? token.m_char : ' ';
            char c2 = (token.m_inputChar) ? token.m_inputChar : ' ';
            std::string className = (token.m_object) ? token.m_object->GetClassName() : "";
            if (token.m_isError) className += " <";
            LogDebug(" %c | %c | %s", c1, c2, className.c_str());
        }
    }
    else {
        std::string row;
        for (auto &token : m_pae) {
            char c = (token.m_inputChar) ? token.m_inputChar : ' ';
            row.push_back(c);
        }
        LogDebug(row.c_str());
        if (m_hasErrors) {
            row.clear();
            for (auto &token : m_pae) {
                char c = (token.m_isError) ? '^' : ' ';
                row.push_back(c);
            }
            LogDebug(row.c_str());
        }
        row.clear();
        for (auto &token : m_pae) {
            std::string className = (token.m_object) ? token.m_object->GetClassName() : " ";
            row.push_back(className.at(0));
        }
        LogDebug(row.c_str());
        row.clear();
        for (auto &token : m_pae) {
            char c = (token.m_char) ? token.m_char : ' ';
            row.push_back(c);
        }
    }
}

bool PAEInput2::Is(pae::Token &token, const std::string &map)
{
    return (map.find(token.m_char) != std::string::npos);
}

bool PAEInput2::Was(pae::Token &token, const std::string &map)
{
    return (map.find(token.m_inputChar) != std::string::npos);
}

void PAEInput2::AddToken(char c, int &position)
{
    m_pae.push_back(pae::Token(c, position));
    // Internal characters are used to replace double letters for easier processing
    // When we add them as token, we want to store the original letters and their position
    // This means converting the internal characters back and stored as m_inputChar
    // The second letter is also marked as pae::VOID so it can be skipped during parsing
    if (this->Is(m_pae.back(), pae::INTERNAL_CHARS)) {
        // The position is incremented because these are actually input chars
        position++;
        if (c == 'Q') {
            m_pae.back().m_inputChar = 'q';
            m_pae.push_back(pae::Token('q', position));
        }
        if (c == 'X') {
            m_pae.back().m_inputChar = 'x';
            m_pae.push_back(pae::Token('x', position));
        }
        if (c == 'Y') {
            m_pae.back().m_inputChar = 'b';
            m_pae.push_back(pae::Token('b', position));
        }
        m_pae.back().m_char = pae::VOID;
    }
}

jsonxx::Object PAEInput2::InputKeysToJson(const std::string &inputKeys)
{
    jsonxx::Object jsonInput;

    std::istringstream iss(inputKeys);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.rfind("@clef:", 0) != std::string::npos) {
            jsonInput << "clef" << line.substr(line.find(":") + 1);
        }
        else if (line.rfind("@key:", 0) != std::string::npos) {
            jsonInput << "key" << line.substr(line.find(":") + 1);
        }
        else if (line.rfind("@keysig:", 0) != std::string::npos) {
            jsonInput << "keysig" << line.substr(line.find(":") + 1);
        }
        else if (line.rfind("@timesig:", 0) != std::string::npos) {
            jsonInput << "timesig" << line.substr(line.find(":") + 1);
        }
        else if (line.rfind("@data:", 0) != std::string::npos) {
            jsonInput << "data" << line.substr(line.find(":") + 1);
        }
    }
    // LogDebug("%s", jsonInput.json().c_str());

    return jsonInput;
}

bool PAEInput2::Import(const std::string &input)
{
    this->ClearTokenObjects();

    m_hasErrors = false;

    if (input.size() == 0) {
        LogError("PAE: Input is empty");
        return false;
    }

    jsonxx::Object jsonInput;
    if (input.at(0) == '{') {
        if (!jsonInput.parse(input)) {
            LogError("PAE: Cannot parse the JSON input");
            return false;
        }
    }
    else {
        jsonInput = this->InputKeysToJson(input);
    }

    // Add a measure at the beginning of the data because there is always at least one measure
    Measure *measure = new Measure(true, 1);
    // By default there is no end barline on an incipit
    measure->SetRight(BARRENDITION_invis);
    m_pae.push_back(pae::Token(0, -1, measure));

    if (jsonInput.has<jsonxx::String>("data")) {
        std::string data = jsonInput.get<jsonxx::String>("data");

        // Remove non PAE internal characters
        for (char c : pae::INTERNAL_CHARS) {
            data.erase(std::remove(data.begin(), data.end(), c), data.end());
        }

        data = std::regex_replace(data, std::regex("qq"), "Q");
        data = std::regex_replace(data, std::regex("xx"), "X");
        data = std::regex_replace(data, std::regex("bb"), "Y");

        int i = 0;
        for (char c : data) {
            // Ignore the charcter that is use internally as container end Token
            if (c == pae::CONTAINER_END) continue;
            // Otherwise go ahead
            this->AddToken(c, i);
            i++;
        }
    }
    else {
        LogError("PAE: No 'data' key in the JSON input");
        return false;
    }

    // Add a token marking the end - special use of the CONTAINER_END with no object
    // See pae::Token::IsEnd();
    m_pae.push_back(pae::Token(pae::CONTAINER_END, -1));

    m_isMensural = false;

    m_pedanticMode = false;

    return this->Parse();
}

bool PAEInput2::Parse()
{
    bool success = true;

    if (success) success = this->ConvertKeySig();

    if (success) success = this->ConvertClef();

    if (success) success = this->ConvertMeterSigOrMensur();

    if (success) success = this->ConvertMeasure();

    if (success) success = this->ConvertMRestOrMultiRest();

    if (success) success = this->ConvertPitch();

    if (success) success = this->ConvertOctave();

    if (success) success = this->ConvertTrill();

    if (success) success = this->ConvertFermata();

    if (success) success = this->ConvertAccidental();

    if (success) success = this->ConvertRest();

    if (success) success = this->ConvertBeam();

    if (success) success = this->ConvertGraceGrp();

    if (success) success = this->ConvertGrace();

    if (success) success = this->CheckHierarchy();

    LogDebugTokens();

    if (m_pedanticMode && !success) {
        this->ClearTokenObjects();
        return false;
    }

    m_doc->Reset();
    m_doc->SetType(Raw);
    // The mdiv
    Mdiv *mdiv = new Mdiv();
    mdiv->m_visibility = Visible;
    m_doc->AddChild(mdiv);
    // The score
    Score *score = new Score();
    mdiv->AddChild(score);
    // the section
    Section *section = new Section();
    score->AddChild(section);

    // add minimal scoreDef
    StaffGrp *staffGrp = new StaffGrp();
    StaffDef *staffDef = new StaffDef();
    staffDef->SetN(1);
    staffDef->SetLines(5);
    staffGrp->AddChild(staffDef);
    m_doc->GetCurrentScoreDef()->AddChild(staffGrp);

    // A stack to which layer element are added. At least a Layer, but then Beam, GraceGrp, Chord, etc.
    ListOfObjects layerElementContainers;
    // If we have a meterSig or a keySig change, we will add a new ScoreDef before the current measure
    ScoreDef *scoreDefChange = NULL;
    // The current measure, used to know where to  add a scoreDef change
    Measure *currentMeasure = NULL;

    for (auto &token : m_pae) {
        if (token.IsVoid()) continue;

        // Double check that we don't have more than the layer on the layerStack
        if (token.IsEnd()) {
            if (layerElementContainers.size() > 1) {
                LogDebug("The layer element container stack should not have more than one element");
            }
            continue;
        }

        // No object to add to the doc - we whould also have no char left
        if (!token.m_object) {
            if (token.m_char != 0) LogDebug("Remaining unprocessed char '%c'", token.m_char);
            continue;
        }

        // Everytime we have a Measure, we need to add it to the section and fill it with a Staff and Layer
        if (token.Is(MEASURE)) {
            currentMeasure = dynamic_cast<Measure *>(token.m_object);
            assert(currentMeasure);
            token.m_object = NULL;

            section->AddChild(currentMeasure);
            Staff *staff = new Staff(1);
            currentMeasure->AddChild(staff);
            Layer *layer = new Layer();
            layer->SetN(1);
            staff->AddChild(layer);
            // Clear the layer element container stack (only max 1 should be left) and the new layer
            if (layerElementContainers.size() > 1) {
                LogDebug("The layer element container stack should not have more than one element");
            }
            layerElementContainers.clear();
            layerElementContainers.push_back(layer);
            // Reset the scoreDefChange pointer - a new one will be created if necessary
            scoreDefChange = NULL;
        }
        // Place the keySig, mensur or meterSig to the scoreDefChange - create it if necessary
        else if (token.m_object->Is({ KEYSIG, MENSUR, METERSIG })) {
            if (!scoreDefChange) {
                scoreDefChange = new ScoreDef();
                section->InsertBefore(currentMeasure, scoreDefChange);
            }
            // For now ignore additional changes - not sure how these should be handled in MEI anyway
            if (scoreDefChange->FindDescendantByType(token.m_object->GetClassId())) {
                LogPAE(
                    StringFormat("%s change cannot occur more than once in a measure", token.GetName().c_str()), token);
                if (m_pedanticMode) return false;
                delete token.m_object;
            }
            else {
                scoreDefChange->AddChild(token.m_object);
                // For the meterSig and mensur, we can have them as attribute. KeySig not because of the enclose
                // attributes
                if (token.m_object->Is({ MENSUR, METERSIG })) {
                    token.m_object->IsAttribute(true);
                }
            }
            // Object are own by the scoreDef
            token.m_object = NULL;
            continue;
        }
        else if (token.m_object->IsLayerElement()) {

            LayerElement *element = dynamic_cast<LayerElement *>(token.m_object);
            assert(element);
            // The object is either a container end, or will be added to the layerElementContainers.back()
            token.m_object = NULL;

            // For a container end, no object to add to the doc.
            if (token.m_char == pae::CONTAINER_END) {
                // Do check that we still have more than the layer before poping it from the stack
                if (layerElementContainers.size() < 2) {
                    LogDebug("The layer element container stack should have a least two elements");
                    continue;
                }
                // Also double check that the open / close element is actually the same
                if (layerElementContainers.back() != element) {
                    LogDebug("The layer element container stack top and the container end should match");
                }
                // Simply pop it and continue
                layerElementContainers.pop_back();
                continue;
            }

            // Something went really wrong... Still delete the element to avoid a memory leak
            if (layerElementContainers.empty()) {
                LogDebug("The layer element container stack should have a least one element");
                delete element;
                continue;
            }
            layerElementContainers.back()->AddChild(element);

            // Add to the stack the layer element that are containers
            if (element->Is({ BEAM, GRACEGRP })) {
                layerElementContainers.push_back(element);
            }
        }
        else if (token.m_object->IsControlElement()) {
            assert(currentMeasure);
            currentMeasure->AddChild(token.m_object);
            token.m_object = NULL;
        }
    }

    // We should have no object left, just in case they need to be delete.
    this->ClearTokenObjects();

    m_doc->ConvertToPageBasedDoc();

    return success;
}

bool PAEInput2::ConvertKeySig()
{
    pae::Token *keySigToken = NULL;
    std::string paeStr;

    for (auto &token : m_pae) {
        if (token.IsVoid()) continue;

        if (token.m_char == pae::KEYSIG_START) {
            keySigToken = &token;
            paeStr.clear();
        }
        else if (keySigToken) {
            if (this->Is(token, pae::KEYSIG)) {
                paeStr.push_back(token.m_char);
                token.m_char = 0;
            }
            else if (!token.IsSpace()) {
                LogPAE("Missig ' ' after a key signature change", token);
                if (m_pedanticMode) return false;
            }
            else {
                keySigToken->m_char = 0;
                // LogDebug("Keysig %s", paeStr.c_str());
                KeySig *keySig = new KeySig();
                keySigToken->m_object = keySig;
                // Will fail in pedantic mode
                if (!this->ParseKeySig(keySig, paeStr, token)) return false;
                keySigToken = NULL;
            }
        }
    }

    return true;
}

bool PAEInput2::ConvertClef()
{
    pae::Token *clefToken = NULL;
    std::string paeStr;

    for (auto &token : m_pae) {
        if (token.IsVoid()) continue;

        if (token.m_char == pae::CLEF_START) {
            clefToken = &token;
            paeStr.clear();
        }
        else if (clefToken) {
            if (this->Is(token, pae::CLEF)) {
                paeStr.push_back(token.m_char);
                token.m_char = 0;
            }
            else if (!token.IsSpace()) {
                LogPAE("Missig ' ' after a clef change", token);
                if (m_pedanticMode) return false;
            }
            else {
                clefToken->m_char = 0;
                // LogDebug("Clef %s", paeStr.c_str());
                Clef *clef = new Clef();
                clefToken->m_object = clef;
                // Will fail in pedantic mode
                if (!this->ParseClef(clef, paeStr, token)) return false;
                clefToken = NULL;
            }
        }
    }

    return true;
}

bool PAEInput2::ConvertMeterSigOrMensur()
{
    pae::Token *meterSigOrMensurToken = NULL;
    std::string paeStr;

    for (auto &token : m_pae) {
        if (token.IsVoid()) continue;

        if (token.m_char == pae::METERSIG_START) {
            meterSigOrMensurToken = &token;
            paeStr.clear();
        }
        else if (meterSigOrMensurToken) {
            if (this->Is(token, pae::METERSIG)) {
                paeStr.push_back(token.m_char);
                token.m_char = 0;
            }
            else if (!token.IsSpace()) {
                LogPAE("Missig ' ' after a meter signature change", token);
                if (m_pedanticMode) return false;
            }
            else {
                meterSigOrMensurToken->m_char = 0;
                // LogDebug("MeterSig %s", paeStr.c_str());
                if (m_isMensural) {
                    Mensur *mensur = new Mensur();
                    meterSigOrMensurToken->m_object = mensur;
                    // Will fail in pedantic mode
                    if (!this->ParseMensur(mensur, paeStr, token)) return false;
                }
                else {
                    MeterSig *meterSig = new MeterSig();
                    meterSigOrMensurToken->m_object = meterSig;
                    // Will fail in pedantic mode
                    if (!this->ParseMeterSig(meterSig, paeStr, token)) return false;
                }
                meterSigOrMensurToken = NULL;
            }
        }
    }

    return true;
}

bool PAEInput2::ConvertMeasure()
{
    Measure *currentMeasure;
    pae::Token *measureToken = NULL;
    std::string paeStr;
    // measureCount is currently ignored by the Measure constructor
    int measureCount = 1;

    for (auto &token : m_pae) {
        if (token.IsVoid()) continue;

        // This is the first (default) measure added to the tokens ::Import
        if (token.Is(MEASURE)) {
            currentMeasure = dynamic_cast<Measure *>(token.m_object);
            assert(currentMeasure);
        }
        if (this->Is(token, pae::MEASURE)) {
            if (!measureToken) {
                measureToken = &token;
            }
            paeStr.push_back(token.m_char);
            token.m_char = 0;
        }
        else if (measureToken) {
            // When reaching a barline, we need to set it to the previous measure (@right)
            if (!this->ParseMeasure(currentMeasure, paeStr, token)) return false;
            // We can now create a new measure but not if we have reached the end of the data
            if (!token.IsEnd()) {
                measureCount++;
                currentMeasure = new Measure(true, measureCount);
                currentMeasure->SetRight(BARRENDITION_invis);
                measureToken->m_object = currentMeasure;
            }
            measureToken = NULL;
            paeStr.clear();
        }
    }

    return true;
}

bool PAEInput2::ConvertMRestOrMultiRest()
{
    pae::Token *mRestOrMultiRestToken = NULL;
    std::string paeStr;

    for (auto &token : m_pae) {
        if (token.IsVoid()) continue;

        if (token.m_char == '=') {
            if (mRestOrMultiRestToken) {
                LogPAE("Invalid = after a =", token);
                if (m_pedanticMode) return false;
            }
            mRestOrMultiRestToken = &token;
            token.m_char = 0;
        }
        else if (mRestOrMultiRestToken) {
            if (isdigit(token.m_char)) {
                paeStr.push_back(token.m_char);
                token.m_char = 0;
            }
            else {
                if (!paeStr.empty() && paeStr.at(0) == '0') {
                    LogPAE("Invalid (multi) measure rest number starting with 0", token);
                    if (m_pedanticMode) return false;
                    paeStr.erase(0, paeStr.find_first_not_of('0'));
                }
                if (paeStr.empty() || paeStr == "1") {
                    mRestOrMultiRestToken->m_object = new MRest();
                }
                else {
                    MultiRest *multiRest = new MultiRest();
                    multiRest->SetNum(atoi(paeStr.c_str()));
                    mRestOrMultiRestToken->m_object = multiRest;
                }
                mRestOrMultiRestToken = NULL;
                paeStr.clear();
            }
        }
    }

    return true;
}

bool PAEInput2::ConvertPitch()
{
    for (auto &token : m_pae) {
        if (token.IsVoid()) continue;

        if (Is(token, pae::NOTENAME)) {
            Note *note = new Note();
            data_PITCHNAME pitch = PITCHNAME_c;
            switch (token.m_char) {
                case 'A': pitch = PITCHNAME_a; break;
                case 'B': pitch = PITCHNAME_b; break;
                case 'C': pitch = PITCHNAME_c; break;
                case 'D': pitch = PITCHNAME_d; break;
                case 'E': pitch = PITCHNAME_e; break;
                case 'F': pitch = PITCHNAME_f; break;
                case 'G': pitch = PITCHNAME_g; break;
                default: break;
            }
            note->SetPname(pitch);
            token.m_object = note;
            token.m_char = 0;
        }
    }

    return true;
}

bool PAEInput2::ConvertOctave()
{
    int oct = 4;
    char readingOct = 0;

    for (auto &token : m_pae) {
        if (token.IsVoid()) continue;

        if (token.m_char == pae::OCTAVEUP) {
            // Init to 4 when starting to read octave '
            if (readingOct != pae::OCTAVEUP) {
                oct = 4;
                readingOct = pae::OCTAVEUP;
            }
            else {
                oct++;
            }
            token.m_char = 0;
        }
        else if (token.m_char == pae::OCTAVEDOWN) {
            // Init to 3 when starting to read octave ,
            if (readingOct != pae::OCTAVEDOWN) {
                oct = 3;
                readingOct = pae::OCTAVEDOWN;
            }
            else {
                oct--;
            }
            token.m_char = 0;
        }
        else {
            // We are not reading octave signs anymore
            readingOct = 0;
        }

        // Simply set is to the notes
        if (token.Is(NOTE)) {
            Note *note = dynamic_cast<Note *>(token.m_object);
            assert(note);
            note->SetOct(oct);
            note->SetDur(DURATION_8);
        }
    }

    return true;
}

bool PAEInput2::ConvertTrill()
{
    Object *note = NULL;

    for (auto &token : m_pae) {
        if (token.IsVoid()) continue;

        // Keep a pointer and simply continue
        if (token.Is(NOTE)) {
            note = token.m_object;
            continue;
        }
        if (token.m_char == 't') {
            token.m_char = 0;
            if (note) {
                Trill *trill = new Trill();
                trill->SetStartid("#" + note->GetUuid());
                token.m_object = trill;
            }
            else {
                LogPAE("Invalid t not after a note", token);
                if (m_pedanticMode) return false;
            }
            note = NULL;
            continue;
        }
        // A trill can be placed after the closing fermata ) or after a tie +
        if (note && (token.m_char == ')' || token.m_char == '+')) {
            continue;
        }
        // Anything else that is not a fermata or tie means that a previous note is no longer a target
        else {
            note = NULL;
        }
    }

    return true;
}

bool PAEInput2::ConvertFermata()
{
    pae::Token *fermataToken = NULL;
    Object *fermataTarget = NULL;

    for (auto &token : m_pae) {
        if (token.IsVoid()) continue;

        if (token.m_char == '(') {
            // Weird case - could be a
            if (fermataToken) {
                LogPAE("Invalid ( after a (", token);
                if (m_pedanticMode) return false;
            }
            fermataToken = &token;
        }
        else if (fermataToken) {
            // We have an open fermata signed but have not reached a fermata target
            if (!fermataTarget) {
                if (token.m_object && token.m_object->Is({ MREST, NOTE, REST })) {
                    fermataTarget = token.m_object;
                    continue;
                }
                // This was probably not a fermata sign but a tuplet one
                else {
                    fermataToken = NULL;
                    continue;
                }
            }
            else {
                if (token.m_char == ')') {
                    Fermata *fermata = new Fermata();
                    fermataToken->m_object = fermata;
                    fermata->SetStartid("#" + fermataTarget->GetUuid());
                    fermataToken->m_char = 0;
                    token.m_char = 0;
                    fermataToken = NULL;
                    fermataTarget = NULL;
                }
                // A trill before the closing fermata ) is valid
                else if (fermataTarget->Is(NOTE) && token.Is(TRILL)) {
                    continue;
                }
                // PAE guidelines are ambiguous because they say fermata should contain only a single rest sign (=)
                // but at the same time allow =1 for a mrest - in non pendantic mode we want to support (=1)
                else if (fermataTarget->Is(MREST) && isdigit(token.m_inputChar)) {
                    LogPAE(StringFormat("Fermata on measure rest with extraneous %c", token.m_inputChar), token);
                    if (m_pedanticMode) return false;
                    continue;
                }
                else {
                    // Leave everything as is - the ( could be a tuplet start
                    fermataToken = NULL;
                    fermataTarget = NULL;
                }
            }
        }
    }

    return true;
}

bool PAEInput2::ConvertAccidental()
{
    data_ACCIDENTAL_WRITTEN accidental = ACCIDENTAL_WRITTEN_NONE;

    for (auto &token : m_pae) {
        if (token.IsVoid()) continue;

        if (Is(token, pae::ACCIDENTAL_INTERNAL)) {
            switch (token.m_char) {
                case 'x': accidental = ACCIDENTAL_WRITTEN_s; break;
                case 'b': accidental = ACCIDENTAL_WRITTEN_f; break;
                case 'n': accidental = ACCIDENTAL_WRITTEN_n; break;
                case 'X': accidental = ACCIDENTAL_WRITTEN_x; break;
                case 'Y': accidental = ACCIDENTAL_WRITTEN_ff; break;
                default: break;
            }
            token.m_char = 0;
        }
        else if (accidental != ACCIDENTAL_WRITTEN_NONE) {
            if (token.Is(NOTE)) {
                Note *note = dynamic_cast<Note *>(token.m_object);
                assert(note);
                Accid *accid = new Accid();
                accid->SetAccid(accidental);
                note->AddChild(accid);
                accidental = ACCIDENTAL_WRITTEN_NONE;
            }
            // The note has a fermata, one more step to get it
            else if (token.Is(FERMATA)) {
                continue;
            }
            else {
                LogPAE("Missing note after an accidental", token);
                if (m_pedanticMode) return false;
                accidental = ACCIDENTAL_WRITTEN_NONE;
            }
        }
    }
    return true;
}

bool PAEInput2::ConvertRest()
{
    for (auto &token : m_pae) {
        if (token.IsVoid()) continue;

        if (token.m_char == '-') {
            token.m_object = new Rest();
            token.m_char = 0;
        }
    }

    return true;
}

bool PAEInput2::ConvertBeam()
{
    Beam *beam = NULL;

    // Here we need an iterator because we might have to add a missing closing tag
    std::list<pae::Token>::iterator token = m_pae.begin();
    while (token != m_pae.end()) {
        if (token->IsVoid()) {
            ++token;
            continue;
        }

        if (token->m_char == '{') {
            token->m_char = 0;
            if (beam) {
                LogPAE("Nested beams are not supported", *token);
                if (m_pedanticMode) return false;
                ++token;
                continue;
            }
            beam = new Beam();
            token->m_object = beam;
        }
        else if (token->m_char == '}') {
            token->m_char = 0;
            if (!beam) {
                LogPAE("Irrelevant closing beam", *token);
                if (m_pedanticMode) return false;
                ++token;
                continue;
            }
            token->m_object = beam;
            token->m_char = pae::CONTAINER_END;
            beam = NULL;
        }
        else if (token->IsEnd() || token->Is(MEASURE)) {
            if (beam) {
                LogPAE("Unclose beam at the end of a measure", *token);
                if (m_pedanticMode) return false;
                token = m_pae.insert(token, pae::Token(pae::CONTAINER_END, -1, beam));
                beam = NULL;
            }
        }
        ++token;
    }

    return true;
}

bool PAEInput2::ConvertGraceGrp()
{
    /*
    // This is now commented and not necessary anymore because qq are replaced by Q in the input
    // Left here for documentation

    // Do a first loop to change 'qq' to 'Q' for eaiser grace groups detection
    pae::Token *graceGrpToken = NULL;

    for (auto &token : m_pae) {
        if (token.IsVoid()) continue;

        if (token.m_char == 'q') {
            if (!graceGrpToken) {
                graceGrpToken = &token;
                continue;
            }
            else {
                graceGrpToken->m_char = 0;
                token.m_char = 'Q';
            }
        }
        graceGrpToken = NULL;
    }
    */

    GraceGrp *graceGrp = NULL;

    // Here we need an iterator because we might have to add a missing closing tag
    std::list<pae::Token>::iterator token = m_pae.begin();
    while (token != m_pae.end()) {
        if (token->IsVoid()) {
            ++token;
            continue;
        }

        if (token->m_char == 'Q') {
            token->m_char = 0;
            if (graceGrp) {
                LogPAE("Nested grace groups are not supported", *token);
                if (m_pedanticMode) return false;
                ++token;
                continue;
            }
            graceGrp = new GraceGrp();
            token->m_object = graceGrp;
        }
        else if (token->m_char == 'r') {
            token->m_char = 0;
            if (!graceGrp) {
                LogPAE("Irrelevant closing grace group", *token);
                if (m_pedanticMode) return false;
                ++token;
                continue;
            }
            token->m_object = graceGrp;
            token->m_char = pae::CONTAINER_END;
            graceGrp = NULL;
        }
        else if (this->Is(*token, pae::GRACE)) {
            if (graceGrp) {
                LogPAE("Grace within a grace group is not supported", *token);
                if (m_pedanticMode) return false;
                token->m_char = 0;
            }
        }
        else if (token->IsEnd() || token->Is(MEASURE)) {
            if (graceGrp) {
                LogPAE("Unclose grace group at the end of a measure", *token);
                if (m_pedanticMode) return false;
                token = m_pae.insert(token, pae::Token(pae::CONTAINER_END, -1, graceGrp));
                graceGrp = NULL;
            }
        }
        ++token;
    }

    return true;
}

bool PAEInput2::ConvertGrace()
{
    pae::Token *graceToken = NULL;
    bool isAcciaccatura = false;

    for (auto &token : m_pae) {
        if (token.IsVoid()) continue;

        if (this->Is(token, pae::GRACE)) {
            // Keep a flag for distinguishing them
            isAcciaccatura = (token.m_char == 'g');
            if (graceToken) {
                LogPAE("Invalid q after an unresolved q", token);
                if (m_pedanticMode) return false;
            }
            graceToken = &token;
            token.m_char = 0;
        }
        else if (graceToken) {
            // Having an accidental is fine
            if (this->Was(token, pae::ACCIDENTAL_INTERNAL)) {
                continue;
            }
            // Having a duration is fine for appogiatura
            if (this->Is(token, pae::DURATION)) {
                // For acciaccature, not in pedantic mode
                if (isAcciaccatura) {
                    LogPAE("Extraneous duration for acciaccatura g", token);
                    if (m_pedanticMode) return false;
                }
                continue;
            }
            if (token.Is(NOTE)) {
                Note *note = dynamic_cast<Note *>(token.m_object);
                assert(note);
                if (isAcciaccatura) {
                    note->SetDur(DURATION_8);
                    note->SetGrace(GRACE_unacc);
                }
                else {
                    note->SetGrace(GRACE_acc);
                }
                note->SetStemDir(STEMDIRECTION_up);
            }
            else {
                LogPAE("Grace q or g not followed by a note", token);
                if (m_pedanticMode) return false;
            }
            graceToken = NULL;
            isAcciaccatura = false;
        }
    }
    return true;
}

bool PAEInput2::CheckHierarchy()
{
    std::list<pae::Token *> stack;
    // A reference layer to test with
    Layer layer;
    pae::Token layerToken('_', -1, &layer);

    bool isValid = false;

    while (!isValid) {
        isValid = true;
        for (auto &token : m_pae) {
            if (token.IsVoid()) continue;

            if (!token.m_object) continue;

            if (token.m_object->Is(MEASURE)) {
                stack.clear();
                stack.push_back(&layerToken);
            }

            if (!token.m_object->IsLayerElement()) continue;

            // These will be added to a scoreDef
            if (token.m_object->Is({ KEYSIG, METERSIG, MENSUR })) continue;

            // Test is the element is supported by the current top container
            if (!token.IsContainerEnd() && !stack.back()->m_object->IsSupportedChild(token.m_object)) {
                LogPAE(StringFormat("Invalid %s within %s", token.GetName().c_str(), stack.back()->GetName().c_str()),
                    token);
                if (m_pedanticMode) return false;
                // Indicate that the data was not valid in this pass so we will check it again
                isValid = false;
                // Remove it and continue (do not add it to the stack anymore)
                this->RemoveContainerToken(token.m_object);
                continue;
            }

            // Add to the stack the layer element that are containers
            if (token.m_object->Is({ BEAM, GRACEGRP })) {
                // Begining of a container - simply push it to the stack
                if (token.m_char != pae::CONTAINER_END) {
                    stack.push_back(&token);
                }
                // End of a container - check for staggered opening and closing tags
                else {
                    // The object is not the same on top of the stack and the one we are popping - the hierarchy is
                    // staggerred
                    if (stack.back()->m_object != token.m_object) {
                        LogPAE(StringFormat("Staggered %s / %s opening and closing tags", token.GetName().c_str(),
                                   stack.back()->GetName().c_str()),
                            token);
                        if (m_pedanticMode) return false;
                        // Indicate that the data was not valid in this pass so we will check it again
                        isValid = false;
                        // If we want ot continue, we should remove the  last one added from the tokens
                        this->RemoveContainerToken(stack.back()->m_object);
                        stack.pop_back();
                        // We should also remove from the stack the object we were expecting
                        auto it = std::remove_if(stack.begin(), stack.end(),
                            [&token](const pae::Token *tokenIt) { return (tokenIt->m_object == token.m_object); });
                        stack.erase(it, stack.end());
                    }
                    // This is all good, simply pop it
                    else {
                        stack.pop_back();
                    }
                }
            }
        }
    }

    return true;
}

void PAEInput2::RemoveContainerToken(Object *object)
{
    bool deleted = false;
    for (auto &token : m_pae) {
        if (token.IsVoid() || !token.m_object) continue;

        if (token.m_object == object) {
            if (!token.IsContainerEnd()) {
                // Make sure we delete it only once - even though it should never be there more than once
                LogDebug("Deleting %s", object->GetClassName().c_str());
                if (!deleted) delete token.m_object;
                deleted = true;
            }
            token.m_char = 0;
            token.m_object = NULL;
        }
    }
}

bool PAEInput2::ParseKeySig(KeySig *keySig, const std::string &paeStr, pae::Token &token)
{
    assert(keySig);

    keySig->Reset();

    int altNumber = 0;
    bool endOfKeysig = false;
    bool enclosed = false;
    bool hasEnclosed = false;
    std::vector<bool> enclosedAccids;
    enclosedAccids.resize(7);
    bool cancel = false;
    data_ACCIDENTAL_WRITTEN alterationType = ACCIDENTAL_WRITTEN_NONE;
    for (auto c : paeStr) {
        switch (c) {
            case 'b':
                altNumber = 0;
                alterationType = ACCIDENTAL_WRITTEN_f;
                break;
            case 'x':
                altNumber = 0;
                alterationType = ACCIDENTAL_WRITTEN_s;
                break;
            case 'n':
                altNumber = 0;
                cancel = true;
                break;
            case '[':
                enclosed = true;
                hasEnclosed = true;
                break;
            case ']': enclosed = false; break;
            case 'F':
            case 'C':
            case 'G':
            case 'D':
            case 'A':
            case 'E':
            case 'B': altNumber++; break;
            default: endOfKeysig = true; break;
        }
        if (!endOfKeysig) {
            if (altNumber < 7) {
                enclosedAccids.at(altNumber) = enclosed;
            }
        }
    }

    // Just in case
    altNumber = std::min(6, altNumber);

    if (alterationType != ACCIDENTAL_WRITTEN_NONE) {
        if (hasEnclosed == true) {
            keySig->IsAttribute(false);
            for (int i = 0; i < altNumber; ++i) {
                KeyAccid *keyAccid = new KeyAccid();
                data_PITCHNAME pname = (alterationType == ACCIDENTAL_WRITTEN_f) ? KeySig::s_pnameForFlats[i]
                                                                                : KeySig::s_pnameForSharps[i];
                keyAccid->SetPname(pname);
                keyAccid->SetAccid(alterationType);
                keySig->AddChild(keyAccid);
                if (enclosedAccids.at(i)) {
                    keyAccid->SetEnclose(ENCLOSURE_brack);
                }
            }
        }
        else {
            keySig->SetSig(std::make_pair(altNumber, alterationType));
        }
        if (cancel) {
            keySig->SetSigShowchange(BOOLEAN_true);
        }
    }
    else {
        keySig->SetSig(std::make_pair(0, ACCIDENTAL_WRITTEN_n));
    }
    return true;
}

bool PAEInput2::ParseClef(Clef *clef, const std::string &paeStr, pae::Token &token)
{
    assert(clef);

    clef->Reset();

    if (paeStr.size() < 3) {
        LogPAE("Clef content cannot be parsed (G-2 in non pedantic mode)", token);
        if (m_pedanticMode) return false;
        clef->SetLine(2);
        clef->SetShape(CLEFSHAPE_G);
        return true;
    }

    char clefShape = paeStr.at(0);
    bool isMensural = (paeStr.at(1) == '+');
    char clefLine = paeStr.at(2);

    if (clefShape == 'G') {
        clef->SetShape(CLEFSHAPE_G);
        clef->SetLine(clefLine - 48);
    }
    else if (clefShape == 'C') {
        clef->SetShape(CLEFSHAPE_C);
        clef->SetLine(clefLine - 48);
    }
    else if (clefShape == 'F') {
        clef->SetShape(CLEFSHAPE_F);
        clef->SetLine(clefLine - 48);
    }
    else if (clefShape == 'g') {
        clef->SetShape(CLEFSHAPE_G);
        clef->SetLine(clefLine - 48);
        clef->SetDis(OCTAVE_DIS_8);
        clef->SetDisPlace(STAFFREL_basic_below);
    }
    else {
        LogPAE(StringFormat("Undefined clef '%s'", paeStr.c_str()), token);
        if (m_pedanticMode) return false;
    }
    return true;
}

bool PAEInput2::ParseMeterSig(MeterSig *meterSig, const std::string &paeStr, pae::Token &token)
{
    assert(meterSig);

    meterSig->Reset();

    if (paeStr.size() < 1) {
        LogPAE("MeterSig content cannot be parsed (4/4 in non pedantic mode)", token);
        if (m_pedanticMode) return false;
        meterSig->SetCount({ 4 });
        meterSig->SetUnit(4);
        return true;
    }

    std::cmatch matches;
    if (regex_match(paeStr.c_str(), matches, std::regex("(\\d+)/(\\d+)"))) {
        meterSig->SetCount({ std::stoi(matches[1]) });
        meterSig->SetUnit(std::stoi(matches[2]));
    }
    else if (regex_match(paeStr.c_str(), matches, std::regex("\\d+"))) {
        meterSig->SetCount({ std::stoi(paeStr.c_str()) });
        meterSig->SetUnit(1);
        meterSig->SetForm(METERFORM_num);
    }
    else if (paeStr == "c") {
        // C
        meterSig->SetSym(METERSIGN_common);
    }
    else if (paeStr == "c/") {
        // C|
        meterSig->SetSym(METERSIGN_cut);
    }
    else if (paeStr == "c3") {
        // C3
        meterSig->SetSym(METERSIGN_common);
        meterSig->SetCount({ 3 });
    }
    else if (paeStr == "c3/2") {
        // C3/2
        meterSig->SetSym(METERSIGN_common); // ??
        meterSig->SetCount({ 3 });
        meterSig->SetUnit(2);
    }
    else {
        LogPAE(StringFormat("Unsupported time signature %s", paeStr.c_str()), token);
        if (m_pedanticMode) return false;
    }
    return true;
}

bool PAEInput2::ParseMensur(Mensur *mensur, const std::string &paeStr, pae::Token &token)
{
    assert(mensur);

    mensur->Reset();

    if (paeStr.size() < 1) {
        LogPAE("Mensur content cannot be parsed (O in non pedantic mode", token);
        if (m_pedanticMode) return false;
        mensur->SetSign(MENSURATIONSIGN_O);
        return true;
    }

    std::cmatch matches;
    if (regex_match(paeStr.c_str(), matches, std::regex("(\\d+)/(\\d+)"))) {
        mensur->SetNum(std::stoi(matches[1]));
        mensur->SetNumbase(std::stoi(matches[2]));
    }
    else if (regex_match(paeStr.c_str(), matches, std::regex("\\d+"))) {
        mensur->SetNum(std::stoi(paeStr.c_str()));
    }
    else if (regex_match(paeStr.c_str(), matches, std::regex("([co])([\\./]?)([\\./]?)(\\d*)/?(\\d*)"))) {
        // C
        if (matches[1] == "c") {
            mensur->SetSign(MENSURATIONSIGN_C);
        }
        // O
        else {
            mensur->SetSign(MENSURATIONSIGN_O);
        }
        // Dot (second or third match since order between . and / is not defined in PAE)
        if ((matches[2] == ".") || (matches[3] == ".")) {
            mensur->SetDot(BOOLEAN_true);
        }
        // Slash (second or third match, ditto)
        if ((matches[2] == "/") || (matches[3] == "/")) {
            mensur->SetSlash(1);
        }
        // Num
        if (matches[4] != "") {
            mensur->SetNum(std::stoi(matches[4]));
        }
        // Numbase (but only if Num is given)
        if ((matches[4] != "") && (matches[5] != "")) {
            mensur->SetNumbase(std::stoi(matches[5]));
        }
    }
    else {
        LogPAE(StringFormat("Unsupported time signature: %s", paeStr.c_str()), token);
        if (m_pedanticMode) return false;
    }
    return true;
}

bool PAEInput2::ParseMeasure(Measure *measure, const std::string &paeStr, pae::Token &token)
{
    assert(measure);

    if (paeStr == "/") {
        measure->SetRight(BARRENDITION_single);
    }
    else if (paeStr == "//") {
        measure->SetRight(BARRENDITION_dbl);
    }
    else if (paeStr == "://") {
        measure->SetRight(BARRENDITION_rptend);
    }
    else if (paeStr == "//:") {
        measure->SetRight(BARRENDITION_rptstart);
    }
    else if (paeStr == "://:") {
        measure->SetRight(BARRENDITION_rptboth);
    }
    else {
        LogPAE(StringFormat("Unsupported barline: %s", paeStr.c_str()), token);
        if (m_pedanticMode) return false;
        // Put a single line by default in non pedantic mode
        measure->SetRight(BARRENDITION_single);
    }

    return true;
}

} // namespace vrv
