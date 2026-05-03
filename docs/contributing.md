---
title: Contributing
parent: Community
nav_order: 1
---

# Preface

LUS accepts any and all contributions. You can interact with the project via PRs, issues, email (kenixwhisperwind@gmail.com), or [Discord](https://discord.gg/shipofharkinian).

# Code of Conduct

Please review and abide by our [Code of Conduct](code-of-conduct).

# Building

Please see the [Overview](index) page for building instructions.

# Pull Requests

## Procedures

Our CI system will automatically check your PR to ensure that it fits [formatting guidelines](https://github.com/Kenix3/libultraship/blob/main/.clang-format), [linter guidelines](https://github.com/Kenix3/libultraship/blob/main/.clang-tidy), and that the code builds. The submitter of a PR is required to ensure their PR passes all CI checks. The submitter of the PR is encouraged to address all PR review comments in a timely manner to ensure a timely merge of the PR.

### Troubleshooting CI Errors

#### tidy-format-validation

If the tidy-format-validation check fails, then you need to run clang-format. Below is a command that can be used on Linux. Most modern IDEs have a clang-format plugin that can be used. The [.clang-format file can be found here](https://github.com/Kenix3/libultraship/blob/main/.clang-format).

##### Running clang-format

###### From within the LUS directory.

Ensure clang-format-14 is installed and available through your system path.

Bash:
```
find src include -name "*.cpp" -o -name "*.h" | sed 's| |\\ |g' | xargs clang-format-14 -i
```

Powershell:
```
Get-ChildItem -Recurse -Path .\src -Include *.cpp, *.h -File | ForEach-Object { clang-format-14.exe -i $_.FullName }
```

##### Running clang-tidy

###### From within the LUS directory.

Ensure `clang-tidy` is installed and available through your system path.

First, generate a `compile_commands.json` so clang-tidy can resolve includes:
```
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --target GenerateScriptKeys
```

Then run clang-tidy-diff on your local changes:

Bash:
```
git diff -U0 HEAD -- src include | clang-tidy-diff -p1 -path build
```

To check a specific file:
```
clang-tidy -p build src/path/to/file.cpp
```
