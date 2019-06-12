@echo off
title hello_world_script
cls
pause
echo "Installing PySerial Package"
python -m pip install pyserial
pause
echo "Navigating to matlabroot directory"
cd "C:\Program Files\MATLAB\R2019a\extern\engines\python"
python setup.py install
echo "Done installing dependencies!"
pause