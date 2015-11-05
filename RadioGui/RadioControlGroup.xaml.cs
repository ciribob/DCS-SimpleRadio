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
        private bool dragging;
        private RadioUpdate lastUpdate = null;

        private RadioTransmit lastActive = null;

        private DateTime lastActiveTime = new DateTime(0L);

        public RadioControlGroup()
        {
            InitializeComponent();
        }

        private void up01_Click(object sender, RoutedEventArgs e)
        {
            sendFrequencyChange(MHz / 10);
        }

        private void up1_Click(object sender, RoutedEventArgs e)
        {
            sendFrequencyChange(MHz);
        }

        private void up10_Click(object sender, RoutedEventArgs e)
        {
            sendFrequencyChange(MHz * 10);
        }

        private void down10_Click(object sender, RoutedEventArgs e)
        {
            sendFrequencyChange(MHz * -10);
        }

        private void down1_Click(object sender, RoutedEventArgs e)
        {
            sendFrequencyChange(MHz * -1);
        }

        private void down01_Click(object sender, RoutedEventArgs e)
        {
            sendFrequencyChange((MHz / 10) * -1);
        }

        private void sendUDPUpdate(RadioCommand update)
        {

            //only send update if the aircraft doesnt have its own radio system, i.e FC3
            if (lastUpdate!=null && !lastUpdate.hasRadio)
            {
                byte[] bytes = Encoding.ASCII.GetBytes(JsonConvert.SerializeObject(update) + "\n");
                //multicast
                send("239.255.50.10", 5060, bytes);
                //unicast
                send("127.0.0.1", 5061, bytes);
            }
        
        }
        private void send(String ipStr, int port, byte[] bytes)
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
        private void sendFrequencyChange(double frequency)
        {
            RadioCommand update = new RadioCommand();
            update.freq = frequency;
            update.radio = this.radioId;
            update.cmdType = RadioCommand.CmdType.FREQUENCY;

            sendUDPUpdate(update);
        }

        private void radioSelectSwitch(object sender, RoutedEventArgs e)
        {
            RadioCommand update = new RadioCommand();
            update.radio = this.radioId;
            update.cmdType = RadioCommand.CmdType.SELECT;

            sendUDPUpdate(update);
        }

        private void radioFrequencyText_Click(object sender, MouseButtonEventArgs e)
        {
            RadioCommand update = new RadioCommand();
            update.radio = this.radioId;
            update.cmdType = RadioCommand.CmdType.SELECT;

            sendUDPUpdate(update);
        }

        private void radioVolume_DragStarted(object sender, RoutedEventArgs e)
        {
            this.dragging = true;
        }


        private void radioVolume_DragCompleted(object sender, RoutedEventArgs e)
        {
          
            RadioCommand update = new RadioCommand();
            update.radio = this.radioId;
            update.volume = ((float)radioVolume.Value) / 100.0f;
            update.cmdType = RadioCommand.CmdType.VOLUME;

            sendUDPUpdate(update);
            this.dragging = false;
        }


        internal void update(RadioUpdate lastUpdate, TimeSpan elapsedSpan)
        {
            this.lastUpdate = lastUpdate;
            if (elapsedSpan.TotalSeconds > 5)
            {
                radioActive.Fill = new SolidColorBrush(Colors.Red);
                radioLabel.Content = "No Radio";
                radioFrequency.Text = "Unknown";

                radioVolume.IsEnabled = false;
               
                up10.IsEnabled = false;
                up1.IsEnabled = false;
                up01.IsEnabled = false;

                down10.IsEnabled = false;
                down1.IsEnabled = false;
                down01.IsEnabled = false;


                //reset dragging just incase
                this.dragging = false;
            }
            else
            {

                if (this.radioId == lastUpdate.selected)
                {
                    radioActive.Fill = new SolidColorBrush(Colors.Green);
                }
                else
                {
                    radioActive.Fill = new SolidColorBrush(Colors.Orange);
                }

                RadioInformation currentRadio = lastUpdate.radios[this.radioId];

                if(currentRadio.modulation == 2) //intercom
                {
                    radioFrequency.Text = "INTERCOM";
                }
                else
                {
                    radioFrequency.Text = (currentRadio.frequency / MHz).ToString("0.000") + (currentRadio.modulation == 0 ? "AM" : "FM");
                    if(currentRadio.secondaryFrequency > 100)
                    {
                        radioFrequency.Text += " G";
                    }
                }
                radioLabel.Content = lastUpdate.radios[this.radioId].name;

                if (lastUpdate.hasRadio)
                {
                    
                    radioVolume.IsEnabled = false;
                    up10.IsEnabled = false;
                    up1.IsEnabled = false;
                    up01.IsEnabled = false;

                    down10.IsEnabled = false;
                    down1.IsEnabled = false;
                    down01.IsEnabled = false;
                

                    //reset dragging just incase
                    this.dragging = false;
                
                }
                else
                {
                    radioVolume.IsEnabled = true;
                    up10.IsEnabled = true;
                    up1.IsEnabled = true;
                    up01.IsEnabled = true;

                    down10.IsEnabled = true;
                    down1.IsEnabled = true;
                    down01.IsEnabled = true;
                }

                if (this.dragging == false)
                {
                    radioVolume.Value = (currentRadio.volume * 100.0);
                }


            }
        }
     
        public void setLastRadioTransmit(RadioTransmit radio)
        {
            this.lastActive = radio;
            lastActiveTime = DateTime.Now;
        }

        internal void repaintRadioTransmit()
        {
            if (this.lastActive == null)
            {
                radioFrequency.Foreground = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#00FF00"));
            }
            else
            {
                //check if current
                long elapsedTicks = DateTime.Now.Ticks - lastActiveTime.Ticks;
                TimeSpan elapsedSpan = new TimeSpan(elapsedTicks);

                if (elapsedSpan.TotalSeconds > 0.5)
                {
                    radioFrequency.Foreground = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#00FF00"));
                }
                else
                {
                    if (this.lastActive.radio == this.radioId)
                    {
                        if(this.lastActive.secondary)
                        {
                            radioFrequency.Foreground = new SolidColorBrush(Colors.Red);
                        }
                        else
                        {
                            radioFrequency.Foreground = new SolidColorBrush(Colors.White);
                        }
                    }
                    else
                    {
                        radioFrequency.Foreground = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#00FF00"));
                    }
                }
            }
        }
    }
}
