#include "core/engine.h"
#include "core/models.h"
#include "core/whisperengine.h"
#ifdef SOTTO_HAVE_SHERPA
#include "core/sherpaengine.h"
#endif

#include <QCoreApplication>

std::unique_ptr<Engine> Engine::load(const ModelInfo &model, int gpu, QString *error)
{
    switch (model.backend) {
    case Backend::Sherpa:
#ifdef SOTTO_HAVE_SHERPA
        return SherpaEngine::load(Models::path(model.id), error);
#else
        *error = QCoreApplication::translate("Engine", "This build of Sotto has no sherpa-onnx: choose a Whisper model");
        return nullptr;
#endif
    case Backend::Whisper:
        return WhisperEngine::load(Models::path(model.id), gpu, error);
    case Backend::Vad:
        break;
    }
    *error = QCoreApplication::translate("Engine", "Could not load the model");
    return nullptr;
}
