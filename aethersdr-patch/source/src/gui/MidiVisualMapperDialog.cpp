#include "MidiVisualMapperDialog.h"

#ifdef HAVE_MIDI

#include "FramelessMessageBox.h"
#include "MidiControllerCanvas.h"
#include "core/AppSettings.h"
#include "core/MidiControlManager.h"
#include "core/MidiSettings.h"
#include "core/ThemeManager.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImage>
#include <QImageReader>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QVBoxLayout>

#include <algorithm>

namespace AetherSDR {

namespace {

const QString kLayoutKey = QStringLiteral("MidiVisualMapperLayout");
constexpr int kParamRole = Qt::UserRole + 1;
constexpr int kMaxPhotoEdge = 2400;

// {{token}} templates: apply via ThemeManager::applyStyleSheet.
const QString kPanelStyle =
    "QLabel { color: {{color.text.primary}}; }"
    "QLabel#midiVmHint, QLabel#midiVmSource { color: {{color.text.secondary}}; }"
    "QLabel#midiVmTitle { font-size: 15px; font-weight: bold; }"
    "QLabel#midiVmFunction { background: {{color.background.1}}; border: 1px solid {{color.border.strong}}; "
    "  border-radius: 4px; padding: 6px 8px; }"
    "QCheckBox { color: {{color.text.primary}}; }"
    "QLineEdit { background: {{color.background.1}}; color: {{color.text.primary}}; "
    "  border: 1px solid {{color.border.strong}}; border-radius: 3px; padding: 4px 6px; }"
    "QListWidget { background: {{color.background.0}}; color: {{color.text.primary}}; "
    "  border: 1px solid {{color.border.strong}}; }"
    "QListWidget::item { padding: 3px 6px; }"
    "QListWidget::item:selected { background: {{color.accent.dim}}; color: {{color.text.primary}}; }"
    "QComboBox { background: {{color.background.1}}; color: {{color.text.primary}}; "
    "  border: 1px solid {{color.border.strong}}; border-radius: 3px; padding: 2px 6px; }"
    "QPushButton { background: {{color.background.1}}; color: {{color.text.primary}}; "
    "  border: 1px solid {{color.border.strong}}; padding: 4px 10px; border-radius: 3px; }"
    "QPushButton:hover { background: {{color.background.2}}; }"
    "QPushButton:checked { background: {{color.accent}}; color: {{color.background.0}}; "
    "  border-color: {{color.accent}}; font-weight: bold; }";

const QString kStatusOk = "QLabel { color: {{color.text.secondary}}; }";
const QString kStatusProblem = "QLabel { color: {{color.accent.danger}}; }";

bool sourceMatches(const MidiBinding& b, const MidiLayoutControl& c)
{
    if (b.number != c.number) return false;
    if (c.message == MidiLayoutControl::Message::ControlChange)
        return b.msgType == MidiBinding::CC;
    return b.msgType == MidiBinding::NoteOn || b.msgType == MidiBinding::NoteOff;
}

QString sourceText(const MidiLayoutControl& c)
{
    const QString hex = QStringLiteral("0x%1").arg(c.number, 2, 16, QLatin1Char('0')).toUpper().replace("0X", "0x");
    if (c.message == MidiLayoutControl::Message::ControlChange) {
        return c.kind == MidiLayoutControl::Kind::Encoder
            ? QStringLiteral("Control Change %1 (%2), endless: 1 = right, 127 = left").arg(hex).arg(c.number)
            : QStringLiteral("Control Change %1 (%2), 0–127").arg(hex).arg(c.number);
    }
    return QStringLiteral("Note %1 (%2), press and release").arg(hex).arg(c.number);
}

} // namespace

MidiVisualMapperDialog::MidiVisualMapperDialog(MidiControlManager* manager, QWidget* parent)
    : PersistentDialog(QStringLiteral("MIDI Visual Mapper"), QStringLiteral("MidiVisualMapperGeometry"), parent),
      m_manager(manager)
{
    theme::setContainer(this, QStringLiteral("dialog/midiVisualMapper"));
    setObjectName(QStringLiteral("midiVisualMapperDialog"));
    setMinimumSize(900, 520);
    ThemeManager::instance().applyStyleSheet(this, "QDialog { background: {{color.background.0}}; }");

    auto* root = new QHBoxLayout(bodyWidget());
    root->setSpacing(10);

    // ── Left: controller picture ────────────────────────────────────────
    auto* left = new QVBoxLayout;
    left->setSpacing(6);
    auto* top = new QHBoxLayout;
    top->setSpacing(6);
    auto* layoutLabel = new QLabel(QStringLiteral("Controller:"));
    top->addWidget(layoutLabel);
    m_layoutCombo = new QComboBox;
    m_layoutCombo->setObjectName(QStringLiteral("midiVmLayoutCombo"));
    m_layoutCombo->setAccessibleName(QStringLiteral("Controller layout"));
    layoutLabel->setBuddy(m_layoutCombo);
    top->addWidget(m_layoutCombo, 1);

    m_bankRow = new QHBoxLayout;
    m_bankRow->setSpacing(0);
    m_bankGroup = new QButtonGroup(this);
    m_bankGroup->setExclusive(true);
    top->addLayout(m_bankRow);

    m_photoBtn = new QPushButton(QStringLiteral("Photo…"));
    m_photoBtn->setObjectName(QStringLiteral("midiVmPhotoButton"));
    m_photoBtn->setToolTip(QStringLiteral("Show a photo of your controller behind the drawing. "
                                          "Crop it to the controller's edges, taken straight from above."));
    connect(m_photoBtn, &QPushButton::clicked, this, &MidiVisualMapperDialog::choosePhoto);
    top->addWidget(m_photoBtn);
    m_removePhotoBtn = new QPushButton(QStringLiteral("Remove photo"));
    m_removePhotoBtn->setObjectName(QStringLiteral("midiVmRemovePhotoButton"));
    connect(m_removePhotoBtn, &QPushButton::clicked, this, &MidiVisualMapperDialog::removePhoto);
    top->addWidget(m_removePhotoBtn);
    left->addLayout(top);

    m_canvas = new MidiControllerCanvas;
    m_canvas->setLabelProvider([this](const MidiLayoutControl& c) { return functionNameFor(c); });
    connect(m_canvas, &MidiControllerCanvas::controlClicked, this, &MidiVisualMapperDialog::selectControl);
    connect(m_canvas, &MidiControllerCanvas::bankSwitchClicked, this, &MidiVisualMapperDialog::cycleBank);
    left->addWidget(m_canvas, 1);

    auto* bottom = new QHBoxLayout;
    m_followHardware = new QCheckBox(QStringLiteral("Select a control by touching it on the controller"));
    m_followHardware->setObjectName(QStringLiteral("midiVmFollowHardware"));
    m_followHardware->setChecked(true);
    m_followHardware->setToolTip(QStringLiteral(
        "Press a button or move a knob to select it here. The control still does its current "
        "job while you touch it, so mind anything bound to MOX or TUNE."));
    bottom->addWidget(m_followHardware);
    bottom->addStretch();
    left->addLayout(bottom);
    m_status = new QLabel;
    m_status->setObjectName(QStringLiteral("midiVmStatus"));
    m_status->setWordWrap(true);
    left->addWidget(m_status);
    root->addLayout(left, 1);

    // ── Right: what the selected control does ───────────────────────────
    auto* panel = new QWidget;
    panel->setObjectName(QStringLiteral("midiVmPanel"));
    panel->setFixedWidth(310);
    auto* pv = new QVBoxLayout(panel);
    pv->setContentsMargins(0, 0, 0, 0);
    pv->setSpacing(6);
    m_selName = new QLabel(QStringLiteral("Pick a control"));
    m_selName->setObjectName(QStringLiteral("midiVmTitle"));
    m_selName->setWordWrap(true);
    pv->addWidget(m_selName);
    m_selSource = new QLabel(QStringLiteral("Click a knob, fader, wheel or button in the picture."));
    m_selSource->setObjectName(QStringLiteral("midiVmSource"));
    m_selSource->setWordWrap(true);
    pv->addWidget(m_selSource);
    m_selFunction = new QLabel;
    m_selFunction->setObjectName(QStringLiteral("midiVmFunction"));
    m_selFunction->setWordWrap(true);
    pv->addWidget(m_selFunction);

    auto* opts = new QHBoxLayout;
    m_invert = new QCheckBox(QStringLiteral("Reverse direction"));
    m_invert->setObjectName(QStringLiteral("midiVmInvert"));
    m_invert->setAccessibleDescription(QStringLiteral(
        "Available for an assigned knob, fader or wheel; buttons have no direction."));
    connect(m_invert, &QCheckBox::toggled, this, &MidiVisualMapperDialog::setSelectedInverted);
    opts->addWidget(m_invert);
    opts->addStretch();
    m_unassign = new QPushButton(QStringLiteral("Unassign"));
    m_unassign->setObjectName(QStringLiteral("midiVmUnassign"));
    m_unassign->setAccessibleDescription(QStringLiteral("Available when the selected control has a function."));
    connect(m_unassign, &QPushButton::clicked, this, &MidiVisualMapperDialog::unassignSelected);
    opts->addWidget(m_unassign);
    pv->addLayout(opts);

    m_search = new QLineEdit;
    m_search->setObjectName(QStringLiteral("midiVmSearch"));
    m_search->setPlaceholderText(QStringLiteral("Search functions (band, NR, MOX…)"));
    m_search->setAccessibleName(QStringLiteral("Search functions"));
    m_search->setClearButtonEnabled(true);
    connect(m_search, &QLineEdit::textChanged, this, &MidiVisualMapperDialog::rebuildFunctionList);
    pv->addWidget(m_search);

    m_functions = new QListWidget;
    m_functions->setObjectName(QStringLiteral("midiVmFunctions"));
    m_functions->setAccessibleName(QStringLiteral("Functions for the selected control"));
    m_functions->setUniformItemSizes(false);
    // One click assigns — the list only offers functions this control can drive.
    connect(m_functions, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        const QString id = item->data(kParamRole).toString();
        if (!id.isEmpty()) assignFunction(id);
    });
    connect(m_functions, &QListWidget::itemActivated, this, [this](QListWidgetItem* item) {
        const QString id = item->data(kParamRole).toString();
        if (!id.isEmpty()) assignFunction(id);
    });
    pv->addWidget(m_functions, 1);

