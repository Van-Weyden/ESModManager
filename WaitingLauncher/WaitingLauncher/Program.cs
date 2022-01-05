using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;
using System.Threading;

namespace WaitingLauncher
{
    class Program
    {
        static readonly bool IsDebugLogEnabled = false;
        static readonly string DebugLogFile = "log.txt";

        public static void Log(string data, bool addEndLine = true, bool addTime = true)
        {
            if (IsDebugLogEnabled)
            {
                File.AppendAllText(DebugLogFile,
                    (addTime ? "[" + DateTime.Now + "] " : "") + 
                    data + 
                    (addEndLine ? "\n" : "")
                );
            }
        }

        public enum ExitCode
        {
            WrongInitArgs = 0,
            ProgramStarted = 1,
            ProgramAfterCloseStarted = 2,
            ProgramFailedToStart = -1,
            ProgramAfterCloseFailedToStart = -2,
        };

        static int Main(string[] args)
        {
            Program.Log("", true, false);
            Program.Log("Main: program started");

            ExitCode returnCode;
            Launcher launcher = new Launcher();

            string[] parsedArgs = new string[] { };

            if (args != null)
            {
                for (int i = 0; i < args.Length; ++i)
                {
                    parsedArgs = parsedArgs.Concat(Regex.Matches(args[i], "((\\\"([^\\\"])*\\\")|([^ \\\"]+))")
                                       .Cast<Match>()
                                       .Select(m => m.Value)
                                       .ToArray()).ToArray();
                }
            }

            Program.Log("Main: args parsed");

            if (launcher.InitFromArgs(parsedArgs) || (parsedArgs.Length == 0 && launcher.InitFromFile("LaunchedProgram.ini")))
                returnCode = launcher.LaunchProgram();
            else
            {
                Program.Log("Main: WrongInitArgs");
                returnCode = ExitCode.WrongInitArgs;
                Launcher.WriteUsageInfo();
            }

            Program.Log("Main: exit code: " + returnCode.ToString());
            return (int)returnCode;
        }
    };

    class Launcher
    {
        internal static class KeyWord
        {
            public const string ProgramFolderPath               = "ProgramPath";
            public const string ProgramName                     = "ProgramName";
            public const string IsLauncherHasSameName           = "IsLauncherHasSameName";
            public const string IsMonitoringNeeded              = "IsMonitoringNeeded";
            public const string DontLaunchIfAlreadyLaunched     = "DontLaunchIfAlreadyLaunched";
            public const string LaunchProgramAfterClose         = "LaunchProgramAfterClose";
            public const string ProgramOnError                  = "ProgramOnError";
            public const string ProgramOnErrorFolderPath        = "ProgramOnErrorPath";
            public const string ProgramOnErrorName              = "ProgramOnErrorName";
            public const string ProgramAfterClose               = "ProgramAfterClose";
            public const string ProgramAfterCloseName           = "ProgramAfterCloseName";
            public const string ProgramAfterCloseFolderPath     = "ProgramAfterClosePath";
        };

        internal static class ArgCode
        {
            public const string IsLauncherHasSameName           = "-" + KeyWord.IsLauncherHasSameName;
            public const string IsMonitoringNeeded              = "-" + KeyWord.IsMonitoringNeeded;
            public const string DontLaunchIfAlreadyLaunched     = "-" + KeyWord.DontLaunchIfAlreadyLaunched;
            public const string LaunchProgramAfterClose         = "-" + KeyWord.LaunchProgramAfterClose;
            public const string ProgramOnError                  = "-" + KeyWord.ProgramOnError;
            public const string ProgramAfterClose               = "-" + KeyWord.ProgramAfterClose;
        };

        public Launcher()
        {
            m_processLauncher = new ProcessLauncher();
            ResetInit();
        }

        public bool IsNeedToLaunchProgram()
        {
            return (!m_dontLaunchIfAlreadyLaunched || !IsProcessRunning(m_programName));
        }

        public Program.ExitCode LaunchProgram()
        {
            Program.Log("Launcher::LaunchProgram");
            if (!IsInit())
                return Program.ExitCode.WrongInitArgs;

            Program.ExitCode returnCode;
            if (m_processLauncher.SetProcess(m_programFolderPath, m_programName))
            {
                returnCode = Program.ExitCode.ProgramStarted;

                if (IsNeedToLaunchProgram())
                    m_processLauncher.StartProcess();

                if (m_isMonitoringNeeded && WaitProgramLaunch(m_programName, 100, 30000))
                {
                    WaitProgramStop(m_programName);

                    if (m_launchProgramAfterClose)
                        returnCode = LaunchProgramAfterClose();
                }
            }
            else
                returnCode = Program.ExitCode.ProgramFailedToStart;

            if (returnCode < 0)
                m_processLauncher.StartProcess(m_programOnErrorFolderPath, m_programOnErrorName);

            return returnCode;
        }

