# Models

| model | size | for |
|---|---|---|
| Base | 57 MB | very fast, average accuracy |
| Small | 181 MB | the best choice without a GPU |
| Medium | 514 MB | more accurate, slow on a CPU |
| **Large v3 Turbo** | 547 MB | accurate and fast on a GPU |
| Large v3 Turbo Q8 | 834 MB | slightly more accurate, more memory |
| Large v3 | 1080 MB | most accurate, slow |

Download a model in **Settings → Recognition**. Only the model you use is loaded, and it is
unloaded again after a while without use.

## Sources

Whisper models come from [huggingface.co/ggerganov/whisper.cpp](https://huggingface.co/ggerganov/whisper.cpp),
Silero VAD (0.9 MB) from [huggingface.co/ggml-org/whisper-vad](https://huggingface.co/ggml-org/whisper-vad),
both under the MIT license. If Hugging Face is unreachable, switch **Source** to the
[hf-mirror.com](https://hf-mirror.com) mirror.

Either way, a download is kept only if its size and SHA-256 match the published ones; a file
that does not match is deleted.

## Your own files

Put any `ggml-*.bin` into `~/.local/share/sotto/models/` and pick it in the settings.

## Language

**Detect automatically** works for most speech. If you always speak one language, choosing it
makes recognition a little faster and avoids wrong guesses on short phrases.