    auto* hint = new QLabel(QStringLiteral(
        "Each function can sit on one control: picking one that's already used moves it here. "
        "Changes are saved at once and show in the MIDI Mapping table."));
    hint->setObjectName(QStringLiteral("midiVmHint"));
    hint->setWordWrap(true);
    pv->addWidget(hint);
    root->addWidget(panel);

    ThemeManager::instance().applyStyleSheet(bodyWidget(), kPanelStyle);
    ThemeManager::instance().applyStyleSheet(m_status, kStatusOk);

    connect(m_manager, &MidiControlManager::midiActivity, this, &MidiVisualMapperDialog::onMidiActivity);
    connect(m_layoutCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &MidiVisualMapperDialog::showLayout);

    populateLayouts();
    updateSelectionPanel();
}

// ── Layouts ─────────────────────────────────────────────────────────────────

void MidiVisualMapperDialog::populateLayouts()
{
    const QSignalBlocker block(m_layoutCombo);
    m_layoutCombo->clear();
    m_layoutFiles.clear();
    QStringList broken;
    for (const QString& file : availableMidiControllerLayoutFiles()) {
        const MidiLayoutLoadResult r = loadMidiControllerLayout(file);
        if (!r.ok()) {
            broken << QStringLiteral("%1: %2").arg(QFileInfo(file).fileName(), r.error);
            continue;
        }
        m_layoutFiles.append(file);
        m_layoutCombo->addItem(r.layout.name, r.layout.id);
    }
    if (!broken.isEmpty())
        setStatus(QStringLiteral("Skipped layout files with problems — %1").arg(broken.join(QStringLiteral("; "))), true);

    if (m_layoutFiles.isEmpty()) {
        m_canvas->setControllerLayout(nullptr);
        return;
    }
    const QString wanted = AppSettings::instance().value(kLayoutKey, QString()).toString();
    int index = m_layoutCombo->findData(wanted);
    if (index < 0) index = 0;
    m_layoutCombo->setCurrentIndex(index);
    showLayout(index);
}

