using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Security.Cryptography;

namespace ESModManagerCleaner
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
                    Assembly.GetEntryAssembly().GetName().Name + ": " +
                    data +
                    (addEndLine ? "\n" : "")
                );
            }
        }

        static void Main(string[] args)
        {
            Log("Main: program started");
            string appPath = Assembly.GetEntryAssembly().Location;
            string appFolderPath = new FileInfo(appPath).DirectoryName + '\\';
            
            RestoreOriginFile(appFolderPath, "Everlasting Summer");
            RestoreOriginFile(appFolderPath, "Everlasting Summer-32");

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

        static void RestoreOriginFile(string appFolderPath, string fileName)
        {
            if (File.Exists(appFolderPath + fileName + " (modified).exe"))
            {
                File.Delete(appFolderPath + fileName + " (modified).exe");
            }

            string originFilePath = appFolderPath + fileName + " (origin).exe";
            string modifiedFilePath = appFolderPath + fileName + ".exe";

            if (File.Exists(originFilePath))
            {
                if (File.Exists(appFolderPath + "ESModManagerCleaner.dat"))
                {
                    byte[] originFileHash = File.ReadAllBytes(appFolderPath + "ESModManagerCleaner.dat");
                    File.Delete(appFolderPath + "ESModManagerCleaner.dat");
                    FileStream stream = File.OpenRead(modifiedFilePath);
                    byte[] modifiedFileHash = MD5.Create().ComputeHash(stream);
                    stream.Close();

                    string originFileHashString = System.Text.Encoding.UTF8.GetString(originFileHash);
                    string modifiedFileHashString = BitConverter.ToString(modifiedFileHash).Replace("-", "").ToLower();

                    Log("originFileHashString: " + originFileHashString);
                    Log("modifiedFileHashString: " + modifiedFileHashString);

                    if (!originFileHashString.Equals(modifiedFileHashString))
                    {
                        Log("hashes are not equal.");
                        return;
                    }
                }

                File.Delete(modifiedFilePath);
                File.Move(originFilePath, modifiedFilePath);
            }
        }
    }
}
