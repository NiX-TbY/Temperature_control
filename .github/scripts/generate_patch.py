#!/usr/bin/env python3
"""
generate_patch.py
=================

Called from ai-fix.yml.  Usage:
  python .github/scripts/generate_patch.py build-log/build-log.txt

Reads the failing PlatformIO compile log, asks GPT-4o to produce a
unified diff that fixes the errors, and writes it to ./patch.diff.
The CI job then applies that diff and opens a pull-request.
"""

import os
import sys
import pathlib
import textwrap
import openai  # pip install openai (handled in the workflow)

# ----------------------------------------------------------------------
# 1. Load OpenAI API key (must be stored as the secret OPENAI_API_KEY)
# ----------------------------------------------------------------------
try:
    openai.api_key = os.environ["OPENAI_API_KEY"]
except KeyError:
    sys.stderr.write("ERROR: OPENAI_API_KEY environment variable not set\n")
    sys.exit(1)

# ----------------------------------------------------------------------
# 2. Read the compile log passed as the first CLI argument
# ----------------------------------------------------------------------
if len(sys.argv) != 2:
    sys.stderr.write("Usage: generate_patch.py <path-to-build-log.txt>\n")
    sys.exit(1)

log_path = pathlib.Path(sys.argv[1])
if not log_path.is_file():
    sys.stderr.write(f"ERROR: log file not found: {log_path}\n")
    sys.exit(1)

compile_log = log_path.read_text(encoding="utf-8", errors="ignore")

# Optionally trim super-long logs to keep prompt under model limits
MAX_CHARS = 15_000
if len(compile_log) > MAX_CHARS:
    compile_log = compile_log[-MAX_CHARS:]

# ----------------------------------------------------------------------
# 3. Build the prompt
# ----------------------------------------------------------------------
prompt = textwrap.dedent(
    f"""
    You are an expert embedded C++ engineer.

    The following PlatformIO compile log failed.
    Produce **only** a unified git diff (no prose and no markdown fences)
    that fixes the errors without touching unrelated code.
    Use context lines (@@ … @@) so `git apply` can locate the patch.

    ### Compile log
    {compile_log}
    """
).strip()

# ----------------------------------------------------------------------
# 4. Call GPT-4o (or switch to a different model if you prefer)
# ----------------------------------------------------------------------
response = openai.chat.completions.create(
    model="gpt-4o",
    messages=[{"role": "user", "content": prompt}],
    temperature=0.0,
    max_tokens=4096,
)

patch_text = response.choices[0].message.content.strip()

# ----------------------------------------------------------------------
# 5. Save the diff to patch.diff (even if empty, so workflow can test)
# ----------------------------------------------------------------------
out_path = pathlib.Path("patch.diff")
out_path.write_text(patch_text, encoding="utf-8")
print(f"Patch written to {out_path.resolve()}")
@@ prompt = textwrap.dedent(
-    You are an expert embedded C++ engineer.
-
-    The following PlatformIO compile log failed.
-    Produce **only** a unified git diff (no prose and no markdown fences)
-    that fixes the errors without touching unrelated code.
-    Use context lines (@@ … @@) so `git apply` can locate the patch.
+    You are an expert embedded C++ engineer.
+
+    The build failed.  When the ONLY errors are “declaration not found”
+    (e.g.  ‘fooBar was not declared in this scope’  or
+           ‘undefined reference to fooBar’),
+    create **minimal stub code** so it compiles:
+
+      • add a forward declaration & empty body for functions
+        (e.g.  `void fooBar() { /* TODO */ }`),
+      • or add a `#include "header.h"` if that header already exists,
+      • or add a missing `extern` variable definition.
+
+    Otherwise, produce the usual targeted fix.
+
+    Respond **only** with a unified git diff (no prose, no markdown).
+    Use context lines (@@ … @@) so `git apply` can locate the patch.