void MidiVisualMapperDialog::showLayout(int comboIndex)
{
    if (comboIndex < 0 || comboIndex >= m_layoutFiles.size()) return;
    const MidiLayoutLoadResult r = loadMidiControllerLayout(m_layoutFiles.at(comboIndex));
    if (!r.ok()) {
        setStatus(r.error, true);
        return;
    }
    m_layout = r.layout;
    m_selectedId.clear();
    m_canvas->setControllerLayout(&m_layout);
    AppSettings::instance().setValue(kLayoutKey, m_layout.id);
    rebuildBankButtons();
    loadPhoto();
    updateSelectionPanel();
    if (m_status->text().isEmpty() || !m_status->property("problem").toBool())
        setStatus(QStringLiteral("Click a control in the picture, or touch it on the controller."));
}

void MidiVisualMapperDialog::rebuildBankButtons()
{
    for (auto* b : m_bankGroup->buttons()) {
        m_bankGroup->removeButton(b);
        b->deleteLater();
    }
    if (m_layout.banks.size() < 2) return;
    for (const auto& bank : m_layout.banks) {
        auto* b = new QPushButton(bank.label);
        b->setCheckable(true);
        b->setProperty("bankId", bank.id);
        b->setAccessibleName(QStringLiteral("Show %1").arg(bank.label));
        b->setChecked(bank.id == m_canvas->bank());
        connect(b, &QPushButton::clicked, this, [this, id = bank.id] { setBank(id); });
        m_bankGroup->addButton(b);
        m_bankRow->addWidget(b);
    }
}

