import subprocess as esmm_subprocess

def esmm_manager_file_name():
    return "ESModManager.exe"
    
def esmm_remove_disabled_mods(dir_path, searchpath):   
    disabled_mods = open(dir_path + "/disabled_mods.txt", "r").read().splitlines()
    for mod_name in disabled_mods:
        searchpath[:] = [
            mod_path
            for mod_path in searchpath
                if not mod_path.endswith("/" + mod_name)
        ]
    return

def esmm_launch_manager(dir_path):
    esmm_subprocess.Popen(dir_path + "/bin/" + esmm_manager_file_name(), cwd=dir_path)
    return True