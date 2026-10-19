# Security policy

Sotto listens to your microphone, downloads model files and types into other windows, so
security problems matter. Please report them privately.

## Reporting

Use GitHub's private vulnerability reporting:
[Report a vulnerability](https://github.com/Trendorin/sotto/security/advisories/new).
Please don't open a public issue for a security problem. You will get an answer within a week.

## What counts

- recording or keeping audio when you did not start dictation, or writing it to disk;
- anything sent over the network apart from the model download you start;
- a model file that is used although its size or SHA-256 differs from the published one;
- another local user or program controlling a running Sotto through its socket;
- the uninstaller or the AppImage integration touching files that are not Sotto's.

## Supported versions

Security fixes go into the latest release.
