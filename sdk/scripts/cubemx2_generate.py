#!/usr/bin/env python3
# Copyright (c) 2026 Analog Devices, Inc.
# SPDX-License-Identifier: BSD-3-Clause

"""Generate a CMake project from an .ioc2 file with the STM32CubeMX2 CLI.

This is the CubeMX2 equivalent of `STM32CubeMX -q <script>.cubemx`.
See sdk/stm32_cubemx2_sdk.cmake for the caller.

Usage:
    cubemx2_generate.py <cube_bin> <ioc2_path> <destination_dir>
"""
import os
import re
import subprocess
import sys


# Pitfalls of the `cube mx` CLI:
#
# 1. ELECTRON_RUN_AS_NODE must not be set. `cube mx` starts an Electron backend.
#    With this variable, Electron runs as plain Node and stops with "bad option:
#    --no-sandbox". `cube mx start` still returns 0, so the failure is silent
#    and later commands time out. VS Code and other Electron IDEs export the
#    variable in their terminals, so a build can work in a shell and fail in an
#    IDE. The script removes the variable.
#
# 2. Do not start a backend and do not pass --port. Each `cube mx` command finds
#    a running backend or starts its own `--guiless` backend (about 2 s). There
#    is no backend to stop, so an interrupted build leaves no Electron process.
#
# 3. The relative pack references of the generated tree do not use one base.
#    See absolutize_external_refs().

# This mode keeps the CMSIS packs in place and references them with relative
# paths. The alternative copies the vendor tree (about 100 MB) into the build
# directory at each generation.
SOURCE_MODE = "reuse-local-with-external-refs"


def cube_env():
    env = os.environ.copy()
    env.pop("ELECTRON_RUN_AS_NODE", None)
    return env


def run_step(cube_bin, args):
    proc = subprocess.run([cube_bin, "mx"] + args, env=cube_env(),
                          capture_output=True, text=True, timeout=900)
    if proc.stdout:
        sys.stdout.write(proc.stdout)
    if proc.stderr:
        sys.stderr.write(proc.stderr)
    # The CLI can fail with a non-zero exit code, or with exit code 0 and a
    # JSON body with "status": "error". Check for both.
    if proc.returncode != 0 or '"status": "error"' in proc.stdout:
        print(f"cubemx2_generate: step failed: {' '.join(args)}",
              file=sys.stderr)
        return proc.returncode or 1
    return 0


# Generated CMake fragments with relative paths to the pack store
EXTERNAL_REF_FILES = ("cmake/components.cmake", "cmake/flags.cmake")

# A reference is "../" repeated, then a path. A quoted path can have spaces
# (macOS packs are in "Application Support"). An unquoted path has no spaces,
# quotes or ')'.
_REL_REF_RE = re.compile(r"(?<=\")(?:\.\./)+[^\"\n]+(?=\")|(?:\.\./)+[^\s)\"']+")


def absolutize_external_refs(destination, ioc2_path):
    """Rewrite the relative pack references of the generated tree as absolute paths.

    CubeMX2 does not use one base for the references. components.cmake uses
    paths relative to the destination directory. flags.cmake uses the absolute
    pack path with the leading "/" replaced by "../../". That path is correct
    only for a destination two levels below the filesystem root. A deeper build
    directory gets include paths that do not exist, and the first header that
    includes a pack header fails.

    The function resolves each reference against three bases and keeps the
    one that exists. It writes the result back as an absolute path.
    """
    bases = (os.path.abspath(destination),
             os.path.dirname(os.path.abspath(ioc2_path)),
             os.sep)

    def replace(match):
        for base in bases:
            resolved = os.path.normpath(os.path.join(base, match.group(0)))
            if os.path.exists(resolved):
                return resolved
        # Unknown path: keep it as generated
        return match.group(0)

    for rel in EXTERNAL_REF_FILES:
        path = os.path.join(destination, rel)
        if not os.path.exists(path):
            continue
        with open(path) as f:
            original = f.read()
        rewritten = _REL_REF_RE.sub(replace, original)
        if rewritten != original:
            with open(path, "w") as f:
                f.write(rewritten)


def main():
    if len(sys.argv) != 4:
        print(__doc__, file=sys.stderr)
        return 2
    cube_bin, ioc2_path, destination = sys.argv[1:4]

    fmt = ["--format", "CMake"]
    steps = [
        ["ide-project", "set-format", "-p", ioc2_path] + fmt,
        ["ide-project", "set-source", "-p", ioc2_path] + fmt +
        ["--source", SOURCE_MODE],
        ["ide-project", "set-destination", "-p", ioc2_path] + fmt +
        ["--destination", destination],
        ["ide-project", "generate", "-p", ioc2_path] + fmt +
        ["--destination", destination, "--force"],
    ]
    for step in steps:
        rc = run_step(cube_bin, step)
        if rc:
            return rc

    absolutize_external_refs(destination, ioc2_path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
