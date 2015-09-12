using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
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
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {

        private UdpClient udpClient;

        private volatile bool end;

        private RadioUpdate lastUpdate;

        private DateTime lastUpdateTime = new DateTime(0L);

        const double MHZ = 1000000;

        public MainWindow()
        {

            InitializeComponent();

            //allows click and drag anywhere on the window
            this.containerPanel.MouseLeftButtonDown += WrapPanel_MouseLeftButtonDown;

            radio1.radioId = 0;
           
            radio2.radioId = 1;
          
            radio3.radioId = 2;
           
            //setup UDP
            this.udpClient = new UdpClient();
            this.udpClient.Client.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReuseAddress, true);
            this.udpClient.ExclusiveAddressUse = false; // only if you want to send/receive on same machine.

            IPAddress multicastaddress = IPAddress.Parse("239.255.50.10");
            udpClient.JoinMulticastGroup(multicastaddress);

            IPEndPoint localEp = new IPEndPoint(IPAddress.Any, 35024);
            this.udpClient.Client.Bind(localEp);

            Task.Run(() =>
            {
                using (udpClient)
                {

                    while (!end)
                    {
                        try
                        {
                            //IPEndPoint object will allow us to read datagrams sent from any source.
                            var remoteEndPoint = new IPEndPoint(IPAddress.Any, 35024);
                            udpClient.Client.ReceiveTimeout = 10000;
                            var receivedResults = udpClient.Receive(ref remoteEndPoint);

                            lastUpdate = JsonConvert.DeserializeObject<RadioUpdate>(Encoding.UTF8.GetString(receivedResults));

                            lastUpdateTime = DateTime.Now;

                        }
                        catch (Exception e)
                        {
                            Console.Out.WriteLine(e.ToString());
                        }


                    }

                    this.udpClient.Close();
                }
            });

            Task.Run(() =>
           {


               while (!end)
               {
                   Thread.Sleep(100);

                    //check 
                    if (lastUpdate != null && lastUpdate.name != null)
                   {
                       Application.Current.Dispatcher.Invoke(new Action(() =>
                       {

                            //check if current
                            long elapsedTicks = DateTime.Now.Ticks - lastUpdateTime.Ticks;
                           TimeSpan elapsedSpan = new TimeSpan(elapsedTicks);

                           if (lastUpdate.allowNonPlayers)
                           {
                               this.allowNonPlayersIndicator.Fill = new SolidColorBrush(Colors.Green);
                               this.allowNonPlayers.Content = "Mute OFF";
                           }
                           else
                           {
                               this.allowNonPlayersIndicator.Fill = new SolidColorBrush(Colors.Red);
                               this.allowNonPlayers.Content = "Mute ON";
                           }

                           if (elapsedSpan.Seconds > 5)
                           {
                               this.statusIndicator.Fill = new SolidColorBrush(Colors.Red);
                               this.statusLabel.Content = "Radio Disabled";
                           }
                           else
                           {
                               this.statusIndicator.Fill = new SolidColorBrush(Colors.Green);
                               this.statusLabel.Content = "OK";
                           }


                           radio1.update(lastUpdate, elapsedSpan);
                           radio2.update(lastUpdate, elapsedSpan);
                           radio3.update(lastUpdate, elapsedSpan);

                       }));
                   }
               }
           });
        }

        private void WrapPanel_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            DragMove();
        }

        protected override void OnClosing(CancelEventArgs e)
        {
            base.OnClosing(e);

            this.end = true;

        }

        ~MainWindow()
        {
            //Shut down threads
            this.end = true;
        }

    }

}