void MidiVisualMapperDialog::setBank(const QString& bank)
{
    m_canvas->setBank(bank);
    for (auto* b : m_bankGroup->buttons())
        b->setChecked(b->property("bankId").toString() == bank);
    // Keep the selection on the same physical key in the other bank.
    const auto* c = selectedControl();
    if (c && !c->bank.isEmpty() && c->bank != bank) {
        for (const auto& other : m_layout.controls) {
            if (other.bank == bank && other.center == c->center) {
                selectControl(other.id);
                return;
            }
        }
        selectControl(QString());
    }
}

void MidiVisualMapperDialog::cycleBank()
{
    if (m_layout.banks.size() < 2) return;
    int at = 0;
    for (int i = 0; i < m_layout.banks.size(); ++i)
        if (m_layout.banks.at(i).id == m_canvas->bank()) at = i;
    setBank(m_layout.banks.at((at + 1) % m_layout.banks.size()).id);
}

// ── Selection ───────────────────────────────────────────────────────────────

const MidiLayoutControl* MidiVisualMapperDialog::selectedControl() const
{
    return m_selectedId.isEmpty() ? nullptr : m_layout.findById(m_selectedId);
}

void MidiVisualMapperDialog::selectControl(const QString& id)
{
    m_selectedId = id;
    m_canvas->setSelectedControl(id);
    m_search->clear();
    updateSelectionPanel();
}

