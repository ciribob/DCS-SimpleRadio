using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RadioGui
{
    class RadioInformation
    {
        public string name = "";
        public double frequency = 1;
        public int modulation = 0;
        public float volume = 1.0f;
        public double secondaryFrequency = 1;

    };
    class RadioUpdate
    {


        public long lastUpdate = 0;
        public string name = "";
        public string unit = "";
        public int selected = 0;

        public RadioInformation[] radios = new RadioInformation[3];
        public bool hasRadio = false;
        public bool allowNonPlayers = true;
        public bool caMode = false;

    };
}
