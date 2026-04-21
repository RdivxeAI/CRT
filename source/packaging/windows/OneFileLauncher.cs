using System;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Windows.Forms;

internal static class OneFileLauncher
{
    private const string PayloadVersion = "__PAYLOAD_VERSION__";
    private const string PayloadResourceName = "PayloadZip";
    private const string MainExecutableName = "CRT_EMULATOR.exe";

    [STAThread]
    private static void Main()
    {
        try
        {
            string root = Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                "MechaML Researchers",
                "CRT_EMULATOR",
                PayloadVersion);

            string appPath = Path.Combine(root, MainExecutableName);
            if (!File.Exists(appPath))
            {
                ExtractPayload(root);
            }

            ProcessStartInfo startInfo = new ProcessStartInfo();
            startInfo.FileName = appPath;
            startInfo.WorkingDirectory = root;
            startInfo.UseShellExecute = true;

            Process.Start(startInfo);
        }
        catch (Exception ex)
        {
            MessageBox.Show(
                "Unable to start CRT EMULATOR.\n\n" + ex.Message,
                "Launch Error",
                MessageBoxButtons.OK,
                MessageBoxIcon.Error);
        }
    }

    private static void ExtractPayload(string root)
    {
        Directory.CreateDirectory(root);

        Assembly assembly = Assembly.GetExecutingAssembly();
        Stream stream = assembly.GetManifestResourceStream(PayloadResourceName);
        if (stream == null)
        {
            throw new InvalidOperationException("Embedded payload was not found.");
        }

        using (stream)
        using (ZipArchive archive = new ZipArchive(stream, ZipArchiveMode.Read))
        {
            foreach (ZipArchiveEntry entry in archive.Entries)
            {
                string destination = Path.Combine(root, entry.FullName);

                if (string.IsNullOrEmpty(entry.Name))
                {
                    Directory.CreateDirectory(destination);
                    continue;
                }

                string directory = Path.GetDirectoryName(destination);
                if (!string.IsNullOrEmpty(directory))
                {
                    Directory.CreateDirectory(directory);
                }

                using (Stream input = entry.Open())
                using (FileStream output = new FileStream(destination, FileMode.Create, FileAccess.Write, FileShare.None))
                {
                    input.CopyTo(output);
                }
            }
        }
    }
}
