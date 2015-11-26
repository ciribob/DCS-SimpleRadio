using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Interop;
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
        private const int UdpClientBroadcastPort = 35024;

        private UdpClient activeRadioUdpClient;
        private const int ActiveRadioClientPort = 35025;

        private UdpClient hotkeyClient;
        private const int HotkeyClientClientPort = 35026;

        private volatile bool end;

        private RadioUpdate lastUpdate;

        private DateTime lastUpdateTime = new DateTime(0L);

        const double MHZ = 1000000;

        private double aspectRatio;

        public MainWindow()
        {

            InitializeComponent();

           // this.SourceInitialized += MainWindow_SourceInitialized;

            if (Is_SimpleRadio_running())
            {
                Close();
            }

            //allows click and drag anywhere on the window
            this.containerPanel.MouseLeftButtonDown += WrapPanel_MouseLeftButtonDown;

            radio1.radioId = 0;
        //    this.radio1.radioControlContainer.MouseLeftButtonDown += WrapPanel_MouseLeftButtonDown;

            radio2.radioId = 1;
        //    this.radio2.radioControlContainer.MouseLeftButtonDown += WrapPanel_MouseLeftButtonDown;

            radio3.radioId = 2;
        //    this.radio3.radioControlContainer.MouseLeftButtonDown += WrapPanel_MouseLeftButtonDown;

            SetupActiveRadio();
            SetupRadioStatus();

            SetupHotKeyListener();

        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            aspectRatio = this.ActualWidth / this.ActualHeight;
        }


        /// <summary>
        /// Only allow one instance of SimpleRadio
        /// </summary>
        /// <returns></returns>
        private bool Is_SimpleRadio_running()
        {
            int i = 0;
            foreach (Process clsProcess in Process.GetProcesses())
            {
                if (clsProcess.ProcessName.ToLower().Equals("dcs-simpleradio"))
                {
                    i++;
                }
            }

            return i > 1;
        }

        private void SetupRadioStatus()
        {
            //setup UDP
            this.udpClient = new UdpClient();
            this.udpClient.Client.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReuseAddress, true);
            this.udpClient.ExclusiveAddressUse = false; // only if you want to send/receive on same machine.

            IPAddress multicastaddress = IPAddress.Parse("239.255.50.10");
            udpClient.JoinMulticastGroup(multicastaddress);

            IPEndPoint localEp = new IPEndPoint(IPAddress.Any, UdpClientBroadcastPort);
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
                            var remoteEndPoint = new IPEndPoint(IPAddress.Any, UdpClientBroadcastPort);
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
                                this.muteStatusNonUsers.Fill = new SolidColorBrush(Colors.Red);
                                this.muteStatusNonUsers.ToolTip = "Mute Non Players OFF";
                            }
                            else
                            {
                                this.muteStatusNonUsers.Fill = new SolidColorBrush(Colors.Green);
                                this.muteStatusNonUsers.ToolTip = "Mute Non Players ON";
                            }

                            if (lastUpdate.caMode)
                            {
                                this.caModeStatus.Fill = new SolidColorBrush(Colors.Green);
                                this.caModeStatus.ToolTip = "Radio ON - CA / JTAC / Spectator Mode";

                            }
                            else
                            {
                                this.caModeStatus.Fill = new SolidColorBrush(Colors.Red);
                                this.caModeStatus.ToolTip = "Radio OFF - CA / JTAC / Spectator Mode";
                            }


                            radio1.update(lastUpdate, elapsedSpan);
                            radio2.update(lastUpdate, elapsedSpan);
                            radio3.update(lastUpdate, elapsedSpan);

                        }));
                    }
                }
            });

        }

        private void SetupActiveRadio()
        {
            //setup UDP
            this.activeRadioUdpClient = new UdpClient();
            this.activeRadioUdpClient.Client.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReuseAddress, true);
            this.activeRadioUdpClient.ExclusiveAddressUse = false; // only if you want to send/receive on same machine.

            IPAddress multicastaddress = IPAddress.Parse("239.255.50.10");
            this.activeRadioUdpClient.JoinMulticastGroup(multicastaddress);

            IPEndPoint localEp = new IPEndPoint(IPAddress.Any, ActiveRadioClientPort);
            this.activeRadioUdpClient.Client.Bind(localEp);

            Task.Run(() =>
            {
                using (activeRadioUdpClient)
                {

                    while (!end)
                    {
                        try
                        {
                            //IPEndPoint object will allow us to read datagrams sent from any source.
                            var remoteEndPoint = new IPEndPoint(IPAddress.Any, ActiveRadioClientPort);
                            activeRadioUdpClient.Client.ReceiveTimeout = 10000;
                            var receivedResults = activeRadioUdpClient.Receive(ref remoteEndPoint);

                            RadioTransmit lastRadioTransmit = JsonConvert.DeserializeObject<RadioTransmit>(Encoding.UTF8.GetString(receivedResults));
                            switch (lastRadioTransmit.radio)
                            {
                                case 0:
                                    radio1.setLastRadioTransmit(lastRadioTransmit);
                                    break;
                                case 1:
                                    radio2.setLastRadioTransmit(lastRadioTransmit);
                                    break;
                                case 2:
                                    radio3.setLastRadioTransmit(lastRadioTransmit);
                                    break;
                                default:
                                    break;
                            }
                        }
                        catch (Exception e)
                        {
                            // Console.Out.WriteLine(e.ToString());
                        }


                    }

                    this.activeRadioUdpClient.Close();
                }
            });

            Task.Run(() =>
            {


                while (!end)
                {
                    Thread.Sleep(100);

                    Application.Current.Dispatcher.Invoke(new Action(() =>
                    {
                        radio1.repaintRadioTransmit();
                        radio2.repaintRadioTransmit();
                        radio3.repaintRadioTransmit();

                    }));
                }

            });

        }

        private void SetupHotKeyListener()
        {
            //setup UDP
            this.hotkeyClient = new UdpClient();
            this.hotkeyClient.Client.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReuseAddress, true);
            this.hotkeyClient.ExclusiveAddressUse = false; // only if you want to send/receive on same machine.

            IPAddress multicastaddress = IPAddress.Parse("239.255.50.10");
            this.hotkeyClient.JoinMulticastGroup(multicastaddress);

            IPEndPoint localEp = new IPEndPoint(IPAddress.Any, HotkeyClientClientPort);
            this.hotkeyClient.Client.Bind(localEp);

            Task.Run(() =>
            {
                using (hotkeyClient)
                {

                    while (!end)
                    {
                        try
                        {
                            //IPEndPoint object will allow us to read datagrams sent from any source.
                            var remoteEndPoint = new IPEndPoint(IPAddress.Any, HotkeyClientClientPort);
                            hotkeyClient.Client.ReceiveTimeout = 10000;
                            var receivedResults = hotkeyClient.Receive(ref remoteEndPoint);

                            var hotKeyStr = System.Text.Encoding.UTF8.GetString(receivedResults).Trim();

                            if (hotKeyStr.Contains("DCS-SR-TOGGLE-STATUS"))
                            {
                                Application.Current.Dispatcher.Invoke(new Action(() =>
                                {

                                    if (this.WindowState == WindowState.Normal)
                                    {

                                        this.WindowState = WindowState.Minimized;
                                    }
                                    else
                                    {
                                        this.WindowState = WindowState.Normal;
                                    }

                                }));



                            }

                        }
                        catch (Exception e)
                        {
                            // Console.Out.WriteLine(e.ToString());
                        }
                    }

                    this.hotkeyClient.Close();
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

        private void Button_Minimise(object sender, RoutedEventArgs e)
        {
            WindowState = WindowState.Minimized;
        }

        private void Button_Close(object sender, RoutedEventArgs e)
        {
            this.end = true;
            Close();
        }

        private void windowOpacitySlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            this.Opacity = e.NewValue;
        }

        private void SendUDPCommand(RadioCommand.CmdType type)
        {
            RadioCommand update = new RadioCommand();
            update.freq = 1;
            update.volume = 1;
            update.radio = 0;
            update.cmdType = type;

            byte[] bytes = Encoding.ASCII.GetBytes(JsonConvert.SerializeObject(update) + "\n");
            //multicast
            Send("239.255.50.10", 5060, bytes);
            //unicast
            Send("127.0.0.1", 5061, bytes);


        }
        private void Send(String ipStr, int port, byte[] bytes)
        {
            try
            {

                UdpClient client = new UdpClient();
                IPEndPoint ip = new IPEndPoint(IPAddress.Parse(ipStr), port);

                client.Send(bytes, bytes.Length, ip);
                client.Close();
            }
            catch (Exception e) { }

        }

        private void Button_ToggleMute(object sender, RoutedEventArgs e)
        {
            SendUDPCommand(RadioCommand.CmdType.TOGGLE_MUTE_NON_RADIO);
        }


        private void Button_Toggle_CA_Mode(object sender, RoutedEventArgs e)
        {
            SendUDPCommand(RadioCommand.CmdType.TOGGLE_FORCE_RADIO_ON);
        }

        private void containerPanel_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            //force aspect ratio
            CalculateScale();
        }

        private void CalculateScale()
        {
            double yScale = ActualHeight / this.myMainWindow.MinWidth;
            double xScale = ActualWidth / this.myMainWindow.MinWidth;
            double value = Math.Min(xScale, yScale);
            ScaleValue = (double)OnCoerceScaleValue(myMainWindow, value);
        }

        protected override void OnRenderSizeChanged(SizeChangedInfo sizeInfo)
        {

            if (sizeInfo.WidthChanged)
                this.Width = sizeInfo.NewSize.Height * aspectRatio;
            else
                this.Height = sizeInfo.NewSize.Width / aspectRatio;

            // Console.WriteLine(this.Height +" width:"+ this.Width);

        }

        #region ScaleValue Depdency Property //StackOverflow: http://stackoverflow.com/questions/3193339/tips-on-developing-resolution-independent-application/5000120#5000120
        public static readonly DependencyProperty ScaleValueProperty = DependencyProperty.Register("ScaleValue", typeof(double), typeof(MainWindow), new UIPropertyMetadata(1.0, new PropertyChangedCallback(OnScaleValueChanged), new CoerceValueCallback(OnCoerceScaleValue)));

        private static object OnCoerceScaleValue(DependencyObject o, object value)
        {
            MainWindow mainWindow = o as MainWindow;
            if (mainWindow != null)
                return mainWindow.OnCoerceScaleValue((double)value);
            else
                return value;
        }

        private static void OnScaleValueChanged(DependencyObject o, DependencyPropertyChangedEventArgs e)
        {
            MainWindow mainWindow = o as MainWindow;
            if (mainWindow != null)
                mainWindow.OnScaleValueChanged((double)e.OldValue, (double)e.NewValue);
        }

        protected virtual double OnCoerceScaleValue(double value)
        {
            if (double.IsNaN(value))
                return 1.0f;

            value = Math.Max(0.1, value);
            return value;
        }

        protected virtual void OnScaleValueChanged(double oldValue, double newValue)
        {

        }

        public double ScaleValue
        {
            get
            {
                return (double)GetValue(ScaleValueProperty);
            }
            set
            {
                SetValue(ScaleValueProperty, value);
            }
        }
        #endregion


    }

}
