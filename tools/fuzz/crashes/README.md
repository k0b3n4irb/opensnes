# Regression inputs from fuzzing

One file per bug the fuzzers found and we fixed, under `<target>/`. `make
-C tools/fuzz replay` runs each through its harness on every push (part of
the sanitizer job): a fixed crash must stay fixed. Name them after the fix
commit or the issue (`itloader/short-header-<sha>.it`). Keep the
directories even when empty — `.keep` files hold them.
