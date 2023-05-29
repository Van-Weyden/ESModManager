init -999 python:
    import os as esmm_os
    import subprocess as esmm_subprocess
    
    class ESModManager:
        def __init__(self):
            self.canUse = False

            if (not renpy.windows):
                return

            self.label = "ESModManager"
            
            self.executableExtension = ".exe"
            
            self.gameFileName = "Everlasting Summer"
            self.managerFileName = "ESModManager"
            self.processCheckerFileName = "ProcessChecker"
            self.waitingLauncherFileName = "ESModManagerLauncher"
            self.managerSettingsFullFileName = "settings.ini"
            
            self.managerFullFileName = self.managerFileName + self.executableExtension
            self.processCheckerFullFileName = self.processCheckerFileName + self.executableExtension
            self.waitingLauncherFullFileName = self.waitingLauncherFileName + self.executableExtension
            
            self.renpyFileNamesList = renpy.list_files()
            for self.renpyFileName in self.renpyFileNamesList:
                if (self.managerFullFileName in self.renpyFileName):
                    self.canUse = True
                    self.managerFileFullPath = renpy.file(self.renpyFileName).name
                    self.managerBinDirPath = esmm_os.path.abspath(esmm_os.path.dirname(self.managerFileFullPath))
                    self.managerDirPath = esmm_os.path.abspath(esmm_os.path.dirname(self.managerBinDirPath))
                    self.managerBinDirPath += '\\'
                    self.managerDirPath += '\\'
                    self.processCheckerFullPath = self.managerBinDirPath + self.processCheckerFullFileName
                    break

            if (self.canUse):
                esmm_processChecker = esmm_subprocess.Popen([self.processCheckerFullPath, self.managerFileName])
                
                if (not esmm_processChecker.wait()):
                    self.managerSettings = open(self.managerBinDirPath + self.managerSettingsFullFileName, "r")
                    for self.line in self.managerSettings:
                        if (self.line.startswith("bReplaceOriginLauncher=")):
                            if (self.line.startswith("bReplaceOriginLauncher=true")):
                                self.runWaitingLauncher(self.gameFileName)
                                renpy.quit()
                            break
                            
        def initEntryInGameMenu(self):
            if (self.canUse):
                if (_preferences.language == None):
                    mods[self.label] = u"Менеджер модов"
                else:
                    mods[self.label] = "Mod Manager"
                
                try:
                    modsImages[self.label] = ("bin/ESModManager.png", False, '')
                    imgsModsMenu_polyMods.append(self.label)
                except:
                    pass
        
        def onClickedInGameMenu(self):
            if (self.canUse):
                esmm_processChecker = esmm_subprocess.Popen([self.processCheckerFullPath, self.managerFileName])

                if (esmm_processChecker.wait()):
                    self.managerSettings = open(self.managerBinDirPath + self.managerSettingsFullFileName, "r")
                    for self.line in self.managerSettings:
                        if (self.line.startswith("bAutoexit=")):
                            #If the autoexit is disabled, the manager will be reopened automatically after closing the game.
                            if (not self.line.startswith("bAutoexit=false")):
                                self.runWaitingLauncher(self.managerFileName)
                            break;
                else:
                    self.runWaitingLauncher(self.gameFileName)

                renpy.quit()
                
        def runWaitingLauncher(self, monitoredProgramName):
            esmm_subprocess.Popen([
                self.managerDirPath + self.waitingLauncherFullFileName, 
                "",                     #Path to the folder with the monitored program (not necessary in our case)
                monitoredProgramName,   #Name of the monitored program
                "true",                 #Flag indicating the need to monitor the program until it is closed
                "true",                 #Flag indicating whether to run the program if it is already running
                "true",                 #Flag indicating whether to run the program after closing the monitored program
                self.managerBinDirPath, #Path to the program folder, which will be launched after the monitored program is closed
                self.managerFileName    #Name of the program which will be launched after the monitored program is closed
            ])
            
    #ESModManager class end
    
    esModManager = ESModManager()

init python:
    esModManager.initEntryInGameMenu()

label ESModManager:
    python:
        esModManager.onClickedInGameMenu()
    return