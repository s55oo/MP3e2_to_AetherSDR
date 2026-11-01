#include "MidiControllerCanvas.h"

#ifdef HAVE_MIDI

#include "core/ThemeManager.h"

#include <QFontMetricsF>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <cmath>

namespace AetherSDR {

namespace {

// Text sizes in layout units; the whole drawing scales with the widget.
constexpr double kLabelPx   = 18.0;
constexpr double kInnerPx   = 17.0;
constexpr double kCaptionPx = 14.0;

QColor token(const QWidget* w, const char* name)
{
    return ThemeManager::instance().color(w, QString::fromLatin1(name));
}

} // namespace

MidiControllerCanvas::MidiControllerCanvas(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setObjectName(QStringLiteral("midiControllerCanvas"));
    setAccessibleName(QStringLiteral("Controller layout"));
    m_flashTimer.setSingleShot(true);
    m_flashTimer.setInterval(450);
    connect(&m_flashTimer, &QTimer::timeout, this, [this] { m_flash.clear(); update(); });
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, qOverload<>(&QWidget::update));
}

void MidiControllerCanvas::setControllerLayout(const MidiControllerLayout* layout)
{
    m_layout = layout;
    m_selected.clear();
    m_hover.clear();
    m_bank = (layout && !layout->banks.isEmpty()) ? layout->banks.first().id : QString();
    updateGeometry();
    update();
}

void MidiControllerCanvas::setBank(const QString& bank)
{
    if (m_bank == bank) return;
    m_bank = bank;
    update();
}

void MidiControllerCanvas::setSelectedControl(const QString& id)
{
    m_selected = id;
    updateAccessibleDescription();
    update();
}

void MidiControllerCanvas::setLabelProvider(LabelProvider provider)
{
    m_labels = std::move(provider);
    update();
}

void MidiControllerCanvas::setPhoto(const QPixmap& photo)
{
    m_photo = photo;
    update();
}

void MidiControllerCanvas::flashControl(const QString& id)
{
    m_flash = id;
    m_flashTimer.start();
    update();
}

QSize MidiControllerCanvas::sizeHint() const
{
    if (!m_layout) return {650, 450};
    return QSize(qRound(m_layout->size.width() * 0.6), qRound(m_layout->size.height() * 0.6));
}

QSize MidiControllerCanvas::minimumSizeHint() const
{
    return {420, 300};
}

// ── Geometry ────────────────────────────────────────────────────────────────

QTransform MidiControllerCanvas::toWidget() const
{
    if (!m_layout) return {};
    const double sx = width() / m_layout->size.width();
    const double sy = height() / m_layout->size.height();
    const double s = std::min(sx, sy);
    const double ox = (width() - m_layout->size.width() * s) / 2.0;
    const double oy = (height() - m_layout->size.height() * s) / 2.0;
    QTransform t;
    t.translate(ox, oy);
    t.scale(s, s);
    return t;
}

QPointF MidiControllerCanvas::toLayout(const QPointF& widgetPoint) const
{
    bool invertible = false;
    const QTransform inv = toWidget().inverted(&invertible);
    return invertible ? inv.map(widgetPoint) : QPointF(-1, -1);
}

bool MidiControllerCanvas::isInCurrentBank(const MidiLayoutControl& c) const
{
    return c.bank.isEmpty() || c.bank == m_bank;
}

QVector<const MidiLayoutControl*> MidiControllerCanvas::visibleControls() const
{
    QVector<const MidiLayoutControl*> out;
    if (!m_layout) return out;
    for (const auto& c : m_layout->controls)
        if (isInCurrentBank(c)) out.append(&c);
    return out;
}

QString MidiControllerCanvas::controlAt(const QPointF& widgetPoint) const
{
    const QPointF lp = toLayout(widgetPoint);
    // Small controls first, so a pill drawn over a large shape still wins.
    const MidiLayoutControl* best = nullptr;
    double bestArea = 0;
    for (const auto* c : visibleControls()) {
        if (!c->contains(lp)) continue;
        const QRectF r = c->hitRect();
        const double area = r.width() * r.height();
        if (!best || area < bestArea) { best = c; bestArea = area; }
    }
    return best ? best->id : QString();
}

int MidiControllerCanvas::bankSwitchAt(const QPointF& widgetPoint) const
{
    if (!m_layout) return -1;
    const QPointF lp = toLayout(widgetPoint);
    for (int i = 0; i < m_layout->bankSwitches.size(); ++i)
        if (m_layout->bankSwitches.at(i).contains(lp)) return i;
    return -1;
}

// ── Input ───────────────────────────────────────────────────────────────────

void MidiControllerCanvas::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) { QWidget::mousePressEvent(event); return; }
    if (bankSwitchAt(event->position()) >= 0) { emit bankSwitchClicked(); return; }
    const QString id = controlAt(event->position());
    if (id.isEmpty()) return;
    setFocus(Qt::MouseFocusReason);
    emit controlClicked(id);
}