        public Program.ExitCode LaunchProgramAfterClose()
        {
            if (!IsInit())
                return Program.ExitCode.WrongInitArgs;

            if (m_processLauncher.StartProcess(m_programAfterCloseFolderPath, m_programAfterCloseName))
                return Program.ExitCode.ProgramAfterCloseStarted;

            return Program.ExitCode.ProgramAfterCloseFailedToStart;
        }

        public bool WaitProgramLaunch(string programName, int waitStep = 100, int timeout = -1)
        {
            return WaitForCondition(() => { return IsProcessRunning(programName); }, waitStep, timeout);
        }

        public bool WaitProgramStop(string programName, int waitStep = 100, int timeout = -1)
        {
            return WaitForCondition(() => { return !IsProcessRunning(programName); }, waitStep, timeout);
        }

        public static bool WaitForCondition(Func<bool> condition, int waitStep = 100, int timeout = -1)
        {
            int elapsedTime = 0;

            while ((timeout <= 0 || elapsedTime < timeout) && !condition())
            {
                elapsedTime += waitStep;
                Thread.Sleep(waitStep);
            }

            return condition();
        }

        public bool IsInit()
        {
            return (m_programFolderPath != null && m_programName != null);
        }

        public void ResetInit()
        {
            m_programFolderPath             = null;
            m_programName                   = null;
            m_isLauncherHasSameName         = false;
            m_isMonitoringNeeded            = false;
            m_dontLaunchIfAlreadyLaunched   = false;
            m_launchProgramAfterClose       = false;
            m_programAfterCloseFolderPath   = null;
            m_programAfterCloseName         = null;
            m_programOnErrorFolderPath      = null;
            m_programOnErrorName            = null;
        }

        public bool InitFromArgs(string[] args)
        {
            Program.Log("Launcher::InitFromArgs");
            ResetInit();

            if (args != null && args.Length >= 2)
            {
                m_programFolderPath = args[0].Trim('"');
                m_programName = args[1].Trim('"');

                for (int i = 2; i < args.Length;)
                {
                    string arg = args[i++];

                    switch (arg)
                    {
                        case ArgCode.IsLauncherHasSameName:
                            m_isLauncherHasSameName = true;
                        break;

                        case ArgCode.IsMonitoringNeeded:
                            m_isMonitoringNeeded = true;
                        break;

                        case ArgCode.DontLaunchIfAlreadyLaunched:
                            m_dontLaunchIfAlreadyLaunched = true;
                        break;

                        case ArgCode.LaunchProgramAfterClose:
                            m_launchProgramAfterClose = true;
                        break;

                        case ArgCode.ProgramOnError:
                            if (i < args.Length - 1)
                            {
                                m_programOnErrorFolderPath = args[i++].Trim('"');
                                m_programOnErrorName = args[i++].Trim('"');
                            }
                        break;

                        case ArgCode.ProgramAfterClose:
                            if (i < args.Length - 1)
                            {
                                m_programAfterCloseFolderPath = args[i++].Trim('"');
                                m_programAfterCloseName = args[i++].Trim('"');
                            }
                        break;

                        default: break;
                    }
                }

                if (m_programAfterCloseFolderPath == null || m_programAfterCloseName == null)
                {
                    m_programAfterCloseFolderPath = m_programFolderPath;
                    m_programAfterCloseName = m_programName;
                }
            }

            return IsInit();
        }

        public bool InitFromFile(string path)
        {
            Program.Log("Launcher::InitFromFile");
            ResetInit();

            if (File.Exists(path))
            {
                StreamReader reader = File.OpenText(path);
//              regex = oneOf({
//                  quotedExpression(escapedSymbol('\"')),
//                  oneOrMoreOccurences(
//                      oneOf({
//                          symbolNotFromSet({
//                              ' ',
//                              escapedSymbol('\"')
//                          })
//                      })
//		            )
//              })

                string[] args = Regex.Matches(reader.ReadToEnd(), "((\\\"([^\\\"])*\\\")|([^ \\\"])+)")
                                     .Cast<Match>()
                                     .Select(m => m.Value)
                                     .ToArray();
                reader.Close();

                return InitFromArgs(args);
            }

            return false;
        }

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool AllocConsole();

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool FreeConsole();

        public bool IsProcessRunning(string processName)
        {
            return (Process.GetProcessesByName(processName).Length > (m_isLauncherHasSameName ? 1 : 0));
        }

