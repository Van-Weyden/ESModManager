using System;
using System.Diagnostics;
using System.IO;
using System.Threading;

namespace ProcessChecker
{
    class Program
    {
        static int Main(string[] args)
        {
            return Process.GetProcessesByName(args[0]).Length;
        }
    }
}
