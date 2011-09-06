using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Threading;
using IronPython.Hosting;
using Microsoft.Scripting.Hosting;
using Microsoft.Win32;

namespace PieLoader
{
    public class Loader : MarshalByRefObject
    {
        [DllImport("kernel32.dll")]
        static extern void OutputDebugString(string lpOutputString);

        public Loader(string scriptPath)
        {
            try
            {
                var thread = new Thread(RunPython);
                thread.SetApartmentState(ApartmentState.STA);
                thread.Start(scriptPath);
            }
            catch (Exception e)
            {
                OutputDebugString(e.Message);
            }
        }

        static void RunPython(object boxedScriptPath)
        {
            try
            {
                var scriptPath = boxedScriptPath.ToString();
                var ironPythonPath = GetIronPythonPath();

                var engine = Python.CreateEngine();
                SetSearchPaths(engine, scriptPath, ironPythonPath);
                engine.Runtime.LoadAssembly(Assembly.LoadFrom(Path.Combine(ironPythonPath, @"DLLs\IronPython.Wpf.dll")));

                var ext = Path.GetExtension(scriptPath);
                switch (ext)
                {
                    case ".py":
                        var source = engine.CreateScriptSourceFromFile(scriptPath);
                        source.Execute();
                        break;

                    default:
                        throw new Exception("unrecognized extension: " + ext);
                }
            }
            catch (Exception e)
            {
                OutputDebugString(e.Message);
            }
        }

        private static void SetSearchPaths(ScriptEngine engine, string scriptPath, string ironPythonPath)
        {
            var paths = engine.GetSearchPaths();
            paths.Add(Path.GetDirectoryName(scriptPath));
            paths.Add(ironPythonPath);
            paths.Add(Path.Combine(ironPythonPath, "DLLs"));
            paths.Add(Path.Combine(ironPythonPath, "Lib"));
            paths.Add(Path.Combine(ironPythonPath, @"Lib\site-packages"));
            engine.SetSearchPaths(paths);
        }

        static string GetIronPythonPath()
        {
            var hklm = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Registry32);
            var key = hklm.OpenSubKey(@"SOFTWARE\IronPython\2.7\InstallPath");
            return key.GetValue(null).ToString();
        }

        static int LoadInNewAppDomain(string scriptPath)
        {
            try
            {
                var appDomain = AppDomain.CreateDomain("IronPythonAppDomain", null,
                    new AppDomainSetup {
                        ApplicationBase = Path.GetDirectoryName(scriptPath)
                    });

                appDomain.CreateInstance(Assembly.GetExecutingAssembly().FullName,
                    typeof(Loader).FullName, false, 0, null, new Object[] { scriptPath }, null, null);
            }
            catch (Exception e)
            {
                OutputDebugString(e.Message);
            }
            return 0;
        }
    }
}
