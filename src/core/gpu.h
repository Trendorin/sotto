#pragma once

#include <QList>
#include <QString>

// The graphics cards ggml (whisper.cpp's compute library) can use, and why one is missing.
namespace Gpu {

struct Device {
    int index = -1;      // among ggml's GPUs: the numbering of whisper_context_params::gpu_device
    QString key;         // "<backend>/<description>", what the settings store
    QString name;
    bool discrete = false;
    quint64 memory = 0;  // bytes
};

// Loads the ggml backends once (CPU variants, Vulkan, CUDA, whatever is installed).
void loadBackends();
// False when not even ggml's CPU backend could be loaded: whisper.cpp and its VAD cannot run.
bool haveCpuBackend();
// GPUs ggml can use, best first.
const QList<Device> &devices();
// The GPU for a stored key: that one, or the best one when the key is empty or the GPU is gone.
// index is -1 when there is no GPU.
Device find(const QString &key);
// The GPU the settings ask for, -1 for the CPU.
int configured();
// Why a graphics card in the computer is not among devices(); empty when nothing is missing.
QString missingHint();

} // namespace Gpu
