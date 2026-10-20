#include "i18n.h"

#include <QCoreApplication>
#include <QHash>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

namespace {

// Source (English) → Russian. Keys must match the tr() strings exactly.
const QHash<QString, QString> &russian()
{
    static const QHash<QString, QString> table{
    {"The downloaded file does not match the published checksum, so it was deleted. Try again or switch the source.", "Скачанный файл не совпадает с опубликованной контрольной суммой и удалён. Попробуйте ещё раз или смените источник."},
    {" (recommended)", " (рекомендуется)"},
    {" min", " мин"},
    {"%1 of %2 MB", "%1 из %2 МБ"},
    {"%1 — %2 MB", "%1 — %2 МБ"},
    {"Acceleration:", "Ускорение:"},
    {"Allow…", "Разрешить…"},
    {"Also delete downloaded models and settings (%1 MB)", "Также удалить скачанные модели и настройки (%1 МБ)"},
    {"Cancel", "Отменить"},
    {"Cancel dictation", "Отменить диктовку"},
    {"Change…", "Изменить…"},
    {"Copied: %1", "Скопировано: %1"},
    {"Could not copy the text to the clipboard", "Не удалось скопировать текст в буфер обмена"},
    {"Could not delete %1", "Не удалось удалить файл %1"},
    {"Could not download the model:\n%1", "Не удалось скачать модель:\n%1"},
    {"Could not load the model", "Не удалось загрузить модель"},
    {"Could not open the microphone", "Не удалось открыть микрофон"},
    {"Could not save the model file", "Не удалось сохранить файл модели"},
    {"Ctrl+Shift+V (terminals)", "Ctrl+Shift+V (терминалы)"},
    {"Cut silence and noise (Silero VAD)", "Отсекать тишину и шум (Silero VAD)"},
    {"Delete", "Удалить"},
    {"Delete model", "Удалить модель"},
    {"Delete the “%1” model from disk?", "Удалить модель «%1» с диска?"},
    {"Detect automatically", "Определять автоматически"},
    {"Download", "Скачать"},
    {"Download a model in Settings first", "Сначала скачайте модель в настройках"},
    {"Downloaded — %1", "Скачана — %1"},
    {"Drag the indicator with the mouse.\nClick to stop recording, right-click to cancel.", "Индикатор можно перетащить мышью.\nКлик — остановить запись, правый клик — отменить."},
    {"Filter:", "Фильтр:"},
    {"For example: Kubernetes, Grafana, Jane Doe", "Например: Kubernetes, Grafana, Анна Сергеевна"},
    {"General", "Общие"},
    {"Indicator:", "Индикатор:"},
    {"Input access:", "Доступ к вводу:"},
    {"Language:", "Язык:"},
    {"Microphone:", "Микрофон:"},
    {"Mirror hf-mirror.com", "Зеркало hf-mirror.com"},
    {"Model download", "Загрузка модели"},
    {"Model downloaded", "Модель скачана"},
    {"Model:", "Модель:"},
    {"Names and terms you often say —\nwhisper will spell them correctly.", "Имена и термины, которые часто звучат в речи, —\nwhisper будет писать их правильно."},
    {"No input access — the text is in the clipboard, paste it yourself", "Нет доступа к вводу — текст в буфере обмена, вставьте его вручную"},
    {"No microphone found", "Микрофон не найден"},
    {"No speech heard — check the microphone", "Не слышно речи — проверьте микрофон"},
    {"Not downloaded — %1", "Не скачана — %1"},
    {"Nothing recognized", "Ничего не распознано"},
    {"Only copy to the clipboard", "Только копировать в буфер обмена"},
    {"Only speech is recognized: faster, and no invented phrases in silence", "Распознаётся только речь: быстрее и без выдуманных фраз в тишине"},
    {"Open settings", "Открыть настройки"},
    {"Open the models folder", "Открыть папку моделей"},
    {"Paste into the active window", "Вставлять в активное окно"},
    {"Paste keys:", "Сочетание вставки:"},
    {"Pasting", "Вставка текста"},
    {"Press %1 and speak.", "Нажмите %1 и говорите."},
    {"Quit", "Выход"},
    {"Quit the running Sotto", "Закрыть запущенный Sotto"},
    {"Ready for dictation.", "Всё готово к диктовке."},
    {"Recognition", "Распознавание"},
    {"Recognition error (%1)", "Ошибка распознавания (%1)"},
    {"Recording", "Запись"},
    {"Recording and pasting", "Запись и вставка"},
    {"Removal:", "Удаление:"},
    {"Remove Sotto", "Удалить Sotto"},
    {"Remove Sotto from this computer", "Удалить Sotto с компьютера"},
    {"Remove Sotto from this computer?\n\nThe program, its menu entry, autostart and shortcut will be removed.", "Удалить Sotto с компьютера?\n\nБудут удалены программа, ярлык в меню, автозапуск и горячая клавиша."},
    {"Remove Sotto…", "Удалить Sotto…"},
    {"Reset position", "Вернуть на место"},
    {"Result:", "Результат:"},
    {"Settings", "Настройки"},
    {"Settings…", "Настройки…"},
    {"Shortcut:", "Горячая клавиша:"},
    {"Sotto is installed in %1. Remove it with your distribution's tools.", "Программа установлена в %1. Удалите её средствами вашего дистрибутива."},
    {"Sotto was removed.", "Sotto удалён."},
    {"Sotto — recording", "Sotto — идёт запись"},
    {"Sotto — transcribing…", "Sotto — распознаю…"},
    {"Sotto — voice typing", "Sotto — голосовой ввод"},
    {"Sotto — voice typing with whisper.cpp", "Sotto — голосовой ввод на whisper.cpp"},
    {"Source:", "Источник:"},
    {"Start at login", "Запускать при входе в систему"},
    {"Start dictation", "Начать диктовку"},
    {"Start or stop dictation", "Начать или закончить диктовку"},
    {"Stop and paste", "Остановить и вставить"},
    {"Stop dictation and paste the text", "Закончить диктовку и вставить текст"},
    {"System default", "Системный по умолчанию"},
    {"Tap: records until the next tap.\nHold: records while the key is held.\nAny key can also run the command “sotto --toggle”.", "Нажмите и отпустите — запись до следующего нажатия.\nУдерживайте — запись, пока клавиша нажата.\nЛюбую клавишу можно назначить на команду «sotto --toggle»."},
    {"The %1 model is slow on a CPU. Install a Vulkan driver or choose the Small model in Settings.", "Модель %1 на процессоре работает медленно. Установите драйвер Vulkan или выберите модель Small в настройках."},
    {"The GPU is not used", "Видеокарта не используется"},
    {"The download is incomplete (%1 of %2 bytes)", "Файл скачался не полностью (%1 из %2 байт)"},
    {"The microphone was disconnected", "Микрофон отключился"},
    {"The package was not removed. Run it yourself:\n%1", "Пакет не удалён. Выполните вручную:\n%1"},
    {"The system asks once for “remote control” permission:\nSotto uses it to press the paste keys. Tick “Remember”.", "Система один раз спросит разрешение на «удалённое управление»:\nчерез него Sotto нажимает сочетание вставки. Отметьте «Запомнить»."},
    {"Unload the model after:", "Выгружать модель через:"},
    {"Use the GPU", "Использовать видеокарту"},
    {"Use the GPU (%1)", "Использовать видеокарту (%1)"},
    {"Vocabulary:", "Словарь:"},
    {"Voice typing", "Голосовой ввод"},
    {"accurate and fast on a GPU", "точно и быстро на видеокарте"},
    {"best choice without a GPU", "лучший выбор без видеокарты"},
    {"connecting…", "подключение…"},
    {"custom model", "своя модель"},
    {"granted", "разрешён"},
    {"more accurate, slow on a CPU", "точнее, но на процессоре медленно"},
    {"most accurate, slow", "максимум точности, медленно"},
    {"never", "никогда"},
    {"not assigned", "не назначена"},
    {"not requested yet", "ещё не запрошен"},
    {"slightly more accurate, more memory", "чуть точнее, больше памяти"},
    {"the GlobalShortcuts portal is unavailable", "портал глобальных сочетаний недоступен"},
    {"the portal did not bind the shortcut", "портал не привязал горячую клавишу"},
    {"the portal refused", "отказ портала"},
    {"the portal rejected the shortcut", "привязка горячей клавиши отклонена порталом"},
    {"unavailable: %1", "недоступна: %1"},
    {"very fast, average accuracy", "очень быстро, точность средняя"},
    {"whisper.cpp models: %1\nVAD: %2", "Модели whisper.cpp: %1\nVAD: %2"},
    {"⚠ No GPU in use — Large models are slow on a CPU.<br>Install a Vulkan driver (on Arch also <b>ggml-vulkan</b>) or choose Small.", "⚠ Видеокарта не используется — модели Large на процессоре медленные.<br>Установите драйвер Vulkan (в Arch также <b>ggml-vulkan</b>) или выберите Small."},
    };
    return table;
}

class Translator : public QTranslator
{
public:
    explicit Translator(const QHash<QString, QString> &table)
        : m_table(table)
    {
    }
    bool isEmpty() const override { return false; }
    QString translate(const char *, const char *source, const char *, int) const override
    {
        return m_table.value(QString::fromUtf8(source));
    }

private:
    const QHash<QString, QString> &m_table;
};

} // namespace

namespace I18n {

void install(QCoreApplication &app)
{
    const QString forced = qEnvironmentVariable("SOTTO_LANG");
    const bool ru = forced.isEmpty() ? QLocale().language() == QLocale::Russian : forced.startsWith(QLatin1String("ru"));
    if (!ru)
        return;
    static Translator translator(russian());
    app.installTranslator(&translator);
    // Standard Qt dialog buttons (Yes, Cancel, ...) follow when Qt's own translations are installed.
    static QTranslator qt;
    if (qt.load(QLocale(QLocale::Russian), QStringLiteral("qtbase"), QStringLiteral("_"),
                QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        app.installTranslator(&qt);
}

} // namespace I18n
