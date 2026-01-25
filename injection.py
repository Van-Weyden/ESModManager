    # ESModManager injection begin
    try:
        esmm_dir = ""
        for dir in renpy.config.searchpath:
            if dir.endswith("/1826799366"):
                esmm_dir = dir
                break
        if not os.path.exists(esmm_dir + "/esmm_lib.py"):
            raise
        esmm_dir = os.path.abspath(esmm_dir)
        sys.path.append(esmm_dir)
        from esmm_lib import esmm_launch_manager, esmm_remove_disabled_mods
        if (esmm_launch_manager(esmm_dir)):
            return 0
        esmm_remove_disabled_mods(esmm_dir, renpy.config.searchpath)
    except:
        pass
    # ESModManager injection end