        public static void WriteUsageInfo()
        {
            if (AllocConsole())
            {
                Console.WriteLine(
                    "\nParameters:\n\n\"" + KeyWord.ProgramFolderPath + "\" \"" + KeyWord.ProgramName + "\" [" +
                    ArgCode.IsMonitoringNeeded          + "] [" +
                    ArgCode.DontLaunchIfAlreadyLaunched + "] [" +
                    ArgCode.LaunchProgramAfterClose     + "] [" +
                    ArgCode.ProgramOnError              + " \"" +
                        KeyWord.ProgramOnErrorFolderPath    + "\" \"" +
                        KeyWord.ProgramOnErrorName          + "\"] [" +
                    ArgCode.ProgramAfterClose           + " \"" +
                        KeyWord.ProgramAfterCloseFolderPath + "\" \"" +
                        KeyWord.ProgramAfterCloseName       + "\"]" +
                    "\n"
                );

                Console.WriteLine(KeyWord.ProgramFolderPath + ": " +
                    "path to the folder with the monitored program;");
                Console.WriteLine(KeyWord.ProgramName + ": " +
                    "name of the monitored program;");

                Console.WriteLine(KeyWord.IsLauncherHasSameName + ": " +
                    "flag indicating that the launcher has the same name and shouldn't be taking in account while processes check" +
                    "(false by default);");
                Console.WriteLine(KeyWord.IsMonitoringNeeded + ": " +
                    "flag indicating the need to monitor the program until it is closed" +
                    "(false by default);");
                Console.WriteLine(KeyWord.DontLaunchIfAlreadyLaunched + ": " +
                    "flag indicating whether to run the program if it is already running" +
                    "(false by default);");
                Console.WriteLine(KeyWord.LaunchProgramAfterClose + ": " +
                    "flag indicating whether to run the program after closing the monitored program" +
                    "(false by default);");

                Console.WriteLine(KeyWord.ProgramOnErrorFolderPath + ": " + 
                    "path to the program folder which will be launched in case of " +
                    "an error when starting the previously specified programs;");
                Console.WriteLine(KeyWord.ProgramOnErrorName + ": " +
                    "name of the program which will be launched in case of " +
                    "an error when starting the previously specified programs;");

                Console.WriteLine(KeyWord.ProgramAfterCloseFolderPath + ": " +
                    "path to the program folder which will be launched after the monitored program is closed " +
                    "(if not specified, the origin program path will be used);");
                Console.WriteLine(KeyWord.ProgramAfterCloseName + ": " +
                    "name of the program which will be launched after the monitored program is closed " +
                    "(if not specified, the origin program name will be used).");

                Console.WriteLine("\nYou can also set the values of these parameters in file 'LaunchedProgram.ini' " +
                    "(each value must be on a new line).");
                Console.Write("\nPress any key to continue...");
                Console.ReadLine();

                FreeConsole();
            }
        }

        private ProcessLauncher m_processLauncher;
        private string m_programFolderPath;
        private string m_programName;
        private bool m_isLauncherHasSameName;
        private bool m_isMonitoringNeeded;
        private bool m_dontLaunchIfAlreadyLaunched;
        private bool m_launchProgramAfterClose;
        private string m_programAfterCloseFolderPath;
        private string m_programAfterCloseName;
        private string m_programOnErrorFolderPath;
        private string m_programOnErrorName;
    }

    class ProcessLauncher
    {
        public ProcessLauncher()
        {
            m_startInfo = new ProcessStartInfo();
            m_slashesRegex = new Regex("(/){1,}");
            m_backslashesRegex = new Regex("(\\\\){2,}");
        }

        public bool StartProcess()
        {
            if (File.Exists(m_startInfo.WorkingDirectory + m_startInfo.FileName))
            {
                Process.Start(m_startInfo);
                return true;
            }

            return false;
        }

        public bool StartProcess(string workingDirectory, string fileName)
        {
            if (SetProcess(workingDirectory, fileName))
            {
                Process.Start(m_startInfo);
                return true;
            }

            return false;
        }

        public bool SetProcess(string workingDirectory, string fileName)
        {
            m_startInfo.WorkingDirectory = null;
            m_startInfo.FileName = null;

            if (workingDirectory == null || fileName == null)
            {
                Program.Log("ProcessLauncher::SetProcess: workingDirectory is null : " + (workingDirectory == null));
                Program.Log("ProcessLauncher::SetProcess: fileName is null : " + (fileName == null));
                return false;
            }

            m_startInfo.WorkingDirectory = m_slashesRegex.Replace(workingDirectory, "\\");
            m_startInfo.WorkingDirectory = m_backslashesRegex.Replace(m_startInfo.WorkingDirectory, "\\");
            m_startInfo.WorkingDirectory += (m_startInfo.WorkingDirectory.EndsWith("\\") ? "" : "\\");
            m_startInfo.FileName = fileName + (fileName.EndsWith(".exe") ? "" : ".exe");

            Program.Log("ProcessLauncher::SetProcess: filePath: " + m_startInfo.WorkingDirectory + m_startInfo.FileName);

            return File.Exists(m_startInfo.WorkingDirectory + m_startInfo.FileName);
        }

        private ProcessStartInfo m_startInfo;
        private Regex m_slashesRegex;
        private Regex m_backslashesRegex;
    }
}
