// 1. Create your PowerShell script
// 2. Specify its path in the pipeline.Commands.AddScript
// 3. Save C# file as c:\temp\something.cs
// 4. Compile your C# file with: c:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe c:\temp\something.cs /r:"C:\Program Files (x86)\Reference Assemblies\Microsoft\WindowsPowerShell\3.0\System.Management.Automation.dll"
// 5. Run the resulting something.exe file

namespace MyPowerShell
{
    class Program
    {
        static void Main()
        {
            System.Management.Automation.Runspaces.Runspace runspace = System.Management.Automation.Runspaces.RunspaceFactory.CreateRunspace();
            runspace.Open();
            System.Management.Automation.Runspaces.Pipeline pipeline = runspace.CreatePipeline();
            pipeline.Commands.AddScript("c:\\temp\\something.ps1");
            System.Collections.ObjectModel.Collection<System.Management.Automation.PSObject> results = pipeline.Invoke();
            runspace.Close();
            System.Text.StringBuilder stringBuilder = new System.Text.StringBuilder();
            foreach (System.Management.Automation.PSObject obj in results)
            {
                stringBuilder.AppendLine(obj.ToString());
            }
            System.Console.WriteLine(stringBuilder.ToString());
        }
    }
}
