#include "core/gpu.h"
#include "core/config.h"
#include "core/log.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>

#include <ggml-backend.h>
#include <whisper.h>

#include <algorithm>
#include <mutex>

namespace Gpu {

static void quietLog(ggml_log_level level, const char *text, void *)
{
    if (level == GGML_LOG_LEVEL_ERROR || qEnvironmentVariableIsSet("SOTTO_DEBUG"))
        fputs(text, stderr);
}

// Packages that carry their own whisper.cpp name the backend folder at build time;
// otherwise backends live in <prefix>/lib/ggml, both in /usr and inside the AppImage.
static QString backendDir()
{
#ifdef SOTTO_GGML_BACKEND_DIR
    return QStringLiteral(SOTTO_GGML_BACKEND_DIR);
#else
    return QCoreApplication::applicationDirPath() + QStringLiteral("/../lib/ggml");
#endif
}

void loadBackends()
{
    static std::once_flag once;
    std::call_once(once, [] {
        ggml_log_set(quietLog, nullptr);
        whisper_log_set(quietLog, nullptr);
        const QString dir = backendDir();
        if (QDir(dir).exists())
            ggml_backend_load_all_from_path(QDir(dir).canonicalPath().toLocal8Bit().constData());
        else
            ggml_backend_load_all();
    });
}

bool haveCpuBackend()
{
    loadBackends();
    static const bool cpu = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_CPU) != nullptr;
    if (!cpu)
        qCWarning(lcSotto) << "no ggml CPU backend in" << backendDir();
    return cpu;
}

static QString hintFor(const QList<Device> &found);

// whisper.cpp takes the first GPU ggml lists, and on laptops with two the integrated one usually
// comes first. List discrete GPUs first, then the ones with more memory.
const QList<Device> &devices()
{
    static const QList<Device> list = [] {
        loadBackends();
        QList<Device> found;
        for (size_t i = 0; i < ggml_backend_dev_count(); ++i) {
            ggml_backend_dev_t dev = ggml_backend_dev_get(i);
            const auto type = ggml_backend_dev_type(dev);
            if (type != GGML_BACKEND_DEVICE_TYPE_GPU && type != GGML_BACKEND_DEVICE_TYPE_IGPU)
                continue;
            size_t free = 0, total = 0;
            ggml_backend_dev_memory(dev, &free, &total);
            Device d;
            d.index = int(found.size());
            d.name = QString::fromUtf8(ggml_backend_dev_description(dev));
            d.key = QString::fromUtf8(ggml_backend_reg_name(ggml_backend_dev_backend_reg(dev))) + QLatin1Char('/') + d.name;
            d.discrete = type == GGML_BACKEND_DEVICE_TYPE_GPU;
            d.memory = total;
            qCInfo(lcSotto) << "gpu" << d.index << d.key << (d.discrete ? "discrete" : "integrated") << total / (1024 * 1024) << "MiB";
            found.append(d);
        }
        std::stable_sort(found.begin(), found.end(), [](const Device &a, const Device &b) {
            return a.discrete != b.discrete ? a.discrete : a.memory > b.memory;
        });
        if (const QString hint = hintFor(found); !hint.isEmpty())
            qCWarning(lcSotto) << "missing gpu:" << hint;
        return found;
    }();
    return list;
}

Device find(const QString &key)
{
    const QList<Device> &list = devices();
    for (const Device &d : list)
        if (d.key == key)
            return d;
    return list.isEmpty() ? Device() : list.constFirst();
}

int configured()
{
    return Config::useGpu() ? find(Config::gpuDevice()).index : -1;
}

QString missingHint()
{
    return hintFor(devices());
}

static QString readSysfs(const QString &path)
{
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? QString::fromLatin1(f.readAll()).trimmed() : QString();
}

// Versions of the NVIDIA user-space driver installed, from its library names (libnvidia-glcore.so.580.95.05).
static QStringList nvidiaLibraryVersions()
{
    QStringList versions;
    for (const char *dir : {"/usr/lib", "/usr/lib64", "/usr/lib/x86_64-linux-gnu"})
        for (const QString &file : QDir(QString::fromLatin1(dir)).entryList({QStringLiteral("libnvidia-glcore.so.*")}, QDir::Files))
            versions.append(file.mid(int(qstrlen("libnvidia-glcore.so."))));
    return versions;
}

