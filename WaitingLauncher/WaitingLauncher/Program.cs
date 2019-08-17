using System;
using System.Diagnostics;
using System.IO;
using System.Threading;

namespace WaitingLauncher
{
    class Program
    {
        static int Main(string[] args)
        {
            string path = "LaunchedProgram.ini";
            Console.WriteLine(path);
            if (!File.Exists(path))
                return 0;

            StreamReader reader = File.OpenText(path);
            String programFolderPath = reader.ReadLine();
            String programName = reader.ReadLine();
            bool isMonitoringNeeded = Convert.ToBoolean(reader.ReadLine());
            reader.Close();

            ProcessStartInfo startInfo = new ProcessStartInfo();
            startInfo.WorkingDirectory = programFolderPath;
            startInfo.FileName = programName + ".exe";
            Process.Start(startInfo);

            //Process.Start(programFolderPath + programName + ".exe");

            if (isMonitoringNeeded)
            {
                bool isRunning = false;
                Process[] pname;
                //Console.WriteLine("nothing");
                while (!isRunning)
                {
                    pname = Process.GetProcessesByName(programName);
                    if (pname.Length != 0)
                        isRunning = true;
                    Thread.Sleep(100);
                }
                //Console.WriteLine("run");
                while (isRunning)
                {
                    pname = Process.GetProcessesByName(programName);
                    if (pname.Length == 0)
                        isRunning = false;
                    Thread.Sleep(100);
                }
                //Console.WriteLine("nothing");
            }
            return 1;
        }
    }
}
