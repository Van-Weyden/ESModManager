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
        static int Main(string[] args)
        {
            Launcher launcher = new Launcher();

            if (!launcher.InitFromArgs(args) && ((args != null && args.Length > 0) || !launcher.InitFromFile("LaunchedProgram.ini")))
            {
                Launcher.WriteUsageInfo();
                return 0;
            }

            return launcher.LaunchProgram();
        }
    };

    class Launcher
    {
        internal static class ArgCode
        {
            public const string IsMonitoringNeeded              = "-imn";
            public const string DontLaunchIfAlreadyLaunched     = "-dll";
            public const string LaunchProgramAfterClose         = "-lac";
            public const string ProgramOnError                  = "-poe";
            public const string ProgramAfterClose               = "-pac";
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

        public int LaunchProgram()
        {
            if (!IsInit())
                return 0;

            int returnCode = m_processLauncher.SetProcess(m_programFolderPath, m_programName) ? 1 : -1;

            if (returnCode != -1)
            {
                if (IsNeedToLaunchProgram())
                    m_processLauncher.StartProcess();

                if (m_isMonitoringNeeded && WaitProgramLaunch(m_programName, 100, 30000))
                {
                    WaitProgramStop(m_programName);

                    if (m_launchProgramAfterClose)
                        returnCode = LaunchProgramAfterClose();
                }
            }

            if (returnCode < 0)
                m_processLauncher.StartProcess(m_programOnErrorFolderPath, m_programOnErrorName);

            return returnCode;
        }

        public int LaunchProgramAfterClose()
        {
            if (!IsInit())
                return 0;

            if (m_processLauncher.StartProcess(m_programAfterCloseFolderPath, m_programAfterCloseName))
                return 2;

            return -2;
        }

        public static bool WaitProgramLaunch(string programName, int waitStep = 100, int timeout = -1)
        {
            return WaitForCondition(() => { return IsProcessRunning(programName); }, waitStep, timeout);
        }

        public static bool WaitProgramStop(string programName, int waitStep = 100, int timeout = -1)
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
            ResetInit();

            if (args != null && args.Length >= 2)
            {
                m_programFolderPath = args[0].Trim('"');
                m_programName = args[1].Trim('"');

                for (int i = 2; i < args.Length - 1;)
                {
                    string arg = args[i++];

                    switch (arg)
                    {
                        case ArgCode.IsMonitoringNeeded:
                            m_isMonitoringNeeded = Convert.ToBoolean(args[i++]);
                        break;

                        case ArgCode.DontLaunchIfAlreadyLaunched:
                            m_dontLaunchIfAlreadyLaunched = Convert.ToBoolean(args[i++]);
                        break;

                        case ArgCode.LaunchProgramAfterClose:
                            m_launchProgramAfterClose = Convert.ToBoolean(args[i++]);
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
            ResetInit();

            if (File.Exists(path))
            {
                StreamReader reader = File.OpenText(path);

//              regex = oneOf({
//                  quotedExpression(escapedSymbol("\"")),
//                  oneOrMoreOccurences(
//                      oneOf({
//                          anyEscapedSymbolInExpression,
//                           symbolNotFromSet({
//                              " ",
//                              escapedSymbol("\"")
//                          })
//                      })
//		            )
//              })

                string[] args = Regex.Matches(reader.ReadToEnd(), "((\\\"((\\\\.)|[^\\\"])*\\\")|((\\\\.)|[^ \\\"])+)")
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

        public static bool IsProcessRunning(string processName)
        {
            return (Process.GetProcessesByName(processName).Length != 0);
        }

        public static void WriteUsageInfo()
        {
            if (AllocConsole())
            {
                Console.WriteLine("\nParameters:\n\n\"ProgramFolderPath\" \"ProgramName\" [" +
                    ArgCode.IsMonitoringNeeded          + " IsMonitoringNeeded] [" +
                    ArgCode.DontLaunchIfAlreadyLaunched + " DontLaunchIfAlreadyLaunched] [" +
                    ArgCode.LaunchProgramAfterClose     + " LaunchProgramAfterClose] [" +
                    ArgCode.ProgramOnError              + " \"ProgramOnErrorFolderPath\" \"ProgramOnErrorName\"] [" +
                    ArgCode.ProgramAfterClose           + " \"ProgramAfterCloseFolderPath\" \"ProgramAfterCloseName\"]" +
                    "\n");

                Console.WriteLine("ProgramFolderPath: " +
                    "path to the folder with the monitored program;");
                Console.WriteLine("ProgramName: " +
                    "name of the monitored program;");

                Console.WriteLine("IsMonitoringNeeded: " +
                    "flag indicating the need to monitor the program until it is closed" +
                    "(false by default);");
                Console.WriteLine("DontLaunchIfAlreadyLaunched: " +
                    "flag indicating whether to run the program if it is already running" +
                    "(false by default);");
                Console.WriteLine("LaunchProgramAfterClose: " +
                    "flag indicating whether to run the program after closing the monitored program" +
                    "(false by default);");

                Console.WriteLine("ProgramOnErrorFolderPath: " +
                    "path to the program folder which will be launched in case of " +
                    "an error when starting the previously specified programs;");
                Console.WriteLine("ProgramOnErrorName: " +
                    "name of the program which will be launched in case of " +
                    "an error when starting the previously specified programs;");

                Console.WriteLine("ProgramAfterCloseFolderPath: " +
                    "path to the program folder which will be launched after the monitored program is closed " +
                    "(if not specified, the origin program path will be used);");
                Console.WriteLine("ProgramAfterCloseName: " +
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
            m_backslashesRegex = new Regex("(\\\\)+");
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
                return false;
            }

            m_startInfo.WorkingDirectory = m_backslashesRegex.Replace(workingDirectory, "/");
            m_startInfo.WorkingDirectory += (m_startInfo.WorkingDirectory.EndsWith("/") ? "" : "/");
            m_startInfo.FileName = fileName + (fileName.EndsWith(".exe") ? "" : ".exe");

            return File.Exists(m_startInfo.WorkingDirectory + m_startInfo.FileName);
        }

        private ProcessStartInfo m_startInfo;
        private Regex m_backslashesRegex;
    }
}