void MidiControllerCanvas::mouseMoveEvent(QMouseEvent* event)
{
    const QString id = controlAt(event->position());
    const bool overSwitch = bankSwitchAt(event->position()) >= 0;
    setCursor(id.isEmpty() && !overSwitch ? Qt::ArrowCursor : Qt::PointingHandCursor);
    if (id != m_hover) {
        m_hover = id;
        const auto* c = m_layout ? m_layout->findById(id) : nullptr;
        setToolTip(c ? c->name : (overSwitch ? QStringLiteral("Switch key bank") : QString()));
        update();
    }
}

void MidiControllerCanvas::leaveEvent(QEvent* event)
{
    m_hover.clear();
    update();
    QWidget::leaveEvent(event);
}

void MidiControllerCanvas::moveSelection(int step)
{
    const auto controls = visibleControls();
    if (controls.isEmpty()) return;
    int at = -1;
    for (int i = 0; i < controls.size(); ++i)
        if (controls.at(i)->id == m_selected) at = i;
    const int n = controls.size();
    const int next = at < 0 ? 0 : ((at + step) % n + n) % n;
    emit controlClicked(controls.at(next)->id);
}

void MidiControllerCanvas::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Right:
    case Qt::Key_Down:
        moveSelection(+1);
        return;
    case Qt::Key_Left:
    case Qt::Key_Up:
        moveSelection(-1);
        return;
    case Qt::Key_B:
        emit bankSwitchClicked();
        return;
    default:
        QWidget::keyPressEvent(event);
    }
}

void MidiControllerCanvas::updateAccessibleDescription()
{
    const auto* c = m_layout ? m_layout->findById(m_selected) : nullptr;
    if (!c) { setAccessibleDescription(QString()); return; }
    const QString fn = m_labels ? m_labels(*c) : QString();
    setAccessibleDescription(QStringLiteral("%1: %2").arg(
        c->name, fn.isEmpty() ? QStringLiteral("not assigned") : fn));
}

// ── Painting ────────────────────────────────────────────────────────────────

void MidiControllerCanvas::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    p.fillRect(rect(), token(this, "color.background.0"));
    if (!m_layout) {
        p.setPen(token(this, "color.text.secondary"));
        p.drawText(rect(), Qt::AlignCenter, QStringLiteral("No controller layout loaded"));
        return;
    }

    p.setTransform(toWidget());
    const QRectF board(QPointF(0, 0), m_layout->size);
    const bool photo = !m_photo.isNull();

    // Board, then the operator's photo fitted inside it.
    QPainterPath boardPath;
    boardPath.addRoundedRect(board.adjusted(4, 4, -4, -4), 24, 24);
    p.fillPath(boardPath, token(this, "color.background.1"));
    if (photo) {
        QSizeF fit = m_photo.size().toSizeF();
        fit.scale(board.size(), Qt::KeepAspectRatio);
        const QRectF target(board.center().x() - fit.width() / 2.0,
                            board.center().y() - fit.height() / 2.0,
                            fit.width(), fit.height());
        p.save();
        p.setClipPath(boardPath);
        p.drawPixmap(target, m_photo, QRectF(m_photo.rect()));
        p.restore();
    } else {
        p.setPen(QPen(token(this, "color.border.strong"), 2));
        p.setBrush(Qt::NoBrush);
        p.drawPath(boardPath);
        for (const auto& s : m_layout->sections)
            p.drawRoundedRect(s, 12, 12);
    }

    // Bank switches (no MIDI of their own: they change what the keys send).
    QFont inner = font();
    inner.setPixelSize(qRound(kInnerPx));
    inner.setBold(true);
    for (int i = 0; i < m_layout->bankSwitches.size(); ++i) {
        const QRectF r = m_layout->bankSwitches.at(i);
        QPen pen(token(this, "color.text.secondary"), 1.5, Qt::DashLine);
        p.setPen(pen);
        p.setBrush(photo ? QBrush(theme::withAlpha(this, "color.background.0", 150)) : QBrush(Qt::NoBrush));
        p.drawRoundedRect(r, 6, 6);
        const QString text = m_layout->bankSwitchLabels.value(i);
        paintText(p, QPointF(r.center().x(), r.center().y() + kInnerPx * 0.35), text, inner,
                  token(this, "color.text.primary"), photo);
    }

    for (const auto& c : m_layout->controls) {
        if (!isInCurrentBank(c)) continue;
        const QString fn = m_labels ? m_labels(c) : QString();
        paintControl(p, c, !fn.isEmpty());
        paintLabel(p, c, fn);
    }
}

