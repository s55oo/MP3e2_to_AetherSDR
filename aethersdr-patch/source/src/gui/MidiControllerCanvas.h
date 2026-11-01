#pragma once

#ifdef HAVE_MIDI

#include "core/MidiControllerLayout.h"

#include <QPixmap>
#include <QTimer>
#include <QWidget>

#include <functional>

namespace AetherSDR {

// Draws a MIDI controller's surface from a MidiControllerLayout and lets the
// operator click a control. Each control shows the function it is bound to
// just above it, so the whole mapping reads at a glance. An optional photo of
// the controller can sit behind the drawing.
//
// The canvas owns no binding state: the dialog supplies a label provider and
// reacts to controlClicked().
class MidiControllerCanvas : public QWidget {
    Q_OBJECT

public:
    explicit MidiControllerCanvas(QWidget* parent = nullptr);

    // Non-owning; the caller keeps the layout alive while it is set.
    void setControllerLayout(const MidiControllerLayout* layout);

    // Visible bank of a banked layout (e.g. the MP3 e2 Loops/Fx keys).
    void    setBank(const QString& bank);
    QString bank() const { return m_bank; }

    void    setSelectedControl(const QString& id);
    QString selectedControl() const { return m_selected; }

    // Returns the bound function's name for a control, or an empty string
    // when nothing is bound.
    using LabelProvider = std::function<QString(const MidiLayoutControl&)>;
    void setLabelProvider(LabelProvider provider);

    void setPhoto(const QPixmap& photo);
    bool hasPhoto() const { return !m_photo.isNull(); }

    // Briefly rings a control, e.g. when the hardware sends its message.
    void flashControl(const QString& id);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void controlClicked(const QString& id);
    void bankSwitchClicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    QTransform toWidget() const;
    QPointF    toLayout(const QPointF& widgetPoint) const;
    bool       isInCurrentBank(const MidiLayoutControl& c) const;
    QString    controlAt(const QPointF& widgetPoint) const;
    int        bankSwitchAt(const QPointF& widgetPoint) const;
    QVector<const MidiLayoutControl*> visibleControls() const;
    void       moveSelection(int step);
    void       updateAccessibleDescription();

    void paintControl(QPainter& p, const MidiLayoutControl& c, bool assigned) const;
    void paintLabel(QPainter& p, const MidiLayoutControl& c, const QString& text) const;
    void paintText(QPainter& p, const QPointF& baseline, const QString& text,
                   const QFont& font, const QColor& color, bool halo) const;

    const MidiControllerLayout* m_layout{nullptr};
    LabelProvider m_labels;
    QString m_bank;
    QString m_selected;
    QString m_hover;
    QString m_flash;
    QPixmap m_photo;
    QTimer  m_flashTimer;
};

} // namespace AetherSDR

#endif // HAVE_MIDI
