#!/usr/bin/env python3
"""
Ask GPT-4o to turn a failing PlatformIO build log into a unified git diff.
Saves the result to patch.diff (even if the model returns no changes).
"""

import os, sys, pathlib, openai, textwrap

openai.api_key = os.environ["OPENAI_API_KEY"]
log_path = pathlib.Path(sys.argv[1])
LOG = log_path.read_text()

prompt = textwrap.dedent(f"""
You are an expert embedded C++ engineer. The following PlatformIO compile
log failed. Produce ONLY a unified git diff that fixes the errors without
changing unrelated code. Do not wrap the diff in Markdown.

### Compile log
{LOG}
""")

resp = openai.chat.completions.create(
    model="gpt-4o",
    messages=[{"role": "user", "content": prompt}],
    temperature=0
)

pathlib.Path("patch.diff").write_text(resp.choices[0].message.content.strip())
print("Patch written to patch.diff")
