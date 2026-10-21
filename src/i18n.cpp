#include "i18n.h"
#include "config.h"

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
    {"Automatic — %1", "Автоматически — %1"},
    {"Processor only", "Только процессор"},
    {"%1 (not found)", "%1 (не найдена)"},
    {"The discrete GPU is not used", "Дискретная видеокарта не используется"},
    {"The GPU part of ggml is not installed: install ggml-vulkan on Arch.", "Не установлена часть ggml для видеокарт: в Arch установите ggml-vulkan."},
    {"The NVIDIA card runs on the nouveau driver, which gives it no Vulkan here. Install the NVIDIA driver, or Mesa's NVK (vulkan-nouveau) for recent cards.", "Видеокарта NVIDIA работает на драйвере nouveau, и Vulkan для неё нет. Установите драйвер NVIDIA или, для новых карт, NVK из Mesa (vulkan-nouveau)."},
    {"The NVIDIA card has no driver loaded. It may be switched off (integrated-only mode in envycontrol, supergfxctl or optimus-manager: choose hybrid), or the NVIDIA driver is not installed.", "Драйвер видеокарты NVIDIA не загружен. Возможно, она выключена (режим «только встроенная» в envycontrol, supergfxctl или optimus-manager — выберите гибридный) или драйвер NVIDIA не установлен."},
    {"The NVIDIA driver was updated (kernel module %1, libraries %2): restart the computer.", "Драйвер NVIDIA обновился (модуль ядра %1, библиотеки %2): перезагрузите компьютер."},
    {"The Vulkan part of the NVIDIA driver is not installed: nvidia-utils on Arch, nvidia-vulkan-icd on Debian, libnvidia-gl on Ubuntu.", "Не установлена часть драйвера NVIDIA для Vulkan: nvidia-utils в Arch, nvidia-vulkan-icd в Debian, libnvidia-gl в Ubuntu."},
    {"The NVIDIA driver is installed but Vulkan does not see the card. Run “vulkaninfo --summary” in a terminal to see why; on Arch, ggml-cuda is another way to use it.", "Драйвер NVIDIA установлен, но Vulkan не видит видеокарту. Запустите в терминале «vulkaninfo --summary», чтобы узнать причину; в Arch видеокарту можно задействовать и через ggml-cuda."},
    {"The AMD graphics have no Vulkan driver: install vulkan-radeon on Arch, mesa-vulkan-drivers on Debian, Ubuntu or Fedora.", "Для видеокарты AMD нет драйвера Vulkan: установите vulkan-radeon в Arch, mesa-vulkan-drivers в Debian, Ubuntu или Fedora."},
    {"The Intel graphics have no Vulkan driver: install vulkan-intel on Arch, mesa-vulkan-drivers on Debian, Ubuntu or Fedora.", "Для графики Intel нет драйвера Vulkan: установите vulkan-intel в Arch, mesa-vulkan-drivers в Debian, Ubuntu или Fedora."},
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
    {"Follow system", "Как в системе"},
    {"Interface language:", "Язык интерфейса:"},
    {"Interface language", "Язык интерфейса"},
    {"Restart Sotto to apply the language.", "Перезапустите Sotto, чтобы применить язык."},
    {"Custom server…", "Свой сервер…"},
    {"Server:", "Сервер:"},
    {"https://your-server.example/ (same layout as huggingface.co)", "https://ваш-сервер.example/ (структура как у huggingface.co)"},
    {"Models are fetched from ADDRESS/ggerganov/whisper.cpp/resolve/main/ggml-NAME.bin", "Модели берутся с АДРЕС/ggerganov/whisper.cpp/resolve/main/ggml-ИМЯ.bin"},
    {"Do not check the published SHA-256 (my own models)", "Не сверять опубликованную SHA-256 (мои модели)"},
    {"Only for a custom server that hosts files other than the official ones. Official models are always verified.", "Только для своего сервера с файлами, отличающимися от официальных. Официальные модели проверяются всегда."},
    {"Download log:", "Журнал загрузки:"},
    {"The download log appears here: server, redirects, checksum.", "Здесь появится журнал загрузки: сервер, перенаправления, контрольная сумма."},
    {"Also saved to %1", "Также сохраняется в %1"},
    {"not a valid address (https://…)", "неверный адрес (нужен https://…)"},
    {"enter the server address below", "введите адрес сервера ниже"},
    {"The custom server address is missing or not valid (use https://…).", "Адрес своего сервера не указан или неверен (нужен https://…)."},
    {"Source: %1", "Источник: %1"},
    {"Downloading %1", "Загрузка %1"},
    {"Redirected to %1", "Перенаправление на %1"},
    {"Checksum does not match: got %1, expected %2", "Контрольная сумма не совпала: получено %1, ожидалось %2"},
    {"SHA-256 matches the published value (%1)", "SHA-256 совпала с опубликованной (%1)"},
    {"Checksum not verified (SHA-256 %1)", "Контрольная сумма не проверялась (SHA-256 %1)"},
    {"Saved %1", "Сохранено: %1"},
    {"Failed: %1", "Ошибка: %1"},
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
    // SOTTO_LANG wins, then the setting; "auto" follows the system language.
    const QString forced = qEnvironmentVariable("SOTTO_LANG");
    const QString choice = forced.isEmpty() ? Config::uiLanguage() : forced;
    const bool ru = choice == QLatin1String("auto") || choice.isEmpty() ? QLocale().language() == QLocale::Russian
                                                                         : choice.startsWith(QLatin1String("ru"));
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