void MidiControllerCanvas::paintControl(QPainter& p, const MidiLayoutControl& c, bool assigned) const
{
    const bool photo = !m_photo.isNull();
    QColor fill = assigned ? token(this, "color.accent") : token(this, "color.background.2");
    if (photo) fill.setAlpha(assigned ? 120 : 70);

    QColor edge = token(this, "color.border.strong");
    double edgeWidth = 1.5;
    if (photo) { edge = token(this, "color.text.primary"); edgeWidth = 2; }
    if (c.id == m_hover) { edge = token(this, "color.text.secondary"); edgeWidth = 2.5; }
    if (c.id == m_selected) { edge = token(this, "color.accent.warning"); edgeWidth = 4; }
    if (c.id == m_flash) { edge = token(this, "color.accent.success"); edgeWidth = 5; }
    p.setPen(QPen(edge, edgeWidth));
    p.setBrush(fill);

    QFont inner = font();
    inner.setPixelSize(qRound(kInnerPx));
    inner.setBold(true);
    QFont caption = font();
    caption.setPixelSize(qRound(kCaptionPx));
    const QColor innerColor = assigned && !photo ? token(this, "color.background.0")
                                                 : token(this, "color.text.primary");

    switch (c.shape) {
    case MidiLayoutControl::Shape::Key:
    case MidiLayoutControl::Shape::Pill: {
        const QRectF r = c.hitRect();
        p.drawRoundedRect(r, 7, 7);
        paintText(p, QPointF(r.center().x(), r.center().y() + kInnerPx * 0.35), c.label, inner,
                  innerColor, photo);
        break;
    }
    case MidiLayoutControl::Shape::Knob: {
        p.drawEllipse(c.center, c.radius, c.radius);
        p.setPen(QPen(token(this, "color.text.primary"), 3, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(c.center.x(), c.center.y() - c.radius + 5),
                   QPointF(c.center.x(), c.center.y() - c.radius / 3.0));
        if (!c.caption.isEmpty())
            paintText(p, QPointF(c.center.x(), c.center.y() + c.radius + kCaptionPx + 4), c.caption,
                      caption, token(this, "color.text.label"), photo);
        break;
    }
    case MidiLayoutControl::Shape::Jog: {
        p.drawEllipse(c.center, c.radius, c.radius);
        if (!photo) {
            p.setPen(QPen(token(this, "color.border.strong"), 1.5));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(c.center, c.radius * 0.62, c.radius * 0.62);
        }
        paintText(p, QPointF(c.center.x(), c.center.y() + c.radius * 0.84), QStringLiteral("JOG"),
                  inner, innerColor, photo);
        break;
    }
    case MidiLayoutControl::Shape::Fader: {
        if (!photo) {
            p.save();
            p.setPen(Qt::NoPen);
            p.setBrush(token(this, "color.background.0"));
            p.drawRoundedRect(QRectF(c.center.x() - 3, c.center.y(), 6, c.length), 3, 3);
            p.restore();
        }
        p.drawRoundedRect(QRectF(c.center.x() - 22, c.center.y() + c.length * 0.45 - 14, 44, 28), 4, 4);
        if (!c.caption.isEmpty())
            paintText(p, QPointF(c.center.x(), c.center.y() + c.length + 24), c.caption, caption,
                      token(this, "color.text.label"), photo);
        break;
    }
    case MidiLayoutControl::Shape::Crossfader: {
        if (!photo) {
            p.save();
            p.setPen(Qt::NoPen);
            p.setBrush(token(this, "color.background.0"));
            p.drawRoundedRect(QRectF(c.center.x() - c.length / 2.0, c.center.y() - 3, c.length, 6), 3, 3);
            p.restore();
        }
        p.drawRoundedRect(QRectF(c.center.x() - 14, c.center.y() - 22, 28, 44), 4, 4);
        break;
    }
    }
}

void MidiControllerCanvas::paintLabel(QPainter& p, const MidiLayoutControl& c, const QString& text) const
{
    QFont f = font();
    f.setPixelSize(qRound(kLabelPx));
    f.setBold(!text.isEmpty());
    const QRectF hit = c.hitRect();
    // Room for the label: a little wider than the control, never narrower
    // than a short word, so neighbouring labels don't run together.
    const double room = std::max(hit.width() + 26.0, 84.0);
    const QString shown = text.isEmpty() ? QStringLiteral("—")
                                         : QFontMetricsF(f).elidedText(text, Qt::ElideRight, room);
    QColor color = text.isEmpty() ? token(this, "color.text.label") : token(this, "color.accent.bright");
    if (c.id == m_selected) color = token(this, "color.accent.warning");
    const double gap = c.shape == MidiLayoutControl::Shape::Jog ? 12.0 : 9.0;
    paintText(p, QPointF(hit.center().x(), hit.top() - gap), shown, f, color, !m_photo.isNull());
}

void MidiControllerCanvas::paintText(QPainter& p, const QPointF& baseline, const QString& text,
                                     const QFont& font, const QColor& color, bool halo) const
{
    if (text.isEmpty()) return;
    const double w = QFontMetricsF(font).horizontalAdvance(text);
    const QPointF origin(baseline.x() - w / 2.0, baseline.y());
    if (halo) {
        // Over a photo, outline the text so it reads on any background.
        QPainterPath path;
        path.addText(origin, font, text);
        p.save();
        p.setPen(QPen(theme::withAlpha(this, "color.background.0", 220), 4.0,
                      Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);
        p.fillPath(path, color);
        p.restore();
        return;
    }
    p.save();
    p.setFont(font);
    p.setPen(color);
    p.drawText(origin, text);
    p.restore();
}

} // namespace AetherSDR

#endif // HAVE_MIDI
