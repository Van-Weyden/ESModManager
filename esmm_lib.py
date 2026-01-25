import locale as esmm_locale
import os as esmm_os
import subprocess as esmm_subprocess
import sys as esmm_sys

def esmm_manager_file_path(dir_path):
    if esmm_sys.platform == 'win32':
        return dir_path + "/bin/ESModManager.exe"
    else:
        return ""
    
def esmm_remove_disabled_mods(dir_path, searchpath):   
    disabled_mods = open(dir_path + "/disabled_mods.txt", "r").read().splitlines()
    searchpath[:] = [
        mod_path
        for mod_path in searchpath
            if mod_path.rsplit('/', 1)[-1] not in disabled_mods
    ]
    return
    
def esmm_encoded_path(path):
    return path.encode(esmm_locale.getpreferredencoding())

def esmm_launch_manager(dir_path):
    if esmm_os.path.isfile(dir_path + "disable_autolaunch"):
        return False
    # ToDo: check "can't load cyrillic path" fix:
    # esmm_subprocess.Popen(esmm_encoded_path(dir_path + "/bin/" + esmm_manager_file_name()), cwd=dir_path, shell=True)
    esmm_subprocess.Popen(esmm_manager_file_path(dir_path), cwd=dir_path)
    return True
