#pragma once

#include <QString>

// Commands that print to the terminal and exit, without the tray or a running Sotto.
namespace Cli {

// Recognises a WAV file (any rate, mono or stereo, 16-bit or float) with the configured or the given
// model and prints the text. Exit code 0 on success.
int transcribe(const QString &file, const QString &modelId);
int listModels();
int keybind();

} // namespace Cli