void MidiVisualMapperDialog::updateSelectionPanel()
{
    const auto* c = selectedControl();
    const bool has = c != nullptr;
    m_search->setEnabled(has);
    m_functions->setEnabled(has);
    if (!has) {
        m_selName->setText(QStringLiteral("Pick a control"));
        m_selSource->setText(QStringLiteral("Click a knob, fader, wheel or button in the picture."));
        m_selFunction->setText(QStringLiteral("Nothing selected"));
        m_invert->setEnabled(false);
        m_unassign->setEnabled(false);
        m_functions->clear();
        return;
    }
    m_selName->setText(c->name);
    m_selSource->setText(sourceText(*c));
    const int bi = bindingIndexFor(*c);
    const QString fn = functionNameFor(*c);
    m_selFunction->setText(fn.isEmpty() ? QStringLiteral("Not assigned") : fn);
    {
        const QSignalBlocker block(m_invert);
        m_invert->setChecked(bi >= 0 && m_manager->bindings().at(bi).inverted);
    }
    // Reversing only means something for knobs, faders and wheels.
    m_invert->setEnabled(bi >= 0 && c->kind != MidiLayoutControl::Kind::Button);
    m_unassign->setEnabled(bi >= 0);
    rebuildFunctionList();
}

void MidiVisualMapperDialog::rebuildFunctionList()
{
    m_functions->clear();
    const auto* c = selectedControl();
    if (!c) return;
    const QString query = m_search->text().trimmed();
    const int bi = bindingIndexFor(*c);
    const QString current = bi >= 0 ? m_manager->bindings().at(bi).paramId : QString();

    QString lastCategory;
    QListWidgetItem* currentItem = nullptr;
    for (const auto& p : m_manager->params()) {
        if (!accepts(*c, p)) continue;
        if (!query.isEmpty()
            && !p.displayName.contains(query, Qt::CaseInsensitive)
            && !p.id.contains(query, Qt::CaseInsensitive)
            && !p.category.contains(query, Qt::CaseInsensitive))
            continue;
        if (p.category != lastCategory) {
            auto* header = new QListWidgetItem(p.category.toUpper(), m_functions);
            header->setFlags(Qt::NoItemFlags);
            QFont f = header->font();
            f.setBold(true);
            if (f.pointSizeF() > 0) f.setPointSizeF(f.pointSizeF() * 0.85);
            header->setFont(f);
            lastCategory = p.category;
        }
        QString text = p.displayName;
        const QString usedOn = controlNameForParam(p.id);
        if (!usedOn.isEmpty() && p.id != current)
            text += QStringLiteral("   · on %1").arg(usedOn);
        auto* item = new QListWidgetItem(text, m_functions);
        item->setData(kParamRole, p.id);
        item->setToolTip(p.id);
        if (p.id == current) {
            QFont f = item->font();
            f.setBold(true);
            item->setFont(f);
            currentItem = item;
        }
    }
    if (m_functions->count() == 0) {
        auto* none = new QListWidgetItem(QStringLiteral("No matching function for this kind of control."), m_functions);
        none->setFlags(Qt::NoItemFlags);
    }
    if (currentItem) {
        m_functions->setCurrentItem(currentItem);
        m_functions->scrollToItem(currentItem);
    }
}

// ── Binding lookups ─────────────────────────────────────────────────────────

int MidiVisualMapperDialog::bindingIndexFor(const MidiLayoutControl& c) const
{
    const auto& bs = m_manager->bindings();
    for (int i = 0; i < bs.size(); ++i)
        if (sourceMatches(bs.at(i), c)) return i;
    return -1;
}

QString MidiVisualMapperDialog::functionNameFor(const MidiLayoutControl& c) const
{
    const int bi = bindingIndexFor(c);
    if (bi < 0) return {};
    const QString id = m_manager->bindings().at(bi).paramId;
    const MidiParam* p = m_manager->findParam(id);
    return p ? p->displayName : id;
}

QString MidiVisualMapperDialog::controlNameForParam(const QString& paramId) const
{
    for (const auto& b : m_manager->bindings()) {
        if (b.paramId != paramId) continue;
        const auto msg = b.msgType == MidiBinding::CC ? MidiLayoutControl::Message::ControlChange
                                                      : MidiLayoutControl::Message::Note;
        if (b.msgType == MidiBinding::PitchBend) return QStringLiteral("pitch bend");
        const auto* c = m_layout.findBySource(msg, b.number);
        return c ? c->name : b.sourceDisplayName();
    }
    return {};
}

