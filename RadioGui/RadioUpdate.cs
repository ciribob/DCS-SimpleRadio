using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RadioGui
{
    class RadioInformation
    {
       public  string name;
        public double frequency;
        public int modulation;
     
    };
    class RadioUpdate
    {


        public long  lastUpdate;
        public string name;
        public string unit;
        public int selected;
    
        public RadioInformation[] radios = new RadioInformation[3];
        public bool hasRadio;
        public bool allowNonPlayers;

    };
}
