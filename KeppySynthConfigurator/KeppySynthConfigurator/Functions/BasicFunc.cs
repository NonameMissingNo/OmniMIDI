﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Diagnostics;
using Microsoft.Win32;

namespace KeppySynthConfigurator
{
    class Functions
    {
        public static bool IsWindows8OrNewer() // Checks if you're using Windows 8.1 or newer
        {
            var reg = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\Windows NT\CurrentVersion");
            string productName = (string)reg.GetValue("ProductName");
            return productName.StartsWith("Windows 8") | productName.StartsWith("Windows 10");
        }

        public static bool IsWindowsXP() // Checks if you're using Windows XP
        {
            OperatingSystem OS = Environment.OSVersion;
            return (OS.Version.Major == 5) || ((OS.Version.Major == 5) && (OS.Version.Minor == 1));
        }

        public static void UserProfileMigration() // Migrates the Keppy's Synthesizer folder from %localappdata% (Unsupported on XP) to %userprofile% (Supported on XP, now used on Vista+ too)
        {
            try
            {
                string oldlocation = System.Environment.GetEnvironmentVariable("LOCALAPPDATA") + "\\Keppy's Synthesizer\\";
                string newlocation = System.Environment.GetEnvironmentVariable("USERPROFILE") + "\\Keppy's Synthesizer\\";
                if (IsWindowsXP() == false)
                {
                    Directory.Move(oldlocation, newlocation);
                }
            }
            catch
            {

            }
        }

        public static void DriverToSynthMigration() // Basically changes the directory's name
        {
            try
            {
                string oldlocation = System.Environment.GetEnvironmentVariable("USERPROFILE") + "\\Keppy's Driver\\";
                string newlocation = System.Environment.GetEnvironmentVariable("USERPROFILE") + "\\Keppy's Synthesizer\\";
                Directory.Move(oldlocation, newlocation);
            }
            catch
            {

            }
        }

        public static void SaveList(String SelectedList) // Saves the selected list to the hard drive
        {
            using (StreamWriter sw = new StreamWriter(SelectedList))
            {
                foreach (var item in KeppySynthConfiguratorMain.Delegate.Lis.Items)
                {
                    sw.WriteLine(item.ToString());
                }
            }
        }

