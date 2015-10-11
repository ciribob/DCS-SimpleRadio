using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RadioGui
{
    class RadioCommand
    {
        public enum CmdType
        {
            FREQUENCY=1,
            VOLUME=2,
            SELECT=3,
            TOGGLE_MUTE_NON_RADIO = 4,
            TOGGLE_FORCE_RADIO_ON = 5
        }
        public double freq = 1;
        public int radio;
        public float volume = 1.0f;
        public CmdType cmdType;
    }
}
