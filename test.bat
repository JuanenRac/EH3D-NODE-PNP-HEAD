@echo off
rem Runs the host tests and the tests of the tools; it never changes the version.
cd /d "%~dp0"
cmake -S tests -B build\host || goto :end
cmake --build build\host || goto :end
ctest --test-dir build\host --output-on-failure || goto :end
python -I -m unittest discover -s tools -p "test_*.py"
:end
echo.
pause
