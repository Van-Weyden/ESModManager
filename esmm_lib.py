import subprocess as esmm_subprocess

def esmm_manager_file_name():
    return "ESModManager.exe"
    
def esmm_remove_disabled_mods(esmm_dir_path, searchpath):   
    esmm_disabled_mods = open(esmm_dir_path + "/disabled_mods.txt", "r").read().splitlines()
    for mod_name in esmm_disabled_mods:
        searchpath[:] = [
            mod_path
            for mod_path in searchpath
                if not mod_path.endswith("/" + mod_name)
        ]
    return

def esmm_launch_manager(esmm_dir_path):
    esmm_subprocess.Popen(esmm_dir_path + "/" + esmm_manager_file_name(), cwd=esmm_dir_path)
    return True