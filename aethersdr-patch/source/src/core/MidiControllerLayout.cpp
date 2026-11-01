#include "MidiControllerLayout.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>
#include <QStandardPaths>

#include <cmath>

namespace AetherSDR {

namespace {

constexpr double kMaxLayoutExtent = 10000.0;

bool finitePositive(double v) { return std::isfinite(v) && v > 0.0 && v <= kMaxLayoutExtent; }
bool finiteInRange(double v)  { return std::isfinite(v) && v >= 0.0 && v <= kMaxLayoutExtent; }

// Reads a centred rectangle {x, y, w, h} (x/y = centre).
bool readCentredRect(const QJsonObject& o, QRectF* out)
{
    const double x = o.value("x").toDouble(-1), y = o.value("y").toDouble(-1);
    const double w = o.value("w").toDouble(-1), h = o.value("h").toDouble(-1);
    if (!finiteInRange(x) || !finiteInRange(y) || !finitePositive(w) || !finitePositive(h))
        return false;
    *out = QRectF(x - w / 2.0, y - h / 2.0, w, h);
    return true;
}

QString rowName(int index, const QJsonObject& o)
{
    const QString id = o.value("id").toString();
    return id.isEmpty() ? QStringLiteral("control #%1").arg(index + 1)
                        : QStringLiteral("control \"%1\"").arg(id);
}

} // namespace

// ── MidiLayoutControl ───────────────────────────────────────────────────────

QRectF MidiLayoutControl::hitRect() const
{
    switch (shape) {
    case Shape::Key:
    case Shape::Pill:
        return QRectF(center.x() - size.width() / 2.0, center.y() - size.height() / 2.0,
                      size.width(), size.height());
    case Shape::Knob:
    case Shape::Jog:
        return QRectF(center.x() - radius, center.y() - radius, 2 * radius, 2 * radius);
    case Shape::Fader:
        return QRectF(center.x() - 24, center.y() - 8, 48, length + 16);
    case Shape::Crossfader:
        return QRectF(center.x() - length / 2.0 - 8, center.y() - 24, length + 16, 48);
    }
    return {};
}

bool MidiLayoutControl::contains(const QPointF& p) const
{
    if (shape == Shape::Knob || shape == Shape::Jog) {
        const QPointF d = p - center;
        return d.x() * d.x() + d.y() * d.y() <= radius * radius;
    }
    return hitRect().contains(p);
}

QPointF MidiLayoutControl::labelAnchor() const
{
    const QRectF r = hitRect();
    return QPointF(r.center().x(), r.top());
}

// ── MidiControllerLayout ────────────────────────────────────────────────────

const MidiLayoutControl* MidiControllerLayout::findById(const QString& wanted) const
{
    for (const auto& c : controls)
        if (c.id == wanted) return &c;
    return nullptr;
}

const MidiLayoutControl* MidiControllerLayout::findBySource(
    MidiLayoutControl::Message message, int number) const
{
    for (const auto& c : controls)
        if (c.message == message && c.number == number) return &c;
    return nullptr;
}

// ── Parsing ─────────────────────────────────────────────────────────────────

MidiLayoutLoadResult parseMidiControllerLayout(const QByteArray& json)
{
    MidiLayoutLoadResult result;
    auto fail = [&result](const QString& why) {
        result.error = why;
        result.layout = {};
        return result;
    };

    QJsonParseError perr{};
    const QJsonDocument doc = QJsonDocument::fromJson(json, &perr);
    if (perr.error != QJsonParseError::NoError)
        return fail(QStringLiteral("Not valid JSON: %1 (offset %2).")
                        .arg(perr.errorString()).arg(perr.offset));
    if (!doc.isObject())
        return fail(QStringLiteral("The file must contain one JSON object."));

    const QJsonObject root = doc.object();
    if (root.value("format").toString() != QLatin1String("aethersdr-midi-layout"))
        return fail(QStringLiteral("Not an AetherSDR MIDI layout (\"format\" must be "
                                   "\"aethersdr-midi-layout\")."));
    if (root.value("version").toInt() != 1)
        return fail(QStringLiteral("Unsupported layout version %1; this AetherSDR reads version 1.")
                        .arg(root.value("version").toInt()));

    MidiControllerLayout& L = result.layout;
    L.id = root.value("id").toString().trimmed();
    L.name = root.value("name").toString().trimmed();
    L.notes = root.value("notes").toString();
    if (L.id.isEmpty() || L.id.contains('/') || L.id.contains('\\') || L.id.startsWith('.'))
        return fail(QStringLiteral("The layout needs a plain \"id\" (no slashes, no leading dot)."));
    if (L.name.isEmpty()) L.name = L.id;

    const double width = root.value("width").toDouble(-1);
    const double height = root.value("height").toDouble(-1);
    if (!finitePositive(width) || !finitePositive(height))
        return fail(QStringLiteral("\"width\" and \"height\" must be positive numbers up to %1.")
                        .arg(kMaxLayoutExtent));
    L.size = QSizeF(width, height);

    QSet<QString> bankIds;
    for (const auto& v : root.value("banks").toArray()) {
        const QJsonObject b = v.toObject();
        MidiLayoutBank bank{b.value("id").toString().trimmed(), b.value("label").toString()};
        if (bank.id.isEmpty() || bankIds.contains(bank.id))
            return fail(QStringLiteral("Every bank needs a unique \"id\"."));
        if (bank.label.isEmpty()) bank.label = bank.id;
        bankIds.insert(bank.id);
        L.banks.append(bank);
    }

    for (const auto& v : root.value("bankSwitches").toArray()) {
        QRectF r;
        if (!readCentredRect(v.toObject(), &r))
            return fail(QStringLiteral("A bank switch has a missing or bad x/y/w/h."));
        L.bankSwitches.append(r);
        L.bankSwitchLabels.append(v.toObject().value("label").toString());
    }
    if (!L.bankSwitches.isEmpty() && L.banks.size() < 2)
        return fail(QStringLiteral("Bank switches need at least two banks to switch between."));

    for (const auto& v : root.value("sections").toArray()) {
        QRectF r;
        if (!readCentredRect(v.toObject(), &r))
            return fail(QStringLiteral("A section outline has a missing or bad x/y/w/h."));
        L.sections.append(r);
    }

    static const QHash<QString, MidiLayoutControl::Kind> kKinds{
        {"button", MidiLayoutControl::Kind::Button},
        {"absolute", MidiLayoutControl::Kind::Absolute},
        {"encoder", MidiLayoutControl::Kind::Encoder}};
    static const QHash<QString, MidiLayoutControl::Shape> kShapes{
        {"key", MidiLayoutControl::Shape::Key},
        {"pill", MidiLayoutControl::Shape::Pill},
        {"knob", MidiLayoutControl::Shape::Knob},
        {"jog", MidiLayoutControl::Shape::Jog},
        {"fader", MidiLayoutControl::Shape::Fader},
        {"crossfader", MidiLayoutControl::Shape::Crossfader}};

    const QJsonArray rows = root.value("controls").toArray();
    if (rows.isEmpty())
        return fail(QStringLiteral("The layout has no \"controls\"."));

    QSet<QString> ids;
    QSet<int> sources;  // (message << 8) | number
    for (int i = 0; i < rows.size(); ++i) {
        const QJsonObject o = rows.at(i).toObject();
        const QString where = rowName(i, o);
        MidiLayoutControl c;
        c.id = o.value("id").toString().trimmed();
        if (c.id.isEmpty() || ids.contains(c.id))
            return fail(QStringLiteral("%1: every control needs a unique \"id\".").arg(where));
        ids.insert(c.id);
        c.name = o.value("name").toString().trimmed();
        if (c.name.isEmpty()) c.name = c.id;
        c.group = o.value("group").toString().trimmed();
        c.label = o.value("label").toString();
        c.caption = o.value("caption").toString();
        c.bank = o.value("bank").toString().trimmed();
        if (!c.bank.isEmpty() && !bankIds.contains(c.bank))
            return fail(QStringLiteral("%1: bank \"%2\" is not listed in \"banks\".").arg(where, c.bank));

        const QString kind = o.value("kind").toString();
        if (!kKinds.contains(kind))
            return fail(QStringLiteral("%1: \"kind\" must be button, absolute or encoder.").arg(where));
        c.kind = kKinds.value(kind);

        const QString msg = o.value("message").toString();
        if (msg == QLatin1String("note")) c.message = MidiLayoutControl::Message::Note;
        else if (msg == QLatin1String("cc")) c.message = MidiLayoutControl::Message::ControlChange;
        else return fail(QStringLiteral("%1: \"message\" must be note or cc.").arg(where));
        if (c.kind != MidiLayoutControl::Kind::Button && c.message != MidiLayoutControl::Message::ControlChange)
            return fail(QStringLiteral("%1: knobs, faders and wheels must send cc.").arg(where));

        const QJsonValue num = o.value("number");
        if (!num.isDouble() || num.toDouble() != std::floor(num.toDouble())
            || num.toInt(-1) < 0 || num.toInt(-1) > 127)
            return fail(QStringLiteral("%1: \"number\" must be a whole number 0-127.").arg(where));
        c.number = num.toInt();
        const int sourceKey = (static_cast<int>(c.message) << 8) | c.number;
        if (sources.contains(sourceKey))
            return fail(QStringLiteral("%1: another control already sends %2 %3.")
                            .arg(where, msg == QLatin1String("cc") ? QStringLiteral("CC") : QStringLiteral("note"))
                            .arg(c.number));
        sources.insert(sourceKey);

        const QString shape = o.value("shape").toString();
        if (!kShapes.contains(shape))
            return fail(QStringLiteral("%1: unknown \"shape\" \"%2\".").arg(where, shape));
        c.shape = kShapes.value(shape);

        const double x = o.value("x").toDouble(-1), y = o.value("y").toDouble(-1);
        if (!finiteInRange(x) || !finiteInRange(y) || x > width || y > height)
            return fail(QStringLiteral("%1: x/y must lie inside the layout.").arg(where));
        c.center = QPointF(x, y);

        switch (c.shape) {
        case MidiLayoutControl::Shape::Key:
        case MidiLayoutControl::Shape::Pill: {
            const double w = o.value("w").toDouble(-1), h = o.value("h").toDouble(-1);
            if (!finitePositive(w) || !finitePositive(h))
                return fail(QStringLiteral("%1: a %2 needs positive \"w\" and \"h\".").arg(where, shape));
            c.size = QSizeF(w, h);
            break;
        }
        case MidiLayoutControl::Shape::Knob:
        case MidiLayoutControl::Shape::Jog:
            c.radius = o.value("r").toDouble(-1);
            if (!finitePositive(c.radius))
                return fail(QStringLiteral("%1: a %2 needs a positive \"r\".").arg(where, shape));
            break;
        case MidiLayoutControl::Shape::Fader:
        case MidiLayoutControl::Shape::Crossfader:
            c.length = o.value("length").toDouble(-1);
            if (!finitePositive(c.length))
                return fail(QStringLiteral("%1: a %2 needs a positive \"length\".").arg(where, shape));
            break;
        }
        L.controls.append(c);
    }
    return result;
}

MidiLayoutLoadResult loadMidiControllerLayout(const QString& filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        MidiLayoutLoadResult r;
        r.error = QStringLiteral("Couldn't open %1.").arg(QDir::toNativeSeparators(filePath));
        return r;
    }
    // Layout files are small; refuse anything absurd before parsing it.
    if (f.size() > 2 * 1024 * 1024) {
        MidiLayoutLoadResult r;
        r.error = QStringLiteral("%1 is larger than 2 MB; that isn't a layout file.")
                      .arg(QFileInfo(filePath).fileName());
        return r;
    }
    return parseMidiControllerLayout(f.readAll());
}

QString midiLayoutUserDir()
{
    // Same root as MidiSettings::profileDir(), kept here so libaethercore's
    // layout code doesn't depend on the RtMidi-guarded settings class.
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
           + QStringLiteral("/AetherSDR/midi/layouts");
}

QStringList availableMidiControllerLayoutFiles()
{
    QStringList files;
    const QDir builtIn(QStringLiteral(":/midi-layouts"));
    for (const auto& fi : builtIn.entryInfoList({QStringLiteral("*.json")}, QDir::Files, QDir::Name))
        files.append(fi.absoluteFilePath());
    const QDir user(midiLayoutUserDir());
    for (const auto& fi : user.entryInfoList({QStringLiteral("*.json")}, QDir::Files, QDir::Name))
        files.append(fi.absoluteFilePath());
    return files;
}

} // namespace AetherSDR