bool MidiVisualMapperDialog::accepts(const MidiLayoutControl& c, const MidiParam& param) const
{
    const bool vfo = MidiControlManager::isVfoTuneKnob(param.id);
    switch (c.kind) {
    case MidiLayoutControl::Kind::Button:
        return param.type == MidiParamType::Toggle || param.type == MidiParamType::Trigger
            || param.type == MidiParamType::Gate;
    case MidiLayoutControl::Kind::Absolute:
        return param.type == MidiParamType::Slider && !vfo;
    case MidiLayoutControl::Kind::Encoder:
        // AetherSDR decodes endless (relative) wheels only for VFO tuning.
        return vfo;
    }
    return false;
}

// ── Editing ─────────────────────────────────────────────────────────────────

void MidiVisualMapperDialog::assignFunction(const QString& paramId)
{
    const auto* c = selectedControl();
    if (!c) return;
    const MidiParam* param = m_manager->findParam(paramId);
    if (!param || !accepts(*c, *param)) return;

    // Where the function lived before, so the move can be named.
    QString movedFrom = controlNameForParam(paramId);
    if (movedFrom == c->name) movedFrom.clear();

    // A control drives one function: drop whatever this control did before,
    // keeping its channel (a learned binding may name one) and, when the
    // function is unchanged, its direction.
    auto& bs = m_manager->bindings();
    int channel = -1;
    bool inverted = false;
    for (int i = bs.size() - 1; i >= 0; --i) {
        if (!sourceMatches(bs.at(i), *c)) continue;
        channel = bs.at(i).channel;
        if (bs.at(i).paramId == paramId) inverted = bs.at(i).inverted;
        bs.removeAt(i);
    }
    m_manager->rebuildIndex();

    MidiBinding b;
    b.channel  = channel;
    b.msgType  = c->message == MidiLayoutControl::Message::ControlChange ? MidiBinding::CC
                                                                         : MidiBinding::NoteOn;
    b.number   = c->number;
    b.paramId  = paramId;
    b.inverted = inverted;
    b.relative = c->message == MidiLayoutControl::Message::ControlChange
                 && MidiControlManager::isVfoTuneKnob(paramId);
    // addBinding() removes any other binding of this function: one function,
    // one control — the same rule the MIDI Mapping table follows.
    m_manager->addBinding(b);

    commit(movedFrom.isEmpty()
               ? QStringLiteral("%1 → %2").arg(c->name, param->displayName)
               : QStringLiteral("%1 moved here from %2.").arg(param->displayName, movedFrom));
}

void MidiVisualMapperDialog::unassignSelected()
{
    const auto* c = selectedControl();
    if (!c) return;
    auto& bs = m_manager->bindings();
    for (int i = bs.size() - 1; i >= 0; --i)
        if (sourceMatches(bs.at(i), *c)) bs.removeAt(i);
    m_manager->rebuildIndex();
    commit(QStringLiteral("%1 cleared.").arg(c->name));
}

void MidiVisualMapperDialog::setSelectedInverted(bool inverted)
{
    const auto* c = selectedControl();
    if (!c) return;
    const int bi = bindingIndexFor(*c);
    if (bi < 0) return;
    m_manager->bindings()[bi].inverted = inverted;
    m_manager->rebuildIndex();
    commit(inverted ? QStringLiteral("%1 reversed.").arg(c->name)
                    : QStringLiteral("%1 back to normal direction.").arg(c->name));
}

bool MidiVisualMapperDialog::commit(const QString& message)
{
    const bool saved = MidiSettings::instance().saveBindings(m_manager->bindings());
    m_canvas->update();
    updateSelectionPanel();
    emit bindingsChanged();
    if (!saved) {
        setStatus(QStringLiteral("The change is active but couldn't be saved — check that %1 is writable.")
                      .arg(QDir::toNativeSeparators(MidiSettings::profileDir())), true);
        return false;
    }
    setStatus(message);
    return true;
}

