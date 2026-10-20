#pragma once

#include <QDialog>

class GlobalShortcut;
class ModelDownloader;
class TextOutput;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QRadioButton;
class QSpinBox;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    SettingsDialog(ModelDownloader *downloader, GlobalShortcut *shortcut, TextOutput *output, QWidget *parent = nullptr);

    void accept() override;

Q_SIGNALS:
    void resetOverlayRequested();
    void uninstallRequested();

private:
    void load();
    void fillModels();
    void updateModelRow();
    void updateShortcutLabel();
    void updateSourceLink();
    void updatePasteAccess();
    QString selectedModel() const;

    ModelDownloader *m_downloader;
    GlobalShortcut *m_shortcut;
    TextOutput *m_output;
    bool m_haveGpu = false;

    QComboBox *m_model;
    QPushButton *m_modelAction;
    QProgressBar *m_progress;
    QLabel *m_modelNote;
    QComboBox *m_source;
    QLabel *m_sourceLink;
    QLineEdit *m_customUrl;
    QCheckBox *m_unverified;
    QPlainTextEdit *m_downloadLog;
    QCheckBox *m_vad;
    QComboBox *m_language;
    QCheckBox *m_gpu;
    QLineEdit *m_prompt;

    QComboBox *m_microphone;
    QLabel *m_shortcutLabel;
    QPushButton *m_shortcutButton;

    QRadioButton *m_paste;
    QRadioButton *m_copyOnly;
    QComboBox *m_pasteKeys;
    QLabel *m_pasteAccess;
    QPushButton *m_pasteAccessButton;

    QComboBox *m_uiLanguage;
    QCheckBox *m_autostart;
    QSpinBox *m_unload;
};
