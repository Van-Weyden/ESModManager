init python:
    import os as esmm_os
    import subprocess as esmm_subprocess
    
    esmm_label = "ESModManager"
    
    esmm_executableExtension = ".exe"
    
    esmm_gameFileName = "Everlasting Summer"
    esmm_managerFileName = "ESModManager"
    esmm_processCheckerFileName = "ProcessChecker"
    esmm_waitingLauncherFileName = "ESModManagerLauncher"
    esmm_managerSettingsFullFileName = "settings.ini"
    
    esmm_managerFullFileName = esmm_managerFileName + esmm_executableExtension
    esmm_processCheckerFullFileName = esmm_processCheckerFileName + esmm_executableExtension
    esmm_waitingLauncherFullFileName = esmm_waitingLauncherFileName + esmm_executableExtension
    
    esmm_isManagerInstalled = False
    
    esmm_renpyFileNamesList = renpy.list_files()
    for esmm_renpyFileName in esmm_renpyFileNamesList:
        if (esmm_managerFullFileName in esmm_renpyFileName):
            esmm_isManagerInstalled = True
            esmm_managerFileFullPath = renpy.file(esmm_renpyFileName).name
            esmm_managerBinDirPath = esmm_os.path.abspath(esmm_os.path.dirname(esmm_managerFileFullPath))
            esmm_managerDirPath = esmm_os.path.abspath(esmm_os.path.dirname(esmm_managerBinDirPath))
            esmm_managerBinDirPath += '\\'
            esmm_managerDirPath += '\\'
            break;
    
    if (esmm_isManagerInstalled):
        if (_preferences.language == None):
            mods[esmm_label] = u"Менеджер модов"
        else:
            mods[esmm_label] = "Mod Manager"
        
        try:
            modsImages[esmm_label] = ("bin/ESModManager.png", False, '')
            imgsModsMenu_polyMods.append(esmm_label)
        except:
            pass

label ESModManager:
    python:
        if (esmm_isManagerInstalled):
            esmm_processCheckerFullPath = esmm_managerBinDirPath + esmm_processCheckerFullFileName
            esmm_processChecker = esmm_subprocess.Popen([esmm_processCheckerFullPath, esmm_managerFileName])
            esmm_needLaunchManager = True
            
            if (esmm_processChecker.wait()):
                esmm_managerSettings = open(esmm_managerBinDirPath + esmm_managerSettingsFullFileName, "r")
                for esmm_line in esmm_managerSettings:
                    if (esmm_line.startswith("bAutoexit=")):
                        #If the autoexit is disabled, the manager will be reopened automatically after closing the game.
                        esmm_needLaunchManager = not esmm_line.startswith("bAutoexit=false")
                        esmm_programName = esmm_managerFileName
                        break;
            else:
                esmm_programName = esmm_gameFileName
            
            if (esmm_needLaunchManager):
                esmm_subprocess.Popen([esmm_managerDirPath + esmm_waitingLauncherFullFileName, 
                                  "",                       #Path to the folder with the monitored program (not necessary in our case)
                                  esmm_programName,         #Pame of the monitored program
                                  "true",                   #Flag indicating the need to monitor the program until it is closed
                                  "true",                   #Flag indicating whether to run the program if it is already running
                                  "true",                   #Flag indicating whether to run the program after closing the monitored program
                                  esmm_managerBinDirPath,   #Path to the program folder, which will be launched after the monitored program is closed
                                  esmm_managerFileName      #Name of the program which will be launched after the monitored program is closed
                                ])
            renpy.quit()
    return