void MidiVisualMapperDialog::refresh()
{
    m_canvas->update();
    updateSelectionPanel();
}

void MidiVisualMapperDialog::setStatus(const QString& text, bool problem)
{
    m_status->setText(text);
    m_status->setProperty("problem", problem);
    ThemeManager::instance().applyStyleSheet(m_status, problem ? kStatusProblem : kStatusOk);
}

// ── Photo ───────────────────────────────────────────────────────────────────

QString MidiVisualMapperDialog::photoPath() const
{
    return midiLayoutUserDir() + QStringLiteral("/%1-photo.png").arg(m_layout.id);
}

void MidiVisualMapperDialog::loadPhoto()
{
    QPixmap photo;
    if (!m_layout.id.isEmpty() && QFileInfo::exists(photoPath()))
        photo.load(photoPath());
    m_canvas->setPhoto(photo);
    m_removePhotoBtn->setEnabled(!photo.isNull());
}

void MidiVisualMapperDialog::choosePhoto()
{
    if (m_layout.id.isEmpty()) return;
    const QString pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    const QString file = QFileDialog::getOpenFileName(
        this, QStringLiteral("Choose a photo of your controller"),
        pictures.isEmpty() ? QDir::homePath() : pictures,
        QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.webp)"));
    if (file.isEmpty()) return;

    QImageReader reader(file);
    reader.setAutoTransform(true);  // honour phone-camera rotation
    QImage image = reader.read();
    if (image.isNull()) {
        FramelessMessageBox::warning(this, QStringLiteral("Controller photo"),
            QStringLiteral("Couldn't open %1 as an image: %2")
                .arg(QFileInfo(file).fileName(), reader.errorString()));
        return;
    }
    if (std::max(image.width(), image.height()) > kMaxPhotoEdge)
        image = image.scaled(kMaxPhotoEdge, kMaxPhotoEdge, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QDir().mkpath(midiLayoutUserDir());
    QSaveFile out(photoPath());
    if (!out.open(QIODevice::WriteOnly) || !image.save(&out, "PNG") || !out.commit()) {
        FramelessMessageBox::warning(this, QStringLiteral("Controller photo"),
            QStringLiteral("Couldn't save the photo to %1.")
                .arg(QDir::toNativeSeparators(midiLayoutUserDir())));
        return;
    }
    loadPhoto();
    setStatus(QStringLiteral("Photo added. For the best fit, crop it to the controller's edges."));
}

void MidiVisualMapperDialog::removePhoto()
{
    if (QFileInfo::exists(photoPath()) && !QFile::remove(photoPath())) {
        setStatus(QStringLiteral("Couldn't remove %1.").arg(QDir::toNativeSeparators(photoPath())), true);
        return;
    }
    loadPhoto();
    setStatus(QStringLiteral("Photo removed."));
}

// ── Hardware follow ─────────────────────────────────────────────────────────

void MidiVisualMapperDialog::onMidiActivity(int /*channel*/, int type, int number, int value)
{
    if (!isVisible() || !m_followHardware->isChecked() || m_manager->isLearning()) return;
    MidiLayoutControl::Message msg;
    if (type == MidiBinding::CC) msg = MidiLayoutControl::Message::ControlChange;
    else if (type == MidiBinding::NoteOn && value > 0) msg = MidiLayoutControl::Message::Note;
    else return;  // releases and pitch bend don't select anything

    const auto* c = m_layout.findBySource(msg, number);
    if (!c) {
        setStatus(QStringLiteral("Received %1 %2, which isn't a control in this layout.")
                      .arg(msg == MidiLayoutControl::Message::ControlChange ? QStringLiteral("CC")
                                                                            : QStringLiteral("note"))
                      .arg(number));
        return;
    }
    if (!c->bank.isEmpty() && c->bank != m_canvas->bank())
        setBank(c->bank);
    if (c->id != m_selectedId)
        selectControl(c->id);
    m_canvas->flashControl(c->id);
}

} // namespace AetherSDR

#endif // HAVE_MIDI
