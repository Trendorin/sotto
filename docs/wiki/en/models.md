# Models

Sotto has two recognition engines. **sherpa-onnx** runs GigaAM and Parakeet: newer models that are
more accurate than Whisper and fast on an ordinary processor. **whisper.cpp** runs Whisper: 99
languages, and the GPU through Vulkan.

| model | engine | size | for |
|---|---|---|---|
| **GigaAM v3** | sherpa-onnx | 221 MB | Russian: the most accurate open model, with punctuation, fast on a CPU |
| **Parakeet TDT v3** | sherpa-onnx | 639 MB | 25 European languages with Russian, Ukrainian and English, punctuation, fast on a CPU |
| **Whisper Large v3 Turbo** | whisper.cpp | 547 MB | 99 languages, accurate and fast on a GPU |
| Whisper Large v3 Turbo Q8 | whisper.cpp | 834 MB | slightly more accurate, more memory |
| Whisper Large v3 | whisper.cpp | 1031 MB | the most accurate Whisper, slow |
| Whisper Medium | whisper.cpp | 514 MB | more accurate, slow on a CPU |
| Whisper Small | whisper.cpp | 181 MB | Whisper for a CPU |
| Whisper Base | whisper.cpp | 57 MB | very fast, average accuracy |

The settings recommend GigaAM when you speak Russian, Whisper Large v3 Turbo with a GPU, and
Parakeet otherwise. Download a model in **Settings → Recognition**. Only the model you use is
loaded, and it is unloaded again after a while without use.

## Which one

- **Russian**: GigaAM v3. It writes punctuation, capital letters and «ё», and an 11-second phrase
  takes about a tenth of its length to recognise on two CPU cores. English words come out in
  Cyrillic; if you mix languages a lot, take Parakeet.
- **Several European languages, or Russian with English terms**: Parakeet TDT v3. It finds the
  language of every phrase itself.
- **Any other language, or a strong GPU**: Whisper Large v3 Turbo. Set **Language** if you
  always speak the same one, and put names and terms into **Vocabulary**.

**Language**, **Acceleration** and **Vocabulary** are Whisper settings; GigaAM and Parakeet know
their languages and run on the processor.

## Phrases

A dictation is cut into phrases at your pauses by [Silero VAD](https://github.com/snakers4/silero-vad)
(0.9 MB, downloaded with the first model). Each phrase is recognised while you go on talking, so
when you stop only the last one is left. Silero also keeps recognition away from silence, where
Whisper tends to invent text. Without the VAD model Sotto cuts at quiet moments instead.

## Sources

| files | from | license |
|---|---|---|
| GigaAM v3 | [csukuangfj/sherpa-onnx-nemo-transducer-punct-giga-am-v3-russian-2025-12-16](https://huggingface.co/csukuangfj/sherpa-onnx-nemo-transducer-punct-giga-am-v3-russian-2025-12-16) ([GigaAM](https://github.com/salute-developers/GigaAM)) | MIT |
| Parakeet TDT v3 | [csukuangfj/sherpa-onnx-nemo-parakeet-tdt-0.6b-v3-int8](https://huggingface.co/csukuangfj/sherpa-onnx-nemo-parakeet-tdt-0.6b-v3-int8) ([NVIDIA](https://huggingface.co/nvidia/parakeet-tdt-0.6b-v3)) | CC-BY-4.0 |
| Whisper | [ggerganov/whisper.cpp](https://huggingface.co/ggerganov/whisper.cpp) | MIT |
| Silero VAD | [ggml-org/whisper-vad](https://huggingface.co/ggml-org/whisper-vad) | MIT |

If Hugging Face is unreachable, switch **Source** to the [hf-mirror.com](https://hf-mirror.com)
mirror. Either way, every file is kept only if its size and SHA-256 match the published ones; a
file that does not match is deleted. Files already downloaded and checked are not fetched again,
so an interrupted download of a model with several files continues where it stopped.

## Your own server

Pick **Custom server…** as the source and enter the address of a server that lays files out like
huggingface.co: `<address>/<repository>/resolve/main/<file>`, with the repositories from the table
above (for example `<address>/ggerganov/whisper.cpp/resolve/main/ggml-small-q5_1.bin`). The
address must start with `https://` (`http://` only for localhost). Official models are still
checked against their published SHA-256; tick **Do not check the published SHA-256** only if your
server hosts files that differ from the official ones.

## Download log

While a model downloads, the settings show a log: the source, the exact URL of every file, where
the server redirected to (Hugging Face serves files from its CDN), the checksum result and where
the file was saved. The same lines are appended to `~/.local/share/sotto/download.log` and the
journal (`journalctl --user -t sotto`). The source and checksum options are locked while a
download runs.

## Your own files

Everything lives in `~/.local/share/sotto/models/`:

- a Whisper model: any `ggml-<name>.bin`;
- a sherpa-onnx NeMo transducer: a folder `<name>/` with `encoder*.onnx`, `decoder*.onnx`,
  `joiner*.onnx` and `tokens.txt` (for example another model from the
  [sherpa-onnx releases](https://github.com/k2-fsa/sherpa-onnx/releases/tag/asr-models)).

They appear at the end of the model list. `sotto --models` lists everything Sotto sees.

## Trying a model on a file

```sh
sotto --transcribe recording.wav                      # with the model from the settings
sotto --transcribe recording.wav --model parakeet-v3  # or any id from sotto --models
```

The text goes to the standard output, the length, phrase count and time to the standard error.
WAV files with 16-, 24- or 32-bit PCM or 32-bit float, any sample rate, mono or stereo.
