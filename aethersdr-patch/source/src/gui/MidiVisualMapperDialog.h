#pragma once

#ifdef HAVE_MIDI

#include "PersistentDialog.h"
#include "core/MidiControllerLayout.h"

#include <QVector>

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;

namespace AetherSDR {

class MidiControlManager;
class MidiControllerCanvas;
struct MidiBinding;
struct MidiParam;

// Visual MIDI mapper: shows a picture of the controller with the function
// bound to every knob, fader, wheel and button written above it. Click a
// control (or touch it on the hardware) and pick its function from the list.
// Changes go straight into the live bindings and midi.settings, the same
// store the MIDI Mapping table edits, so the two views always agree.
// Opened from the MIDI Mapping dialog's "Visual Mapper…" button.
class MidiVisualMapperDialog : public PersistentDialog {
    Q_OBJECT

public:
    explicit MidiVisualMapperDialog(MidiControlManager* manager, QWidget* parent = nullptr);

    // Re-read the bindings after another view changed them.
    void refresh();

signals:
    // Emitted after this window changed and saved the bindings.
    void bindingsChanged();

private:
    void populateLayouts();
    void showLayout(int comboIndex);
    void rebuildBankButtons();
    void setBank(const QString& bank);
    void cycleBank();
    void selectControl(const QString& id);
    void updateSelectionPanel();
    void rebuildFunctionList();

    void assignFunction(const QString& paramId);
    void unassignSelected();
    void setSelectedInverted(bool inverted);
    bool commit(const QString& message);
    void setStatus(const QString& text, bool problem = false);

    void choosePhoto();
    void removePhoto();
    void loadPhoto();
    QString photoPath() const;

    void onMidiActivity(int channel, int type, int number, int value);

    const MidiLayoutControl* selectedControl() const;
    int  bindingIndexFor(const MidiLayoutControl& c) const;
    QString functionNameFor(const MidiLayoutControl& c) const;
    QString controlNameForParam(const QString& paramId) const;
    bool accepts(const MidiLayoutControl& c, const MidiParam& param) const;

    MidiControlManager* m_manager;
    QVector<QString>     m_layoutFiles;
    MidiControllerLayout m_layout;
    QString              m_selectedId;

    QComboBox*            m_layoutCombo{nullptr};
    QHBoxLayout*          m_bankRow{nullptr};
    QButtonGroup*         m_bankGroup{nullptr};
    QPushButton*          m_photoBtn{nullptr};
    QPushButton*          m_removePhotoBtn{nullptr};
    QCheckBox*            m_followHardware{nullptr};
    MidiControllerCanvas* m_canvas{nullptr};
    QLabel*               m_status{nullptr};

    QLabel*      m_selName{nullptr};
    QLabel*      m_selSource{nullptr};
    QLabel*      m_selFunction{nullptr};
    QLineEdit*   m_search{nullptr};
    QListWidget* m_functions{nullptr};
    QCheckBox*   m_invert{nullptr};
    QPushButton* m_unassign{nullptr};
};

} // namespace AetherSDR

#endif // HAVE_MIDI
