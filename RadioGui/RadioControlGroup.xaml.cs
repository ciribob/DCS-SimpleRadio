using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
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

namespace RadioGui
{
    /// <summary>
    /// Interaction logic for RadioControlGroup.xaml
    /// </summary>
    public partial class RadioControlGroup : UserControl
    {
        const double MHz = 1000000;
        public int radioId;
        public RadioControlGroup()
        {
            InitializeComponent();
        }

        private void up01_Click(object sender, RoutedEventArgs e)
        {
            sendMessage(MHz/10);
        }

        private void up1_Click(object sender, RoutedEventArgs e)
        {
            sendMessage(MHz);
        }

        private void up10_Click(object sender, RoutedEventArgs e)
        {
            sendMessage(MHz * 10);
        }

        private void down10_Click(object sender, RoutedEventArgs e)
        {
            sendMessage(MHz * -10);
        }

        private void down1_Click(object sender, RoutedEventArgs e)
        {
            sendMessage(MHz * -1);
        }

        private void down01_Click(object sender, RoutedEventArgs e)
        {
            sendMessage((MHz/10) * -1);
        }

        private void sendMessage(double frequency)
        {
            try
            {
                RadioCommand update = new RadioCommand();
                update.freq = frequency;
                update.radio = this.radioId;


                UdpClient client = new UdpClient();
                IPEndPoint ip = new IPEndPoint(IPAddress.Parse("239.255.50.10"), 5060);
                byte[] bytes = Encoding.ASCII.GetBytes(JsonConvert.SerializeObject(update)+"\n");
                client.Send(bytes, bytes.Length, ip);
                client.Close();
            }
            catch (Exception e) { }
            
            
        }
    }
}
