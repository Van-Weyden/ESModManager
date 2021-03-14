using System;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Runtime.InteropServices;

namespace WaitingLauncher
{
    class Program
    {
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool AllocConsole();

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool FreeConsole();

        static bool IsProcessRunning(string processName)
        {
            return (Process.GetProcessesByName(processName).Length != 0);
        }

        static int Main(string[] args)
        {
            string programFolderPath = null;
            string programName = null;
            bool isMonitoringNeeded = false;
            bool dontLaunchIfAlreadyLaunched = false;
            bool launchProgramAfterClose = false;
            string secondProgramFolderPath = null;
            string secondProgramName = null;

            if (args != null && args.Length >= 2)
            {
                programFolderPath = args[0];
                programName = args[1];

                if (args.Length > 2)
                    isMonitoringNeeded = Convert.ToBoolean(args[2]);

                if (args.Length > 3)
                    dontLaunchIfAlreadyLaunched = Convert.ToBoolean(args[3]);

                if (args.Length > 4)
                    launchProgramAfterClose = Convert.ToBoolean(args[4]);

                if (args.Length > 6)
                {
                    secondProgramFolderPath = args[5];
                    secondProgramName = args[6];
                }
            }
            else
            {
                string path = "LaunchedProgram.ini";
                if ((args == null || args.Length == 0) && File.Exists(path))
                {
                    StreamReader reader = File.OpenText(path);
                    programFolderPath = reader.ReadLine();
                    programName = reader.ReadLine();
                    isMonitoringNeeded = Convert.ToBoolean(reader.ReadLine());
                    launchProgramAfterClose = Convert.ToBoolean(reader.ReadLine());
                    dontLaunchIfAlreadyLaunched = Convert.ToBoolean(reader.ReadLine());
                    secondProgramFolderPath = reader.ReadLine();
                    secondProgramName = reader.ReadLine();
                    reader.Close();
                }

                if (programFolderPath == null || programName == null)
                {
                    if (AllocConsole())
                    {
                        Console.WriteLine("\nParameters:\n\nProgramFolderPath ProgramName " +
                            "[IsMonitoringNeeded] [LaunchProgramAfterClose] [DontLaunchIfAlreadyLaunched] " +
                            "[SecondProgramFolderPath] [SecondProgramName]\n");
                        Console.WriteLine("ProgramFolderPath: " +
                            "path to the folder with the monitored program;");
                        Console.WriteLine("ProgramName: " +
                            "name of the monitored program;");
                        Console.WriteLine("IsMonitoringNeeded: " +
                            "flag indicating the need to monitor the program until it is closed;");
                        Console.WriteLine("LaunchProgramAfterClose: " +
                            "flag indicating whether to run the program if it is already running;");
                        Console.WriteLine("DontLaunchIfAlreadyLaunched: " +
                            "flag indicating whether to run the program after closing the monitored program;");
                        Console.WriteLine("SecondProgramFolderPath: " +
                            "path to the program folder, which will be launched after the monitored program is closed " +
                            "(if not specified, the origin program path will be used);");
                        Console.WriteLine("SecondProgramName: " +
                            "name of the program which will be launched after the monitored program is closed " +
                            "(if not specified, the origin program name will be used).");
                        Console.WriteLine("\nYou can also set the values of these parameters in file 'LaunchedProgram.ini' " +
                            "(each value must be on a new line).");
                        Console.Write("\nPress any key to continue...");
                        Console.ReadLine();

                        FreeConsole();
                    }
                    
                    return 0;
                }
            }

            if (secondProgramFolderPath == null || secondProgramName == null)
            {
                secondProgramFolderPath = programFolderPath;
                secondProgramName = programName;
            }

            ProcessStartInfo startInfo = new ProcessStartInfo();
            startInfo.WorkingDirectory = programFolderPath;
            startInfo.FileName = programName + ".exe";

            if (!dontLaunchIfAlreadyLaunched || !IsProcessRunning(programName))
                Process.Start(startInfo);

            if (isMonitoringNeeded)
            {
                while (!IsProcessRunning(programName))
                    Thread.Sleep(100);

                while (IsProcessRunning(programName))
                    Thread.Sleep(100);

                if (launchProgramAfterClose)
                {
                    startInfo.WorkingDirectory = secondProgramFolderPath;
                    startInfo.FileName = secondProgramName + ".exe";
                    Process.Start(startInfo);
                }
            }

            return 1;
        }
    }
}
