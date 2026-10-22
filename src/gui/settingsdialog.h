#pragma once

#include <QDialog>

class GlobalShortcut;
class ModelDownloader;
class TextOutput;
class QCheckBox;
class QComboBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QWidget;

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
    QWidget *recognitionPage();
    QWidget *recordingPage();
    QWidget *generalPage();
    void load();
    void fillModels();
    void updateModelRow();
    void updateShortcutLabel();
    void fitHeight();
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
    QLabel *m_customUrlLabel;
    QLineEdit *m_customUrl;
    QCheckBox *m_unverified;
    QPlainTextEdit *m_downloadLog;
    QComboBox *m_language;
    QComboBox *m_gpu;
    QLineEdit *m_prompt;
    QFormLayout *m_recognitionForm;
    QList<QWidget *> m_whisperRows; // rows shown only for Whisper models

    QComboBox *m_microphone;
    QLabel *m_shortcutLabel;
    QPushButton *m_shortcutButton;
    QWidget *m_binding;
    QLabel *m_bindingWhere;
    QPlainTextEdit *m_bindingSnippet;
    QCheckBox *m_live;
    QSpinBox *m_pause;

    QRadioButton *m_paste;
    QRadioButton *m_copyOnly;
    QComboBox *m_pasteKeys;
    QComboBox *m_inputMethod;
    QLabel *m_portalAccessLabel;
    QWidget *m_portalAccess;
    QLabel *m_pasteAccess;
    QPushButton *m_pasteAccessButton;

    QComboBox *m_uiLanguage;
    QCheckBox *m_autostart;
    QSpinBox *m_unload;
};
