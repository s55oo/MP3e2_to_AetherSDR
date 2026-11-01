#pragma once

#include <QByteArray>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QString>
#include <QStringList>
#include <QVector>

namespace AetherSDR {

// A picture of a MIDI controller's surface: where each knob, fader, wheel and
// button sits, and which MIDI message it sends. The visual MIDI mapper draws
// this and lets the operator click a control to bind it. Layouts are data
// (JSON), so supporting another controller means adding a file, not code.
//
// File format (format "aethersdr-midi-layout", version 1):
//   { "format": "aethersdr-midi-layout", "version": 1,
//     "id": "vendor-model", "name": "Vendor Model",
//     "width": 1300, "height": 900,              // layout coordinate space
//     "banks": [ {"id": "loop", "label": "..."} ],            // optional
//     "bankSwitches": [ {"x":..,"y":..,"w":..,"h":..,"label":".."} ], // optional
//     "sections": [ {"x":..,"y":..,"w":..,"h":..} ],          // optional outlines
//     "controls": [
//       { "id": "a.jog", "name": "Deck A Jog wheel", "group": "Deck A",
//         "kind": "button" | "absolute" | "encoder",
//         "message": "note" | "cc", "number": 0-127,
//         "shape": "key" | "pill" | "knob" | "jog" | "fader" | "crossfader",
//         "x": .., "y": ..,            // centre (fader: top of the track)
//         "w": .., "h": ..,            // key, pill
//         "r": ..,                     // knob, jog
//         "length": ..,                // fader, crossfader
//         "label": "..", "caption": "..", "bank": "loop" } ] }
//
// Every row is validated on load (Principle: boundary input validation); a
// file with any bad row is refused as a whole with the reason named.
struct MidiLayoutControl {
    enum class Kind { Button, Absolute, Encoder };
    enum class Message { Note, ControlChange };
    enum class Shape { Key, Pill, Knob, Jog, Fader, Crossfader };

    QString id;
    QString name;
    QString group;
    QString label;     // text drawn on the control itself
    QString caption;   // small text drawn under knobs and faders
    QString bank;      // empty = always visible
    Kind    kind{Kind::Button};
    Message message{Message::Note};
    Shape   shape{Shape::Pill};
    int     number{0};
    QPointF center;    // fader: top centre of the track
    QSizeF  size;      // key, pill
    double  radius{0}; // knob, jog
    double  length{0}; // fader, crossfader

    // Area that accepts a click, in layout coordinates.
    QRectF hitRect() const;
    bool   contains(const QPointF& layoutPoint) const;
    // Top centre of the space just above the control, where its binding
    // label is drawn.
    QPointF labelAnchor() const;
};

struct MidiLayoutBank {
    QString id;
    QString label;
};

struct MidiControllerLayout {
    QString id;
    QString name;
    QString notes;
    QSizeF  size;
    QVector<MidiLayoutBank>    banks;
    QVector<QRectF>            bankSwitches;
    QStringList                bankSwitchLabels;
    QVector<QRectF>            sections;
    QVector<MidiLayoutControl> controls;

    const MidiLayoutControl* findById(const QString& id) const;
    const MidiLayoutControl* findBySource(MidiLayoutControl::Message message,
                                          int number) const;
};

struct MidiLayoutLoadResult {
    MidiControllerLayout layout;
    QString error;
    bool ok() const { return error.isEmpty(); }
};

MidiLayoutLoadResult parseMidiControllerLayout(const QByteArray& json);
MidiLayoutLoadResult loadMidiControllerLayout(const QString& filePath);

// Layouts shipped in the Qt resource bundle (":/midi-layouts/*.json") plus any
// the operator dropped into <MIDI profile dir>/layouts/. Returns file paths.
QStringList availableMidiControllerLayoutFiles();

// Where operator-supplied layouts and controller photos live.
QString midiLayoutUserDir();

} // namespace AetherSDR
