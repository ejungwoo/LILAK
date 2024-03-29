#!/usr/bin/env python3

import os

count_block = 0
def bline():
    global count_block
    count_block = count_block + 1
    print(f"\n== {count_block} ===============================================================================")

bline()
print("## LILAK configuration macro")

lilak_path = os.path.dirname(os.path.abspath(__file__))
log_path = os.path.join(lilak_path,"log")
os.environ["LILAK_PATH"] = lilak_path
print("   LILAK_PATH is", lilak_path)

cmake_file_name = os.path.join(log_path, "build_options.cmake")

main_project = "lilak"
options = {
    "ACTIVATE_EVE": False,
    "CREATE_GIT_LOG": True,
    "BUILD_GEANT4_SIM": True,
    "BUILD_DOXYGEN_DOC": False,
}

project_list = []
def print_project_list():
    global project_list
    global main_project
    if main_project=="lilak":
        print(f"   >> Main: {main_project}")
    if len(project_list)==0:
        print(f"   No Projects")
    else:
        for line in project_list:
            if line==main_project:
                print(f"   Project: {line} (main)")
            else:
                print(f"   Project: {line}")

def input01(question="<1/0>",possible_options=['0','1']):
    while True:
        user_input = input(question)
        if user_input in ['0', '1']:
            return int(user_input)
        else:
            print("Invalid input. Please try again.")
    return user_input

if os.path.exists(cmake_file_name):
    with open(cmake_file_name, "r") as f:
        for line in f:
            line = line.strip()
            if line.find("set(")==0:
                tokens = line[line.find("set(")+4:].split()
                if len(tokens)>1:
                    if tokens[0]=="LILAK_PROJECT_MAIN":
                        main_project = tokens[1]
                    else:
                        options[tokens[0]] = (True if tokens[1]=="ON" else False)
            elif len(line)>0 and line!=")" and line.strip().find("CACHE INTERNAL")<0 and line.strip().find("LILAK")!=0:
                comment = ""
                if line.find("#")>0:
                    line, comment = line[:line.find("#")].strip(), line[line.find("#")+1:].strip()
                project_list.append(line)

while True:
    bline()
    print("## Loading configuration from", cmake_file_name)
    print()
    for key, value in options.items():
        print(f"   {key} = {value}")
    print()
    print_project_list()
    print()
    confirm = input01("Use above options? <1/0>: ")
    if confirm==1:
        print("saving options to", cmake_file_name)
        with open(cmake_file_name, "w") as f:
            for key, value in options.items():
                vonoff = "ON" if value==1 else "OFF"
                f.write(f"set({key} {vonoff} CACHE INTERNAL \"\")\n")
            project_all = ""
            for name in project_list:
                project_all = project_all+'\n    '+name
            f.write(f"""
set(LILAK_PROJECT_LIST{project_all}
    CACHE INTERNAL ""
)""")
            if main_project!="lilak":
                f.write(f'\nset(LILAK_PROJECT_MAIN {main_project} CACHE INTERNAL "")')
        break

    bline()
    print("## Setting options")
    options["CREATE_GIT_LOG"]    = input01("1) Create Git Log? [Recommanded] <1/0>: ")
    options["ACTIVATE_EVE"]      = input01("2) Activate ROOT EVE?            <1/0>: ")
    options["BUILD_GEANT4_SIM"]  = input01("3) Build Geant4 Simulation?      <1/0>: ")
    #options["BUILD_DOXYGEN_DOC"] = input01("4) Build Doxygen document? <1/0>: ")
    options["BUILD_DOXYGEN_DOC"] = 0;

    print()
    user_input_project = "x"
    project_list = []
    while len(user_input_project)>0:
        user_input_project = input("Type project name to add. Type <Enter> if non: ")
        if len(user_input_project)>0:
            if os.path.exists(user_input_project) and os.path.isdir(user_input_project):
                print(f"Adding project {user_input_project}")
                if user_input_project in project_list:
                    print(f"Project {user_input_project} already added!")
                else:
                    project_list.append(user_input_project)
            else:
                print(f"Directory {user_input_project} do not exist in lilak home directory!")
        else:
            bline()
            print("## List of projects")
            print_project_list()
            main_project = "lilak"
            while True:
                if len(project_list)==0:
                    break
                main_project = input("Type project name to set as main. Type <Enter> for no change: ")
                if main_project in project_list:
                    print(f"Main project is: {main_project}")
                    break
                else:
                    print(f"Project must be one in the list!")

bline()
print( "## How to build lilak")
print()
print(f"   1) Open login script and put:")
print(f"      export LILAK_PATH=\"{lilak_path}\"")
print()
print(f"   2) Open ~/.rootrc and put:")
print(f"      Rint.Logon: {lilak_path}/macros/rootlogon.C")
print()
print(f"   3) Build:")
print(f"      cd {lilak_path}")
print(f"      mkdir build")
print(f"      cd build")
print(f"      cmake ..")
print(f"      make")
