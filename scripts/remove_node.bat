@echo off
:: find and remove default node.exe to avoid conflicts
:: node: we will still rely on default installed npm (should work)
CALL node -e "console.log(process.execPath)" > node_path.txt
SET /p NODE_EXE_PATH=<node_path.txt
CALL del node_path.txt
CALL del /q /s "%NODE_EXE_PATH%"