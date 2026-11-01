// Visual MIDI mapper layouts: the shipped MP3 e2 layout loads and matches the
// controller's MIDI map, and malformed files are refused with a reason.

#include "core/MidiControllerLayout.h"

#include <QSet>

#include <iostream>

using namespace AetherSDR;

namespace {

bool expect(bool condition, const char* label)
{
    std::cout << (condition ? "[ OK ] " : "[FAIL] ") << label << '\n';
    return condition;
}

QByteArray minimalLayout(const QByteArray& controls)
{
    return QByteArray(R"({"format":"aethersdr-midi-layout","version":1,"id":"t","name":"T",)"
                      R"("width":100,"height":100,"controls":)") + controls + "}";
}

} // namespace

int main()
{
    bool ok = true;

    // ── Shipped MP3 e2 layout ───────────────────────────────────────────
    const MidiLayoutLoadResult mp3 = loadMidiControllerLayout(QStringLiteral(AETHER_MP3E2_LAYOUT));
    ok &= expect(mp3.ok(), "MP3 e2 layout loads");
    if (!mp3.ok()) std::cout << "       " << mp3.error.toStdString() << '\n';
    const MidiControllerLayout& L = mp3.layout;
    ok &= expect(L.controls.size() == 55, "MP3 e2 layout has 55 controls");
    ok &= expect(L.banks.size() == 2 && L.bankSwitches.size() == 2, "two key banks, two Loops/Fx switches");

    // Spot-check the MIDI map (Mixxx MP3 e2 mapping, channel 1).
    using M = MidiLayoutControl::Message;
    const auto* jogA = L.findBySource(M::ControlChange, 0x30);
    ok &= expect(jogA && jogA->kind == MidiLayoutControl::Kind::Encoder, "CC 0x30 is the Deck A jog wheel, an encoder");
    const auto* xfade = L.findBySource(M::ControlChange, 0x38);
    ok &= expect(xfade && xfade->kind == MidiLayoutControl::Kind::Absolute, "CC 0x38 is the crossfader");
    const auto* playB = L.findBySource(M::Note, 0x23);
    ok &= expect(playB && playB->id == QLatin1String("b.play"), "note 0x23 is Deck B Play");
    const auto* key5 = L.findBySource(M::Note, 0x05);
    ok &= expect(key5 && key5->bank == QLatin1String("fx"), "note 0x05 is a key in the Fx bank");

    // Hit testing: the centre of each visible control selects that control.
    int hits = 0;
    for (const auto& c : L.controls)
        if (c.contains(c.center)) ++hits;
    ok &= expect(hits == L.controls.size(), "every control contains its own centre");

    // ── Refusals ────────────────────────────────────────────────────────
    const QByteArray key = R"({"id":"k","kind":"button","message":"note","number":1,"shape":"key","x":10,"y":10,"w":5,"h":5})";
    ok &= expect(parseMidiControllerLayout(minimalLayout("[" + key + "]")).ok(), "minimal layout accepted");
    ok &= expect(!parseMidiControllerLayout("not json").ok(), "garbage refused");
    ok &= expect(!parseMidiControllerLayout(minimalLayout("[]")).ok(), "layout without controls refused");
    ok &= expect(!parseMidiControllerLayout(minimalLayout("[" + key + "," + key + "]")).ok(),
                 "duplicate id refused");
    QByteArray badNumber = key;
    badNumber.replace("\"number\":1", "\"number\":128");
    ok &= expect(!parseMidiControllerLayout(minimalLayout("[" + badNumber + "]")).ok(), "number 128 refused");
    QByteArray noteKnob = R"({"id":"n","kind":"absolute","message":"note","number":2,"shape":"knob","x":10,"y":10,"r":5})";
    ok &= expect(!parseMidiControllerLayout(minimalLayout("[" + noteKnob + "]")).ok(), "a knob sending notes refused");
    QByteArray outside = key;
    outside.replace("\"x\":10", "\"x\":500");
    ok &= expect(!parseMidiControllerLayout(minimalLayout("[" + outside + "]")).ok(), "control outside the layout refused");
    QByteArray badBank = key;
    badBank.replace("\"shape\"", "\"bank\":\"nope\",\"shape\"");
    ok &= expect(!parseMidiControllerLayout(minimalLayout("[" + badBank + "]")).ok(), "unknown bank refused");

    return ok ? 0 : 1;
}