        public static void SaveSettings() // Saves the settings to the registry 
        {
            /*
             * Key: HKEY_CURRENT_USER\Software\Keppy's Synthesizer\Settings\
             */
            try
            {
                // Normal settings
                KeppySynthConfiguratorMain.SynthSettings.SetValue("polyphony", KeppySynthConfiguratorMain.Delegate.PolyphonyLimit.Value.ToString(), RegistryValueKind.DWord);
                KeppySynthConfiguratorMain.SynthSettings.SetValue("cpu", KeppySynthConfiguratorMain.Delegate.MaxCPU.Value, RegistryValueKind.DWord);
                if (string.IsNullOrWhiteSpace(KeppySynthConfiguratorMain.Delegate.Frequency.Text))
                {
                    KeppySynthConfiguratorMain.Delegate.Frequency.Text = "48000";
                    KeppySynthConfiguratorMain.SynthSettings.SetValue("frequency", KeppySynthConfiguratorMain.Delegate.Frequency.Text, RegistryValueKind.DWord);
                }
                else
                {
                    KeppySynthConfiguratorMain.SynthSettings.SetValue("frequency", KeppySynthConfiguratorMain.Delegate.Frequency.Text, RegistryValueKind.DWord);
                }

                // Advanced SynthSettings
                KeppySynthConfiguratorMain.SynthSettings.SetValue("buflen", KeppySynthConfiguratorMain.Delegate.bufsize.Value.ToString(), RegistryValueKind.DWord);
                KeppySynthConfiguratorMain.SynthSettings.SetValue("tracks", KeppySynthConfiguratorMain.Delegate.TracksLimit.Value.ToString(), RegistryValueKind.DWord);

                // Let's not forget about the volume!
                int VolumeValue = 0;
                double x = KeppySynthConfiguratorMain.Delegate.VolTrackBar.Value / 100;
                VolumeValue = Convert.ToInt32(x);
                KeppySynthConfiguratorMain.Delegate.VolSimView.Text = VolumeValue.ToString("000\\%");
                KeppySynthConfiguratorMain.Delegate.VolIntView.Text = "Value: " + KeppySynthConfiguratorMain.Delegate.VolTrackBar.Value.ToString("00000");
                KeppySynthConfiguratorMain.SynthSettings.SetValue("volume", KeppySynthConfiguratorMain.Delegate.VolTrackBar.Value.ToString(), RegistryValueKind.DWord);

                // Checkbox stuff yay
                if (KeppySynthConfiguratorMain.Delegate.Preload.Checked == true)
                {
                    KeppySynthConfiguratorMain.SynthSettings.SetValue("preload", "1", RegistryValueKind.DWord);
                }
                else
                {
                    KeppySynthConfiguratorMain.SynthSettings.SetValue("preload", "0", RegistryValueKind.DWord);
                }
                if (KeppySynthConfiguratorMain.Delegate.DisableSFX.Checked == true)
                {
                    KeppySynthConfiguratorMain.SynthSettings.SetValue("nofx", "1", RegistryValueKind.DWord);
                }
                else
                {
                    KeppySynthConfiguratorMain.SynthSettings.SetValue("nofx", "0", RegistryValueKind.DWord);
                }
                if (KeppySynthConfiguratorMain.Delegate.VMSEmu.Checked == true)
                {
                    KeppySynthConfiguratorMain.SynthSettings.SetValue("vmsemu", "1", RegistryValueKind.DWord);
                }
                else
                {
                    KeppySynthConfiguratorMain.SynthSettings.SetValue("vmsemu", "0", RegistryValueKind.DWord);
                }
                if (KeppySynthConfiguratorMain.Delegate.NoteOffCheck.Checked == true)
                {
                    KeppySynthConfiguratorMain.SynthSettings.SetValue("noteoff", "1", RegistryValueKind.DWord);
                }
                else
                {
                    KeppySynthConfiguratorMain.SynthSettings.SetValue("noteoff", "0", RegistryValueKind.DWord);
                }
                if (KeppySynthConfiguratorMain.Delegate.SysResetIgnore.Checked == true)
                {
                    KeppySynthConfiguratorMain.SynthSettings.SetValue("sysresetignore", "1", RegistryValueKind.DWord);
                }
                else
                {
                    KeppySynthConfiguratorMain.SynthSettings.SetValue("sysresetignore", "0", RegistryValueKind.DWord);
                }
                if (KeppySynthConfiguratorMain.Delegate.OutputWAV.Checked == true)
                {
                    KeppySynthConfiguratorMain.SynthSettings.SetValue("encmode", "1", RegistryValueKind.DWord);
                }
                else
                {
                    KeppySynthConfiguratorMain.SynthSettings.SetValue("encmode", "0", RegistryValueKind.DWord);
                }
                if (KeppySynthConfiguratorMain.Delegate.XAudioDisable.Checked == true)
                {
                    KeppySynthConfiguratorMain.SynthSettings.SetValue("sinc", "0", RegistryValueKind.DWord);
                    KeppySynthConfiguratorMain.SynthSettings.SetValue("xaudiodisabled", "1", RegistryValueKind.DWord);
                }
                else
                {
                    if (KeppySynthConfiguratorMain.Delegate.SincInter.Checked == true)
                    {
                        KeppySynthConfiguratorMain.SynthSettings.SetValue("sinc", "1", RegistryValueKind.DWord);
                    }
                    else
                    {
                        KeppySynthConfiguratorMain.SynthSettings.SetValue("sinc", "0", RegistryValueKind.DWord);
                    }
                    KeppySynthConfiguratorMain.SynthSettings.SetValue("xaudiodisabled", "0", RegistryValueKind.DWord);
                }
            }
            catch
            {
                // Something bad happened hehe
                MessageBox.Show("Fatal error during the execution of this program!\n\nPress OK to quit.", "Fatal error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                Application.Exit();
            }
        }

        // -------------------------
        // Soundfont lists functions

        public static void TriggerReload() // Tells Keppy's Synthesizer to load a specific list
        {
            try
            {
                if (Convert.ToInt32(KeppySynthConfiguratorMain.Watchdog.GetValue("currentsflist")) == KeppySynthConfiguratorMain.whichone)
                {
                    KeppySynthConfiguratorMain.Watchdog.SetValue("rel" + KeppySynthConfiguratorMain.whichone.ToString(), "1", RegistryValueKind.DWord);
                }
            }
            catch
            {
                Functions.InitializeLastPath();
            }
        }

        public static void SetLastPath(string path) // Saves the last path from the SoundfontImport dialog to the registry 
        {
            try
            {
                KeppySynthConfiguratorMain.SynthPaths.SetValue("lastpathsfimport", path);
            }
            catch
            {
                Functions.InitializeLastPath();
            }
        }

        public static void SetLastImportExportPath(string path) // Saves the last path from the ExternalListImport/ExternalListExport dialog to the registry 
        {
            try
            {
                KeppySynthConfiguratorMain.SynthPaths.SetValue("lastpathlistimpexp", path);
            }
            catch { }
        }

        // NOT SUPPORTED ON XP
        public static void OpenFileDialogAddCustomPaths(FileDialog dialog) // Allows you to add favorites to the SoundfontImport dialog
        {
            try
            {
                // Import the blacklist file
                using (StreamReader r = new StreamReader(System.Environment.GetEnvironmentVariable("USERPROFILE").ToString() + "\\Keppy's Synthesizer\\keppymididrv.favlist"))
                {
                    string line;
                    while ((line = r.ReadLine()) != null)
                    {
                        dialog.CustomPlaces.Add(line);
                    }
                    return;
                }
            }
            catch
            {
                return;
            }
        }
        // NOT SUPPORTED ON XP

        public static void ChangeList(string WhichList) // When you select a list from the combobox, it'll load the items from the selected list to the listbox
        {
            try
            {
                if (!System.IO.Directory.Exists(KeppySynthConfiguratorMain.AbsolutePath))
                {
                    Directory.CreateDirectory(KeppySynthConfiguratorMain.AbsolutePath);
                    Directory.CreateDirectory(KeppySynthConfiguratorMain.ListsPath);
                    File.Create(WhichList).Dispose();
                    KeppySynthConfiguratorMain.Delegate.Lis.Items.Clear();
                }
                if (!System.IO.Directory.Exists(KeppySynthConfiguratorMain.ListsPath))
                {
                    Directory.CreateDirectory(KeppySynthConfiguratorMain.ListsPath);
                    File.Create(WhichList).Dispose();
                    KeppySynthConfiguratorMain.Delegate.Lis.Items.Clear();
                }
                if (!System.IO.File.Exists(WhichList))
                {
                    File.Create(WhichList).Dispose();
                    KeppySynthConfiguratorMain.Delegate.Lis.Items.Clear();
                }
                try
                {
                    using (StreamReader r = new StreamReader(WhichList))
                    {
                        string line;
                        KeppySynthConfiguratorMain.Delegate.Lis.Items.Clear();
                        while ((line = r.ReadLine()) != null)
                        {
                            KeppySynthConfiguratorMain.Delegate.Lis.Items.Add(line);
                        }
                    }
                }
                catch
                {
                    // Oops, list is missing
                    File.Create(WhichList).Dispose();
                    MessageBox.Show("The soundfont list was missing, so the configurator automatically created it for you.", "Information", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
            }
            catch (Exception ex)
            {
                // Oops, something went wrong
                MessageBox.Show("Fatal error during the execution of the program.\n\nPress OK to quit.\n\n.NET error:\n" + ex.Message.ToString(), "Fatal error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                Application.ExitThread();
            }
        }

        public static void InitializeLastPath() // Initialize the paths the app saved before (SetLastPath() and SetLastImportExportPath())
        {
            try
            {
                if (KeppySynthConfiguratorMain.SynthPaths.GetValue("lastpathsfimport", null) == null)
                {
                    Registry.CurrentUser.CreateSubKey("SOFTWARE\\Keppy's Synthesizer\\Paths");
                    KeppySynthConfiguratorMain.SynthPaths = Registry.CurrentUser.OpenSubKey("SOFTWARE\\Keppy's Synthesizer\\Paths", true);
                    KeppySynthConfiguratorMain.SynthPaths.SetValue("lastpathsfimport", Environment.GetFolderPath(Environment.SpecialFolder.Desktop), RegistryValueKind.String);
                    KeppySynthConfiguratorMain.LastBrowserPath = KeppySynthConfiguratorMain.SynthPaths.GetValue("lastpathsfimport", Environment.GetFolderPath(Environment.SpecialFolder.Desktop)).ToString();
                    KeppySynthConfiguratorMain.Delegate.SoundfontImport.InitialDirectory = KeppySynthConfiguratorMain.LastBrowserPath;
                }
                else if (KeppySynthConfiguratorMain.SynthPaths.GetValue("lastpathlistimpexp", null) == null)
                {
                    Registry.CurrentUser.CreateSubKey("SOFTWARE\\Keppy's Synthesizer\\Paths");
                    KeppySynthConfiguratorMain.SynthPaths = Registry.CurrentUser.OpenSubKey("SOFTWARE\\Keppy's Synthesizer\\Paths", true);
                    KeppySynthConfiguratorMain.SynthPaths.SetValue("lastpathlistimpexp", Environment.GetFolderPath(Environment.SpecialFolder.Desktop), RegistryValueKind.String);
                    KeppySynthConfiguratorMain.LastImportExportPath = KeppySynthConfiguratorMain.SynthPaths.GetValue("lastpathlistimpexp", Environment.GetFolderPath(Environment.SpecialFolder.Desktop)).ToString();
                    KeppySynthConfiguratorMain.Delegate.ExternalListImport.InitialDirectory = KeppySynthConfiguratorMain.LastImportExportPath;
                    KeppySynthConfiguratorMain.Delegate.ExternalListExport.InitialDirectory = KeppySynthConfiguratorMain.LastImportExportPath;
                }
                else if (KeppySynthConfiguratorMain.SynthPaths.GetValue("lastpathsfimport", null) == null && KeppySynthConfiguratorMain.SynthPaths.GetValue("lastpathlistimpexp", null) == null)
                {
                    Registry.CurrentUser.CreateSubKey("SOFTWARE\\Keppy's Synthesizer\\Paths");
                    KeppySynthConfiguratorMain.SynthPaths = Registry.CurrentUser.OpenSubKey("SOFTWARE\\Keppy's Synthesizer\\Paths", true);
                    KeppySynthConfiguratorMain.SynthPaths.SetValue("lastpathsfimport", Environment.GetFolderPath(Environment.SpecialFolder.Desktop), RegistryValueKind.String);
                    KeppySynthConfiguratorMain.SynthPaths.SetValue("lastpathlistimpexp", Environment.GetFolderPath(Environment.SpecialFolder.Desktop), RegistryValueKind.String);
                    KeppySynthConfiguratorMain.LastBrowserPath = KeppySynthConfiguratorMain.SynthPaths.GetValue("lastpathsfimport", Environment.GetFolderPath(Environment.SpecialFolder.Desktop)).ToString();
                    KeppySynthConfiguratorMain.LastImportExportPath = KeppySynthConfiguratorMain.SynthPaths.GetValue("lastpathlistimpexp", Environment.GetFolderPath(Environment.SpecialFolder.Desktop)).ToString();
                    KeppySynthConfiguratorMain.Delegate.SoundfontImport.InitialDirectory = KeppySynthConfiguratorMain.LastBrowserPath;
                    KeppySynthConfiguratorMain.Delegate.ExternalListImport.InitialDirectory = KeppySynthConfiguratorMain.LastImportExportPath;
                    KeppySynthConfiguratorMain.Delegate.ExternalListExport.InitialDirectory = KeppySynthConfiguratorMain.LastImportExportPath;
                }
                else
                {
                    KeppySynthConfiguratorMain.LastBrowserPath = KeppySynthConfiguratorMain.SynthPaths.GetValue("lastpathsfimport", Environment.GetFolderPath(Environment.SpecialFolder.Desktop)).ToString();
                    KeppySynthConfiguratorMain.LastImportExportPath = KeppySynthConfiguratorMain.SynthPaths.GetValue("lastpathlistimpexp", Environment.GetFolderPath(Environment.SpecialFolder.Desktop)).ToString();
                    KeppySynthConfiguratorMain.Delegate.SoundfontImport.InitialDirectory = KeppySynthConfiguratorMain.LastBrowserPath;
                    KeppySynthConfiguratorMain.Delegate.ExternalListImport.InitialDirectory = KeppySynthConfiguratorMain.LastImportExportPath;
                    KeppySynthConfiguratorMain.Delegate.ExternalListExport.InitialDirectory = KeppySynthConfiguratorMain.LastImportExportPath;
                }
            }
            catch
            {
                Registry.CurrentUser.CreateSubKey("SOFTWARE\\Keppy's Synthesizer\\Paths");
                KeppySynthConfiguratorMain.SynthPaths = Registry.CurrentUser.OpenSubKey("SOFTWARE\\Keppy's Synthesizer\\Paths", true);
                KeppySynthConfiguratorMain.SynthPaths.SetValue("lastpathsfimport", Environment.GetFolderPath(Environment.SpecialFolder.Desktop), RegistryValueKind.String);
                KeppySynthConfiguratorMain.SynthPaths.SetValue("lastpathlistimpexp", Environment.GetFolderPath(Environment.SpecialFolder.Desktop), RegistryValueKind.String);
                KeppySynthConfiguratorMain.LastBrowserPath = KeppySynthConfiguratorMain.SynthPaths.GetValue("lastpathsfimport", Environment.GetFolderPath(Environment.SpecialFolder.Desktop)).ToString();
                KeppySynthConfiguratorMain.LastImportExportPath = KeppySynthConfiguratorMain.SynthPaths.GetValue("lastpathlistimpexp", Environment.GetFolderPath(Environment.SpecialFolder.Desktop)).ToString();
                KeppySynthConfiguratorMain.Delegate.SoundfontImport.InitialDirectory = KeppySynthConfiguratorMain.LastBrowserPath;
                KeppySynthConfiguratorMain.Delegate.ExternalListImport.InitialDirectory = KeppySynthConfiguratorMain.LastImportExportPath;
                KeppySynthConfiguratorMain.Delegate.ExternalListExport.InitialDirectory = KeppySynthConfiguratorMain.LastImportExportPath;
                KeppySynthConfiguratorMain.Delegate.SoundfontImport.InitialDirectory = KeppySynthConfiguratorMain.LastBrowserPath;
            }
        }

        public static void ReinitializeList(Exception ex, String selectedlistpath) // The app encountered an error, so it'll restore the original soundfont list back
        {
            try
            {
                MessageBox.Show("Your computer doesn't seem to like soundfont lists.\n\nThe configurator encountered an error while trying to save the following list:\n" + selectedlistpath + "\n\n.NET error:\n" + ex.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                KeppySynthConfiguratorMain.Delegate.Lis.Items.Clear();
                using (StreamReader r = new StreamReader(selectedlistpath))
                {
                    string line;
                    while ((line = r.ReadLine()) != null)
                    {
                        KeppySynthConfiguratorMain.Delegate.Lis.Items.Add(line);
                    }
                }
            }
            catch
            {
                MessageBox.Show("Fatal error during the execution of this program!\n\nPress OK to quit.", "Fatal error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                Environment.Exit(-1);
            }
        }
    }
}