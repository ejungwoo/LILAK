#!/usr/bin/env python3

import os

bline = "===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ====="
#bline = "==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ==== ===="
#bline = "==== + ==== ==== + ==== ==== + ==== ==== + ==== ==== + ==== ==== + ===="

print("\n"+bline)
print("## LILAK configuration")

lilak_path = os.path.dirname(os.path.abspath(__file__))
log_path = os.path.join(lilak_path,"log")
os.environ["LILAK_PATH"] = lilak_path
print("   LILAK_PATH is", lilak_path)

option_file_name = os.path.join(log_path, "build_options.conf")
cmake_file_name = os.path.join(log_path, "build_options.cmake")

options = {
    "ACTIVATE_EVE": False,
    "CREATE_GIT_LOG": True,
    "BUILD_GEANT4_SIM": True,
    "BUILD_DOXYGEN_DOC": False,
}

def input01(question="(1/0)",possible_options=['0','1']):
    while True:
        user_input = input(question)
        if user_input in ['0', '1']:
            return int(user_input)
        else:
            print("Invalid input. Please try again.")
    return user_input

confirm=0
if os.path.exists(option_file_name):
    with open(option_file_name, "r") as f:
        for line in f:
            key, value = line.strip().split("=")
            options[key] = bool(int(value))
    print("\n"+bline)
    print("## Saved options in", option_file_name)
    for key, value in options.items():
        print(f"   {key} = {value}")
    confirm = input01("Use saved options? (1/0): ")

while confirm==0:
    print("\n"+bline)
    print("## Setting options")
    options["ACTIVATE_EVE"]      = input01("1) Activate ROOT EVE? [EVE should be installed beforehand] (1/0): ")
    options["BUILD_GEANT4_SIM"]  = input01("2) Build Geant4 Simulation? (1/0): ")
    #options["BUILD_DOXYGEN_DOC"] = input01("3) Build Doxygen document? (1/0): ")
    options["BUILD_DOXYGEN_DOC"] = 0;
    options["CREATE_GIT_LOG"]    = input01("4) Create Git Log? [Recommanded] (1/0): ")
    print("saving options to", option_file_name)
    with open(option_file_name, "w") as f:
        for key, value in options.items():
            f.write(f"{key}={int(value)}\n")
    with open(cmake_file_name, "w") as f:
        for key, value in options.items():
            vonoff = "ON" if value==1 else "OFF"
            f.write(f"set({key} {vonoff} CACHE INTERNAL \"\")\n")
    confirm = 1

print("\n"+bline)
print( "## Settings:")
print(f"1) Open ~/.zshrc or any login script and write:  ")
print(f"     export LILAK_PATH=\"{lilak_path}\"")
print( "   or")
print(f"     echo export LILAK_PATH=\\\"{lilak_path}\\\" >> ~/.zshrc")
print(f"2) Open ~/.rootrc and write:")
print(f"     Rint.Logon: {lilak_path}/macros/rootlogon.C")
print( "   or")
print(f"     echo Rint.Logon: {lilak_path}/macros/rootlogon.C >> ~/.rootrc")

print("\n"+bline)
print( "## How to build")
print(f"   cd {lilak_path}")
print(f"   mkdir build")
print(f"   cd build")
print(f"   cmake ..")
print(f"   make")
