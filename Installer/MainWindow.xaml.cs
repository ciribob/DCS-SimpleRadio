using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using MahApps.Metro.Controls;
using System.Diagnostics;

using Microsoft.Win32;
using System.IO;

namespace Installer
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow
    {
        const string REG_PATH = "HKEY_CURRENT_USER\\SOFTWARE\\DCS-SimpleRadio";

        const string version = "1.1.9";

        string currentPath;
        string currentDirectory;
        public MainWindow()
        {
            InitializeComponent();

            this.intro.Content = this.intro.Content + " v"+ version;

            //allows click and drag anywhere on the window
            this.containerPanel.MouseLeftButtonDown += GridPanel_MouseLeftButtonDown;

            string ts3PathStr = ReadTS3Path();
            if (ts3PathStr != "")
            {
                teamspeakPath.Text = ts3PathStr;
            }
            string srPathStr = ReadSRPath();
            if (srPathStr != "")
            {
                srPath.Text = srPathStr;
            }

            //To get the location the assembly normally resides on disk or the install directory
            currentPath = System.Reflection.Assembly.GetExecutingAssembly().CodeBase;

            //once you have the path you get the directory with:
            currentDirectory = System.IO.Path.GetDirectoryName(currentPath);

            if (currentDirectory.StartsWith("file:\\"))
            {
                currentDirectory = currentDirectory.Replace("file:\\", "");
            }
        }

        private string ReadTS3Path()
        {
            // Your default value is returned if the name/value pair
            // does not exist.
            string ts3Path = (string)Registry.GetValue(REG_PATH,
                "TS3Path",
                "");

            return ts3Path == null ? "" : ts3Path;
        }

        private void WriteTS3Path(String path)
        {
            Registry.SetValue(REG_PATH,
                "TS3Path",
                path);
        }

        private string ReadSRPath()
        {
            string srPath = (string)Registry.GetValue(REG_PATH,
                "SRPath",
                "");

            return srPath == null ? "" : srPath;
        }

        private void WriteSRPath(String path)
        {
            Registry.SetValue(REG_PATH,
                "SRPath",
                path);
        }

        private void DeleteRegKeys()
        {
            Registry.SetValue(REG_PATH,
                "SRPath",
                "");
            Registry.SetValue(REG_PATH,
              "TS3Path",
              "");
        }

        private bool Is_Teamspeak_running()
        {
            foreach (Process clsProcess in Process.GetProcesses())
            {
                if (clsProcess.ProcessName.ToLower().Contains("ts3client"))
                {
                    return true;
                }
            }
            return false;

        }
        //
        private bool Is_SimpleRadio_running()
        {
            foreach (Process clsProcess in Process.GetProcesses())
            {
                if (clsProcess.ProcessName.ToLower().Contains("dcs-simpleradio"))
                {
                    return true;
                }
            }
            return false;

        }

        private void GridPanel_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            DragMove();
        }

        private void Locate_teamspeak_button(object sender, RoutedEventArgs e)
        {
            // Create OpenFileDialog
            System.Windows.Forms.FolderBrowserDialog dlg = new System.Windows.Forms.FolderBrowserDialog();


            // Display OpenFileDialog by calling ShowDialog method
            System.Windows.Forms.DialogResult result = dlg.ShowDialog();

            // Get the selected file name and display in a TextBox
            if (result.ToString() == "OK")
            {
                // Open document
                string filename = dlg.SelectedPath;
                teamspeakPath.Text = filename;

            }
        }

        private void Set_Install_Path(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.FolderBrowserDialog dlg = new System.Windows.Forms.FolderBrowserDialog();
            System.Windows.Forms.DialogResult result = dlg.ShowDialog();
            if (result.ToString() == "OK")
            {
                // Open document
                string filename = dlg.SelectedPath;

                if (!filename.EndsWith("\\"))
                {
                    filename = filename + "\\";
                }
                filename = filename + "DCS-SimpleRadio\\";

                srPath.Text = filename;

            }
        }

        private void Install_Beta(object sender, RoutedEventArgs e)
        {
            InstallSR(true);
        }

        private void Install_Release(object sender, RoutedEventArgs e)
        {
            InstallSR(false);
        }

        private void InstallSR(bool beta)
        {
            if (this.Is_Teamspeak_running())
            {
                MessageBox.Show("Please close Teamspeak 3 before installing!", "DCS-SimpleRadio Installer", MessageBoxButton.OK, MessageBoxImage.Error);

                return;
            }

            if (this.Is_SimpleRadio_running())
            {
                MessageBox.Show("Please close SimpleRadio Overlay before updating!", "DCS-SimpleRadio Installer", MessageBoxButton.OK, MessageBoxImage.Error);

                return;
            }

            //check plugin path

            string teamspeakPlugins = teamspeakPath.Text + "\\plugins";

            if (!Directory.Exists(teamspeakPlugins))
            {
                MessageBox.Show("Check TeamSpeak Path - no plugins folder found", "DCS-SimpleRadio Installer", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            string savedGamesPath = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile) + "\\Saved Games\\";

            string dcsPath = savedGamesPath + "DCS";

            if (beta)
            {
                dcsPath = dcsPath + ".openbeta";
            }

            if (!Directory.Exists(dcsPath))
            {
                if (beta)
                {
                    MessageBox.Show("Unable to find DCS Open Beta Profile in Saved Games", "DCS-SimpleRadio Installer", MessageBoxButton.OK, MessageBoxImage.Error);
                }
                else
                {
                    MessageBox.Show("Unable to find DCS Profile in Saved Games", "DCS-SimpleRadio Installer", MessageBoxButton.OK, MessageBoxImage.Error);
                }

                return;
            }

            string scriptsPath = dcsPath + "\\Scripts";

            InstallScripts(scriptsPath);

            InstallPlugins(teamspeakPlugins);

            //install program
            InstallProgram(this.srPath.Text);

            //TODO save registry settings
            WriteSRPath(this.srPath.Text);
            WriteTS3Path(teamspeakPath.Text);

            MessageBox.Show("Installation / Update Completed Succesfully!", "DCS-SimpleRadio Installer", MessageBoxButton.OK, MessageBoxImage.Information);

            //open to installation location
            System.Diagnostics.Process.Start("explorer.exe", (this.srPath.Text));
        }
        public void InstallProgram(string path)
        {
            Directory.CreateDirectory(path);

            File.Delete(path + "\\DCS-SimpleRadio.exe");
            File.Delete(path + "\\Newtonsoft.Json.dll");

            File.Copy(currentDirectory + "\\DCS-SimpleRadio\\DCS-SimpleRadio.exe", path + "\\DCS-SimpleRadio.exe");
            File.Copy(currentDirectory + "\\DCS-SimpleRadio\\Newtonsoft.Json.dll", path + "\\Newtonsoft.Json.dll");

        }

        private void InstallScripts(string path)
        {
            //if scripts folder doesnt exist, create it
            Directory.CreateDirectory(path);

            bool write = true;
            //does it contain an export.lua?
            if (File.Exists(path + "\\Export.lua"))
            {
                String contents = File.ReadAllText(path + "\\Export.lua");

                if (contents.Contains("Scripts\\DCS-SimpleRadio\\SimpleRadioInit.lua"))
                {
                    write = false;

                }
            }

            if (write)
            {
                StreamWriter writer = File.AppendText(path + "\\Export.lua");

                writer.WriteLine("\n\n dofile(lfs.writedir()..[[Scripts\\DCS-SimpleRadio\\SimpleRadioInit.lua]])");
                writer.Close();
            }

            //delete old one
            if (Directory.Exists(path + "\\DCS-SimpleRadio"))
            {
                DeleteDirectory(path + "\\DCS-SimpleRadio");
            }


            //now copy DCS-SimpleRadio to scripts  folder
            Directory.CreateDirectory(path + "\\DCS-SimpleRadio");

            File.Copy(currentDirectory + "\\DCS-SimpleRadio\\Scripts\\DCS-SimpleRadio\\SimpleRadioInit.lua", path + "\\DCS-SimpleRadio\\SimpleRadioInit.lua");
        }

        //http://stackoverflow.com/questions/329355/cannot-delete-directory-with-directory-deletepath-true
        //Recursive Directory Delete
        public static void DeleteDirectory(string target_dir)
        {
            string[] files = Directory.GetFiles(target_dir);
            string[] dirs = Directory.GetDirectories(target_dir);

            foreach (string file in files)
            {
                File.SetAttributes(file, FileAttributes.Normal);
                File.Delete(file);
            }

            foreach (string dir in dirs)
            {
                DeleteDirectory(dir);
            }

            Directory.Delete(target_dir, false);
        }

        private void InstallPlugins(string path)
        {
            //remove old ones
            File.Delete(path + "\\DCS-SimpleRadio-x64.dll");
            File.Delete(path + "\\DCS-SimpleRadio-x86.dll");

            File.Copy(currentDirectory + "\\DCS-SimpleRadio\\DCS-SimpleRadio-x64.dll", path + "\\DCS-SimpleRadio-x64.dll");
            File.Copy(currentDirectory + "\\DCS-SimpleRadio\\DCS-SimpleRadio-x86.dll", path + "\\DCS-SimpleRadio-x86.dll");
        }


        private void UninstallSR()
        {
            if (this.Is_Teamspeak_running())
            {
                MessageBox.Show("Please close Teamspeak 3 before removing!", "DCS-SimpleRadio Installer", MessageBoxButton.OK, MessageBoxImage.Error);

                return;
            }

            if (this.Is_SimpleRadio_running())
            {
                MessageBox.Show("Please close SimpleRadio Overlay before removing!", "DCS-SimpleRadio Installer", MessageBoxButton.OK, MessageBoxImage.Error);

                return;
            }

            //check plugin path

            string teamspeakPlugins = teamspeakPath.Text + "\\plugins";

            if (!Directory.Exists(teamspeakPlugins))
            {
                MessageBox.Show("Check TeamSpeak Path - no plugins folder found", "DCS-SimpleRadio Installer", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            string savedGamesPath = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile) + "\\Saved Games\\";

            string dcsPath = savedGamesPath + "DCS";

            string scriptsPath = dcsPath + "\\Scripts";

            RemoveScripts(dcsPath + ".openbeta\\Scripts");
            RemoveScripts(dcsPath + "\\Scripts");

            RemovePlugin(teamspeakPlugins);

            if (Directory.Exists(srPath.Text) && File.Exists(srPath.Text + "\\DCS-SimpleRadio.exe")) 
            {
                DeleteDirectory(srPath.Text);
            }

            DeleteRegKeys();

            MessageBox.Show("DCS-SimpleRadio Removed Successfully!", "DCS-SimpleRadio Installer", MessageBoxButton.OK, MessageBoxImage.Information);
        }


        private void RemoveScripts(string path)
        {
          
            //does it contain an export.lua?
            if (File.Exists(path + "\\Export.lua"))
            {
                String contents = File.ReadAllText(path + "\\Export.lua");

                if (contents.Contains("Scripts\\DCS-SimpleRadio\\SimpleRadioInit.lua"))
                {
                    contents = contents.Replace("dofile(lfs.writedir()..[[Scripts\\DCS-SimpleRadio\\SimpleRadioInit.lua]])","");

                    File.WriteAllText(path + "\\Export.lua", contents);
                }
            }
           
            //delete old one
            if (Directory.Exists(path + "\\DCS-SimpleRadio"))
            {
                DeleteDirectory(path + "\\DCS-SimpleRadio");
            }
        }


        private void RemovePlugin(string path)
        {
            //remove old ones
            File.Delete(path + "\\DCS-SimpleRadio-x64.dll");
            File.Delete(path + "\\DCS-SimpleRadio-x86.dll");
        }

        private void Remove_Plugin(object sender, RoutedEventArgs e)
        {
            UninstallSR();
        }
    }
}
