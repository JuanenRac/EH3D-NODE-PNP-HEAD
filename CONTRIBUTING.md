# Contributing to EH3D-NODE-PNP-HEAD

Thank you for looking. This repository is at its first steps, so a contribution is most useful when it says plainly what it does and what has been tried.

1. **Say what runs and what is only planned.** Something counts as working on a machine only when it has been run on one; a test that passes on a computer is described as that.
2. **Before a pull request:** `python -I tools/ci_validate.py`, the tests of the tools (`python -I -m unittest discover -s tools -p "test_*.py"`) and the host tests (`cmake -S tests -B build/host && cmake --build build/host && ctest --test-dir build/host --output-on-failure`) must pass. A new requirement goes into `docs/REQUIREMENTS.md` with the test that shows it, and a new setting into the example settings file.
3. **Texts exist in eight languages** (English, Spanish, French, Italian, German, Chinese, Japanese and European Portuguese) in the README files, which keep the same structure.
4. Commits are written in English, small and about one thing.