static bool haveVulkanIcd(const QString &pattern)
{
    for (const char *dir : {"/usr/share/vulkan/icd.d", "/etc/vulkan/icd.d", "/usr/local/share/vulkan/icd.d"})
        if (!QDir(QString::fromLatin1(dir)).entryList({pattern}, QDir::Files).isEmpty())
            return true;
    return false;
}

// A discrete card ggml does not list is the usual reason recognition runs on the integrated GPU or the
// CPU. Find the graphics cards on the PCI bus and say why the best one is missing.
static QString hintFor(const QList<Device> &found)
{
    const auto tr = [](const char *text) { return QCoreApplication::translate("Gpu", text); };
    if (std::any_of(found.begin(), found.end(), [](const Device &d) { return d.discrete; }))
        return {};
    const auto listed = [&](std::initializer_list<const char *> words) {
        return std::any_of(found.begin(), found.end(), [&](const Device &d) {
            return std::any_of(words.begin(), words.end(), [&](const char *w) { return d.name.contains(QLatin1String(w), Qt::CaseInsensitive); });
        });
    };
    bool nvidia = false, amd = false, intel = false;
    const QDir pci(QStringLiteral("/sys/bus/pci/devices"));
    for (const QString &dev : pci.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (!readSysfs(pci.filePath(dev + QStringLiteral("/class"))).startsWith(QLatin1String("0x03")))
            continue; // not a display controller
        const QString vendor = readSysfs(pci.filePath(dev + QStringLiteral("/vendor")));
        nvidia |= vendor == QLatin1String("0x10de");
        amd |= vendor == QLatin1String("0x1002");
        intel |= vendor == QLatin1String("0x8086");
    }

    // A backend that finds no device is not loaded at all, so look for its library instead.
    const QDir backends(backendDir());
    if ((nvidia || amd || intel) && backends.exists()
        && backends.entryList({QStringLiteral("libggml-vulkan.so"), QStringLiteral("libggml-cuda.so")}, QDir::Files).isEmpty())
        return tr("The GPU part of ggml is not installed: install ggml-vulkan on Arch.");
    if (nvidia) { // a card ggml lists would be discrete
        const QString kernel = readSysfs(QStringLiteral("/sys/module/nvidia/version"));
        if (kernel.isEmpty() && QDir(QStringLiteral("/sys/module/nouveau")).exists())
            return tr("The NVIDIA card runs on the nouveau driver, which gives it no Vulkan here. "
                      "Install the NVIDIA driver, or Mesa's NVK (vulkan-nouveau) for recent cards.");
        if (kernel.isEmpty())
            return tr("The NVIDIA card has no driver loaded. It may be switched off (integrated-only mode in "
                      "envycontrol, supergfxctl or optimus-manager: choose hybrid), or the NVIDIA driver is not installed.");
        const QStringList libraries = nvidiaLibraryVersions();
        if (!libraries.isEmpty() && !libraries.contains(kernel))
            return tr("The NVIDIA driver was updated (kernel module %1, libraries %2): restart the computer.")
                .arg(kernel, libraries.join(QStringLiteral(", ")));
        if (!haveVulkanIcd(QStringLiteral("nvidia*.json")))
            return tr("The Vulkan part of the NVIDIA driver is not installed: nvidia-utils on Arch, "
                      "nvidia-vulkan-icd on Debian, libnvidia-gl on Ubuntu.");
        return tr("The NVIDIA driver is installed but Vulkan does not see the card. "
                  "Run “vulkaninfo --summary” in a terminal to see why; on Arch, ggml-cuda is another way to use it.");
    }
    if (amd && !listed({"amd", "radeon"}))
        return tr("The AMD graphics have no Vulkan driver: install vulkan-radeon on Arch, mesa-vulkan-drivers on Debian, Ubuntu or Fedora.");
    if (intel && found.isEmpty())
        return tr("The Intel graphics have no Vulkan driver: install vulkan-intel on Arch, mesa-vulkan-drivers on Debian, Ubuntu or Fedora.");
    return {};
}

} // namespace Gpu
