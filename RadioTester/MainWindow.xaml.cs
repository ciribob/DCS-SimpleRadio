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

namespace RadioTester
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private volatile bool end;

        private DateTime lastUpdateTime = new DateTime(0L);

        const double MHZ = 1000000;

        private volatile int selected = -1;

        public MainWindow()
        {

            InitializeComponent();

            // radio1.radioId = 0;


            //allows click and drag anywhere on the window
            //   this.containerPanel.MouseLeftButtonDown += WrapPanel_MouseLeftButtonDown;

            radio1.radioLabel.Content = "UHF Test";
            radio1.radioId = 0;
            radio1.radioFrequency.Text = "120.0";

            radio2.radioId = 1;
            radio2.radioLabel.Content = "VHF Test";
            radio2.radioFrequency.Text = "150.0";

            radio3.radioId = 2;
            radio3.radioLabel.Content = "FM Test";
            radio3.radioFrequency.Text = "30.0";



            Task.Run(() =>
           {


               while (!end)
               {
                   Thread.Sleep(300);


                   Application.Current.Dispatcher.Invoke(new Action(() =>
                       {
                           RadioUpdate update = new RadioUpdate();

                           update.radios = new RadioInformation[3];

                           update.radios[0] = new RadioInformation();
                           update.radios[0].name = "UHF";
                           update.radios[0].frequency = Double.Parse(radio1.radioFrequency.Text) * MHZ;
                           update.radios[0].volume = (float)radio1.radioVolume.Value;
                           update.radios[0].modulation = 0;

                           update.radios[1] = new RadioInformation();
                           update.radios[1].name = "VHF";
                           update.radios[1].frequency = Double.Parse(radio2.radioFrequency.Text) * MHZ;
                           update.radios[1].volume = (float)radio2.radioVolume.Value;
                           update.radios[1].modulation = 0;

                           update.radios[2] = new RadioInformation();
                           update.radios[2].name = "FM";
                           update.radios[2].frequency = Double.Parse(radio3.radioFrequency.Text) * MHZ;
                           update.radios[2].volume = (float)radio3.radioVolume.Value;
                           update.radios[2].modulation = 1;

                           update.selected = selected;

                           sendUDPUpdate(update);



                       }));

               }
           });
        }


        private void sendUDPUpdate(RadioUpdate update)
        {


            byte[] bytes = Encoding.ASCII.GetBytes(JsonConvert.SerializeObject(update) + "\n");

            //unicast
            send("127.0.0.1", 5056, bytes);


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

        private void select1_Click(object sender, RoutedEventArgs e)
        {
            selected = 0;
        }

        private void select2_Click(object sender, RoutedEventArgs e)
        {
            selected = 1;
        }

        private void select3_Click(object sender, RoutedEventArgs e)
        {
            selected = 2;
        }

        private void select4_Click(object sender, RoutedEventArgs e)
        {
            selected = -1;

        }
    }

}
