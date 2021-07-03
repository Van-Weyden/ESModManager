using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;

namespace ESModManagerCleaner
{
    class Program
    {
        static void Main(string[] args)
        {
            string appPath = System.Reflection.Assembly.GetEntryAssembly().Location;
            string appFolderPath = new FileInfo(appPath).DirectoryName + '\\';

            RestoreGameLauncher(appFolderPath);

            File.Delete(appFolderPath + "Everlasting Summer (modified).exe");
            File.Delete(appFolderPath + "LaunchedProgram.ini");

            Process.Start(new ProcessStartInfo()
            {
                WorkingDirectory = appFolderPath,
                FileName = "Everlasting Summer.exe"
            });

            Process.Start(new ProcessStartInfo()
            {
                Arguments = "/C choice /C Y /N /D Y /T 5 & Del \"" + Assembly.GetEntryAssembly().Location + "\"",
                WindowStyle = ProcessWindowStyle.Hidden,
                CreateNoWindow = true,
                FileName = "cmd.exe"
            });
        }

        static void RestoreGameLauncher(string appFolderPath)
        {
            if (File.Exists(appFolderPath + "Everlasting Summer (origin).exe"))
            {
                if (File.Exists(appFolderPath + "ESModManagerCleaner.dat"))
                {
                    byte[] launcherHash = File.ReadAllBytes(appFolderPath + "ESModManagerCleaner.dat");
                    File.Delete(appFolderPath + "ESModManagerCleaner.dat");
                    FileStream stream = File.OpenRead(appFolderPath + "Everlasting Summer.exe");
                    byte[] executableHash = MD5.Create().ComputeHash(stream);
                    stream.Close();

                    if (!launcherHash.SequenceEqual(executableHash))
                        return;
                }

                File.Delete(appFolderPath + "Everlasting Summer.exe");
                File.Move(appFolderPath + "Everlasting Summer (origin).exe", appFolderPath + "Everlasting Summer.exe");
            }
        }
    }
}
