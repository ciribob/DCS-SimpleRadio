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

            radio1Frequency.Text = "Unknown";
        
            radio2Frequency.Text = "Unknown";

            radio3Frequency.Text = "Unknown";

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
                    string loggingEvent = "";
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
                        catch(Exception e)
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
                    if (lastUpdate != null && lastUpdate.name !=null)
                    {
                        Application.Current.Dispatcher.Invoke(new Action(() => {

                            //check if current
                            long elapsedTicks =   DateTime.Now.Ticks - lastUpdateTime.Ticks;
                            TimeSpan elapsedSpan = new TimeSpan(elapsedTicks);

                            if(elapsedSpan.Seconds > 3)
                            {
                                this.statusIndicator.Fill = new SolidColorBrush(Colors.Red);
                                this.statusLabel.Content = "Radio Disabled";
                            }
                            else
                            {
                                this.statusIndicator.Fill = new SolidColorBrush(Colors.Green);
                                this.statusLabel.Content = "OK";
                            }

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

                           
                            

                            switch (lastUpdate.selected)
                            {
                                case 0:
                                    radio1Active.Fill = new SolidColorBrush(Colors.Green);
                                    radio2Active.Fill = new SolidColorBrush(Colors.Orange);
                                    radio3Active.Fill = new SolidColorBrush(Colors.Orange);
                                    break;
                                case 1:
                                    radio1Active.Fill = new SolidColorBrush(Colors.Orange);
                                    radio2Active.Fill = new SolidColorBrush(Colors.Green);
                                    radio3Active.Fill = new SolidColorBrush(Colors.Orange);
                                    break;
                                case 2:
                                    radio1Active.Fill = new SolidColorBrush(Colors.Orange);
                                    radio2Active.Fill = new SolidColorBrush(Colors.Orange);
                                    radio3Active.Fill = new SolidColorBrush(Colors.Green);
                                    break;
                            }

                           

                            radio1Frequency.Text = (lastUpdate.radios[0].frequency/MHZ).ToString("0.000") + (lastUpdate.radios[0].modulation==0?"AM":"FM");
                            radio1Label.Content = lastUpdate.radios[0].name;

                            radio2Frequency.Text = (lastUpdate.radios[1].frequency / MHZ).ToString("0.000") + (lastUpdate.radios[1].modulation == 0 ? "AM" : "FM");
                            radio2Label.Content = lastUpdate.radios[1].name;

                            radio3Frequency.Text = (lastUpdate.radios[2].frequency / MHZ).ToString("0.000") + (lastUpdate.radios[2].modulation == 0 ? "AM" : "FM");
                            radio3Label.Content = lastUpdate.radios[2].name;

                        }));
                    }

                 


                }
                
            });

        }

        protected override void OnClosing(CancelEventArgs e)
        {
            base.OnClosing(e);

            this.end = true;

        }

    

        ~MainWindow()
        {
            //Shut down threads
        }

    }
    
}
