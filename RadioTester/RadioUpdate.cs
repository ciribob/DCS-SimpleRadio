using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RadioTester
{
    class RadioInformation
    {
        public string name = "";
        public double frequency = 1;
        public int modulation = 0;
        public float volume = 1.0f;

    };
    class RadioUpdate
    {

        public string name = "Tester";
        public string unit = "test-radio";
        public int selected = 0;

        public RadioInformation[] radios = new RadioInformation[3];
        public bool hasRadio = false;
        public bool groundCommander = false;
  
    };
}
