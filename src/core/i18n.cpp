#include "core/config.h"
#include "core/i18n.h"

#include <QCoreApplication>
#include <QHash>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

namespace {

// Source (English) → Russian. Keys must match the tr() strings exactly: tools/check-translations.py
// (a ctest) finds missing and unused entries.
const QHash<QString, QString> &russian()
{
    static const QHash<QString, QString> table{
    {"%1 (not found)", "%1 (не найдена)"},
    {"%1 is slow on a CPU. Install a Vulkan driver or choose GigaAM or Parakeet in Settings.", "%1 на процессоре работает медленно. Установите драйвер Vulkan или выберите в настройках GigaAM или Parakeet."},
    {"%1 — model page", "%1 — страница модели"},
    {"%1 has no shortcut portal", "у %1 нет портала горячих клавиш"},
    {"%1 of %2 MB", "%1 из %2 МБ"},
    {"%1 s of audio, %2 phrases, %3 ms (%4)", "звук: %1 с, фраз: %2, за %3 мс (%4)"},
    {"%1 — %2 MB", "%1 — %2 МБ"},
    {" (recommended)", " (рекомендуется)"},
    {"* downloaded, in %1", "* скачана, лежит в %1"},
    {"25 European languages with Russian, punctuation, fast on a CPU", "25 европейских языков, включая русский, с пунктуацией, быстро на процессоре"},
    {"99 languages, accurate and fast on a GPU", "99 языков, точно и быстро на видеокарте"},
    {"Acceleration:", "Ускорение:"},
    {"Allow…", "Разрешить…"},
    {"Also delete downloaded models and settings (%1 MB)", "Также удалить скачанные модели и настройки (%1 МБ)"},
    {"Also saved to %1", "Также сохраняется в %1"},
    {"Automatic — %1", "Автоматически — %1"},
    {"Automatic — nothing found", "Автоматически — ничего не найдено"},
    {"Cancel", "Отменить"},
    {"Cancel dictation", "Отменить диктовку"},
    {"Change…", "Изменить…"},
    {"Checksum does not match: got %1, expected %2", "Контрольная сумма не совпала: получено %1, ожидалось %2"},
    {"Checksum not verified (SHA-256 %1)", "Контрольная сумма не проверялась (SHA-256 %1)"},
    {"connecting…", "подключение…"},
    {"Copied: %1", "Скопировано: %1"},
    {"Copy", "Копировать"},
    {"Could not copy the text to the clipboard", "Не удалось скопировать текст в буфер обмена"},
    {"Could not delete %1", "Не удалось удалить файл %1"},
    {"Could not download the model:\n%1", "Не удалось скачать модель:\n%1"},
    {"Could not load the model", "Не удалось загрузить модель"},
    {"Could not open the microphone", "Не удалось открыть микрофон"},
    {"Could not save the model file", "Не удалось сохранить файл модели"},
    {"Ctrl+Shift+V (terminals)", "Ctrl+Shift+V (терминалы)"},
    {"custom model", "своя модель"},
    {"Custom server…", "Свой сервер…"},
    {"Delete", "Удалить"},
    {"Delete model", "Удалить модель"},
    {"Delete the “%1” model from disk?", "Удалить модель «%1» с диска?"},
    {"Desktop portal (KDE, GNOME, COSMIC)", "Портал рабочего стола (KDE, GNOME, COSMIC)"},
    {"Detect automatically", "Определять автоматически"},
    {"Do not check the published SHA-256 (my own models)", "Не сверять опубликованную SHA-256 (мои модели)"},
    {"Download", "Скачать"},
    {"Download a model in Settings first", "Сначала скачайте модель в настройках"},
    {"Download log:", "Журнал загрузки:"},
    {"Downloaded — %1", "Скачана — %1"},
    {"Downloading %1", "Загрузка %1"},
    {"Drag the indicator with the mouse. Click to stop recording, right-click to cancel.", "Индикатор можно перетащить мышью. Клик — остановить запись, правый клик — отменить."},
    {"enter the server address below", "введите адрес сервера ниже"},
    {"Failed: %1", "Ошибка: %1"},
    {"Files are fetched from ADDRESS/REPOSITORY/resolve/main/FILE, as on Hugging Face", "Файлы берутся с АДРЕС/РЕПОЗИТОРИЙ/resolve/main/ФАЙЛ, как на Hugging Face"},
    {"Finds the language itself; runs on the processor.", "Язык определяет сама; работает на процессоре."},
    {"Follow system", "Как в системе"},
    {"For example: Kubernetes, Grafana, Jane Doe", "Например: Kubernetes, Grafana, Анна Сергеевна"},
    {"General", "Общие"},
    {"granted", "разрешён"},
    {"https://your-server.example/ (same layout as huggingface.co)", "https://ваш-сервер.example/ (структура как у huggingface.co)"},
    {"Indicator:", "Индикатор:"},
    {"Input access:", "Доступ к вводу:"},
    {"Interface language", "Язык интерфейса"},
    {"Interface language:", "Язык интерфейса:"},
    {"Language:", "Язык:"},
    {"List the models and exit", "Показать модели и выйти"},
    {"Microphone:", "Микрофон:"},
    {" min", " мин"},
    {"Mirror hf-mirror.com", "Зеркало hf-mirror.com"},
    {"Model download", "Загрузка модели"},
    {"Model downloaded", "Модель скачана"},
    {"Model for --transcribe (default: the one in the settings)", "Модель для --transcribe (по умолчанию — из настроек)"},
    {"Model:", "Модель:"},
    {"more accurate, slow on a CPU", "точнее, но на процессоре медленно"},
    {"most accurate Whisper, slow", "самый точный Whisper, медленно"},
    {" ms", " мс"},
    {"Names and terms you often say — Whisper will spell them correctly.", "Имена и термины, которые часто звучат в речи, — Whisper будет писать их правильно."},
    {"never", "никогда"},
    {"No GPU for Whisper: install a Vulkan driver (on Arch also ggml-vulkan). GigaAM and Parakeet are fast on the processor anyway.", "Видеокарты для Whisper нет: установите драйвер Vulkan (в Arch также ggml-vulkan). GigaAM и Parakeet и так быстры на процессоре."},
    {"No input access — the text is in the clipboard, paste it yourself", "Нет доступа к вводу — текст в буфере обмена, вставьте его вручную"},
    {"No microphone found", "Микрофон не найден"},
    {"No speech heard — check the microphone", "Не слышно речи — проверьте микрофон"},
    {"not a valid address (https://…)", "неверный адрес (нужен https://…)"},
    {"not a WAV file", "это не WAV-файл"},
    {"not assigned", "не назначена"},
    {"Not downloaded — %1", "Не скачана — %1"},
    {"not requested yet", "ещё не запрошен"},
    {"Nothing can press keys here: install wtype or ydotool — the text is in the clipboard", "Нечем нажать клавиши: установите wtype или ydotool — текст в буфере обмена"},
    {"Nothing recognized", "Ничего не распознано"},
    {"On %1 add to %2:", "В %1 добавьте в %2:"},
    {"On %1 bind a key yourself — add to %2:", "В %1 клавишу назначают вручную — добавьте в %2:"},
    {"Only copy to the clipboard", "Только копировать в буфер обмена"},
    {"Only for a custom server that hosts files other than the official ones. Official models are always verified.", "Только для своего сервера с файлами, отличающимися от официальных. Официальные модели проверяются всегда."},
    {"Open settings", "Открыть настройки"},
    {"Open the models folder", "Открыть папку моделей"},
    {"or hold to talk:", "или говорить, удерживая клавишу:"},
    {"Paste into the active window", "Вставлять в активное окно"},
    {"Paste keys:", "Сочетание вставки:"},
    {"Pause that ends a phrase:", "Пауза в конце фразы:"},
    {"Phrases are recognised while you go on talking. Off: everything is pasted when the recording stops.", "Фразы распознаются, пока вы продолжаете говорить. Если выключено — весь текст вставляется после остановки записи."},
    {"Press %1 and speak.", "Нажмите %1 и говорите."},
    {"Press keys with:", "Нажимать клавиши через:"},
    {"Print the text of a WAV file and exit", "Распознать WAV-файл, вывести текст и выйти"},
    {"Processor only", "Только процессор"},
    {"Quit", "Выход"},
    {"Quit the running Sotto", "Закрыть запущенный Sotto"},
    {"Ready for dictation.", "Всё готово к диктовке."},
    {"Recognises Russian; runs on the processor.", "Распознаёт русский; работает на процессоре."},
    {"Recognition", "Распознавание"},
    {"Recognition error (%1)", "Ошибка распознавания (%1)"},
    {"Recording and pasting", "Запись и вставка"},
    {"Redirected to %1", "Перенаправление на %1"},
    {"Removal:", "Удаление:"},
    {"Remove Sotto", "Удалить Sotto"},
    {"Remove Sotto from this computer", "Удалить Sotto с компьютера"},
    {"Remove Sotto from this computer?\n\nThe program, its menu entry, autostart and shortcut will be removed.", "Удалить Sotto с компьютера?\n\nБудут удалены программа, ярлык в меню, автозапуск и горячая клавиша."},
    {"Remove Sotto…", "Удалить Sotto…"},
    {"Reset position", "Вернуть на место"},
    {"Restart Sotto to apply the language.", "Перезапустите Sotto, чтобы применить язык."},
    {"Result:", "Результат:"},
    {"Saved %1", "Сохранено: %1"},
    {"Server:", "Сервер:"},
    {"Settings", "Настройки"},
    {"Settings → Keyboard → Keyboard Shortcuts → Custom Shortcuts → Add Shortcut", "Настройки → Клавиатура → Сочетания клавиш → Свои сочетания → Добавить"},
    {"Settings → Keyboard → View and Customize Shortcuts → Custom Shortcuts", "Настройки → Клавиатура → Комбинации клавиш → Дополнительные комбинации"},
    {"Settings…", "Настройки…"},
    {"SHA-256 matches the published value (%1)", "SHA-256 совпала с опубликованной (%1)"},
    {"Shortcut:", "Горячая клавиша:"},
    {"Show how to bind dictation to a key on this desktop", "Показать, как назначить клавишу диктовки на этом рабочем столе"},
    {"slightly more accurate, more memory", "чуть точнее, больше памяти"},
    {"Sotto is installed in %1. Remove it with your distribution's tools.", "Программа установлена в %1. Удалите её средствами вашего дистрибутива."},
    {"Sotto was removed.", "Sotto удалён."},
    {"Sotto — recording", "Sotto — идёт запись"},
    {"Sotto — transcribing…", "Sotto — распознаю…"},
    {"Sotto — voice typing", "Sotto — голосовой ввод"},
    {"Sotto — voice typing for Linux", "Sotto — голосовой ввод для Linux"},
    {"Source:", "Источник:"},
    {"Source: %1", "Источник: %1"},
    {"Start at login", "Запускать при входе в систему"},
    {"Start dictation", "Начать диктовку"},
    {"Start or stop dictation", "Начать или закончить диктовку"},
    {"Stop and paste", "Остановить и вставить"},
    {"Stop dictation and paste the text", "Закончить диктовку и вставить текст"},
    {"System default", "Системный по умолчанию"},
    {"System Settings → Keyboard → Shortcuts → Add New → Command or Script", "Параметры системы → Клавиатура → Комбинации клавиш → Добавить → Команда или сценарий"},
    {"Tap: records until the next tap. Hold: records while the key is held. Any key can also run the command “sotto --toggle”.", "Нажмите и отпустите — запись до следующего нажатия. Удерживайте — запись, пока клавиша нажата. Любую клавишу можно назначить на команду «sotto --toggle»."},
    {"The AMD graphics have no Vulkan driver: install vulkan-radeon on Arch, mesa-vulkan-drivers on Debian, Ubuntu or Fedora.", "Для видеокарты AMD нет драйвера Vulkan: установите vulkan-radeon в Arch, mesa-vulkan-drivers в Debian, Ubuntu или Fedora."},
    {"The custom server address is missing or not valid (use https://…).", "Адрес своего сервера не указан или неверен (нужен https://…)."},
    {"The discrete GPU is not used", "Дискретная видеокарта не используется"},
    {"The download is incomplete (%1 of %2 bytes)", "Файл скачался не полностью (%1 из %2 байт)"},
    {"The download log appears here: server, redirects, checksum.", "Здесь появится журнал загрузки: сервер, перенаправления, контрольная сумма."},
    {"The downloaded file does not match the published checksum, so it was deleted. Try again or switch the source.", "Скачанный файл не совпадает с опубликованной контрольной суммой и удалён. Попробуйте ещё раз или смените источник."},
    {"the GlobalShortcuts portal is unavailable", "портал глобальных сочетаний недоступен"},
    {"The GPU is not used", "Видеокарта не используется"},
    {"The GPU part of ggml is not installed: install ggml-vulkan on Arch.", "Не установлена часть ggml для видеокарт: в Arch установите ggml-vulkan."},
    {"The Intel graphics have no Vulkan driver: install vulkan-intel on Arch, mesa-vulkan-drivers on Debian, Ubuntu or Fedora.", "Для графики Intel нет драйвера Vulkan: установите vulkan-intel в Arch, mesa-vulkan-drivers в Debian, Ubuntu или Fedora."},
    {"the keyboard shortcut settings of your desktop", "настройки сочетаний клавиш вашего рабочего стола"},
    {"The microphone was disconnected", "Микрофон отключился"},
    {"The model “%1” is not downloaded: choose one in the settings or with --model (see --models).", "Модель «%1» не скачана: выберите её в настройках или через --model (список — --models)."},
    {"the most accurate for Russian, with punctuation, fast on a CPU", "самая точная для русского, с пунктуацией, быстро на процессоре"},
    {"The NVIDIA card has no driver loaded. It may be switched off (integrated-only mode in envycontrol, supergfxctl or optimus-manager: choose hybrid), or the NVIDIA driver is not installed.", "Драйвер видеокарты NVIDIA не загружен. Возможно, она выключена (режим «только встроенная» в envycontrol, supergfxctl или optimus-manager — выберите гибридный) или драйвер NVIDIA не установлен."},
    {"The NVIDIA card runs on the nouveau driver, which gives it no Vulkan here. Install the NVIDIA driver, or Mesa's NVK (vulkan-nouveau) for recent cards.", "Видеокарта NVIDIA работает на драйвере nouveau, и Vulkan для неё нет. Установите драйвер NVIDIA или, для новых карт, NVK из Mesa (vulkan-nouveau)."},
    {"The NVIDIA driver is installed but Vulkan does not see the card. Run “vulkaninfo --summary” in a terminal to see why; on Arch, ggml-cuda is another way to use it.", "Драйвер NVIDIA установлен, но Vulkan не видит видеокарту. Запустите в терминале «vulkaninfo --summary», чтобы узнать причину; в Arch видеокарту можно задействовать и через ggml-cuda."},
    {"The NVIDIA driver was updated (kernel module %1, libraries %2): restart the computer.", "Драйвер NVIDIA обновился (модуль ядра %1, библиотеки %2): перезагрузите компьютер."},
    {"The package was not removed. Run it yourself:\n%1", "Пакет не удалён. Выполните вручную:\n%1"},
    {"the portal did not bind the shortcut", "портал не привязал горячую клавишу"},
    {"the portal refused", "отказ портала"},
    {"the portal rejected the shortcut", "привязка горячей клавиши отклонена порталом"},
    {"The system asks once for “remote control” permission: Sotto uses it to press the paste keys. Tick “Remember”.", "Система один раз спросит разрешение на «удалённое управление»: через него Sotto нажимает сочетание вставки. Отметьте «Запомнить»."},
    {"The Vulkan part of the NVIDIA driver is not installed: nvidia-utils on Arch, nvidia-vulkan-icd on Debian, libnvidia-gl on Ubuntu.", "Не установлена часть драйвера NVIDIA для Vulkan: nvidia-utils в Arch, nvidia-vulkan-icd в Debian, libnvidia-gl в Ubuntu."},
    {"This build of Sotto has no sherpa-onnx: choose a Whisper model", "Эта сборка Sotto без sherpa-onnx: выберите модель Whisper"},
    {"this desktop", "этот рабочий стол"},
    {"Type each phrase as soon as I pause", "Печатать каждую фразу, как только я делаю паузу"},
    {"Typing:", "Ввод:"},
    {"unavailable: %1", "недоступна: %1"},
    {"Unload the model after:", "Выгружать модель через:"},
    {"unsupported WAV format (use 16-bit PCM or 32-bit float)", "неподдерживаемый формат WAV (нужен 16-битный PCM или 32-битный float)"},
    {"very fast, average accuracy", "очень быстро, точность средняя"},
    {"Vocabulary:", "Словарь:"},
    {"Voice typing", "Голосовой ввод"},
    {"Whisper for a CPU", "Whisper для процессора"},
    {"whisper.cpp is incomplete: its ggml backends are missing. Reinstall Sotto.", "whisper.cpp установлен не полностью: нет его модулей ggml. Переустановите Sotto."},
    {"wtype (Hyprland, niri, sway, river)", "wtype (Hyprland, niri, sway, river)"},
    {"xdotool (X11)", "xdotool (X11)"},
    {"ydotool (any desktop, needs ydotoold)", "ydotool (любой рабочий стол, нужен ydotoold)"},
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
