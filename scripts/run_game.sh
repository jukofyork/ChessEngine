#!/bin/bash
# run_game.sh
# Runs a chess game with the engine
# Converted from PlayChess.bat

# Default: Run two computers playing each other for 60 seconds with 0.005 random swing
./PlayChess --thinking --time 60 --mode two-computers --random-swing 0.005
