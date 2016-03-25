-- Version 1.4.2
-- Special thanks to Cap. Zeen, Tarres and Splash for all the help
-- with getting the radio information :)
SR = {}

SR.enableInternalPTT = false -- set this to TRUE to use the in-cockpit PTT
SR.unicast = false -- if you've setup DCS Correctly and the plugin isn't talking to DCS,
-- set unicast to true and type "/sr switch" in the TeamSpeak chat window

SR.dbg = {}
SR.logFile = io.open(lfs.writedir()..[[Logs\DCS-SimpleRadio.log]], "w")
function SR.log(str)
    if SR.logFile then
        SR.logFile:write(str.."\n")
        SR.logFile:flush()
    end
end

package.path  = package.path..";.\\LuaSocket\\?.lua"
package.cpath = package.cpath..";.\\LuaSocket\\?.dll"

local socket = require("socket")

local JSON = loadfile("Scripts\\JSON.lua")()
SR.JSON = JSON


--dofile(lfs.writedir()..[[Scripts\DCS-SimpleRadio\SimpleRadio-IO.lua]])

SR.UDPSendSocket = socket.udp()
SR.UDPSendSocket:settimeout(0)

-- Prev Export functions.
local _prevExport = {}
_prevExport.LuaExportActivityNextEvent = LuaExportActivityNextEvent

LuaExportActivityNextEvent = function(tCurrent)
    local tNext = tCurrent + 0.2

    local _status,_result = pcall(function()

        local _update  =
        {
            name = "",
            unit = "",
            selected = -1,
            unitId = -1,

            radios =
            {
                { id = 1, name = "init", frequency = 0, modulation = 0, volume = 1.0, secondaryFrequency = 1, freqMin = 200*1000000, freqMax = 400*1000000},
                { id = 2, name = "init", frequency = 0, modulation = 0, volume = 1.0, secondaryFrequency = 1, freqMin = 100*1000000, freqMax = 200*1000000 },
                { id = 3, name = "init", frequency = 0, modulation = 0, volume = 1.0, secondaryFrequency = 1, freqMin = 10*1000000, freqMax = 60*1000000 }
            },
            hasRadio = true,
            groundCommander = false,
        }

        local _data = LoGetSelfData()

        if _data ~= nil then


            _update.name =  _data.UnitName
            _update.unit = _data.Name
            _update.unitId = LoGetPlayerPlaneId()
            --            _update.pos.x = _data.Position.x
            --            _update.pos.y = _data.Position.z

            if _update.unit == "UH-1H" then
                _update = SR.exportRadioUH1H(_update)
            elseif _update.unit == "Ka-50" then
                _update = SR.exportRadioKA50(_update)
            elseif _update.unit == "Mi-8MT" then
                _update = SR.exportRadioMI8(_update)
            elseif string.find(_update.unit, "L-39")  then
                _update = SR.exportRadioL39(_update)
            elseif _update.unit == "A-10C" then
                _update = SR.exportRadioA10C(_update)
            elseif _update.unit == "F-86F Sabre" then
                _update = SR.exportRadioF86Sabre(_update)
            elseif _update.unit == "MiG-15bis" then
                _update = SR.exportRadioMIG15(_update)
            elseif _update.unit == "MiG-21Bis" then
                _update = SR.exportRadioMIG21(_update)
            elseif _update.unit == "P-51D" or  _update.unit == "TF-51D" then
                _update = SR.exportRadioP51(_update)
            elseif _update.unit == "FW-190D9" then
                _update = SR.exportRadioFW190(_update)
            elseif _update.unit == "Bf-109K-4" then
                _update = SR.exportRadioBF109(_update)
            elseif _update.unit == "C-101EB" then
                _update = SR.exportRadioC101(_update)
            elseif _update.unit == "Hawk" then
                _update = SR.exportRadioHawk(_update)
            elseif _update.unit == "M-2000C" then
                _update = SR.exportRadioM2000C(_update)
			elseif _update.unit == "A-10A" then
				_update = SR.exportRadioA10A(_update)
			elseif _update.unit == "F-15C" then
				_update = SR.exportRadioF15C(_update)
			elseif _update.unit == "MiG-29A" or  _update.unit == "MiG-29S" or  _update.unit == "MiG-29G" then
				_update = SR.exportRadioMiG29(_update)
			elseif _update.unit == "Su-27" or  _update.unit == "Su-33" then
				_update = SR.exportRadioSU27(_update)
			elseif _update.unit == "Su-25" or  _update.unit == "Su-25T" then
				_update = SR.exportRadioSU25(_update)
            else
                -- FC 3
                _update.radios[1].name = "FC3 UHF"
                _update.radios[1].frequency = 251.0*1000000
                _update.radios[1].modulation = 0
                _update.radios[1].secondaryFrequency = 243.0*1000000

                _update.radios[2].name = "FC3 VHF"
                _update.radios[2].frequency = 124.8*1000000
                _update.radios[2].modulation = 0
                _update.radios[2].secondaryFrequency = 121.5*1000000

                _update.radios[3].name = "FC3 FM"
                _update.radios[3].frequency = 30.0*1000000
                _update.radios[3].modulation = 1

                _update.radios[1].volume = 1.0
                _update.radios[2].volume = 1.0
                _update.radios[3].volume = 1.0

                _update.hasRadio = false;

                _update.selected = 1
            end

            if SR.unicast then
                socket.try(SR.UDPSendSocket:sendto(SR.JSON:encode(_update).." \n", "127.0.0.1", 5056))
            else
                socket.try(SR.UDPSendSocket:sendto(SR.JSON:encode(_update).." \n", "239.255.50.10", 5050))
            end

        else

            --Ground Commander or spectator
            local _update  =
            {
                name = "Unknown",
                unit = "CA",
                selected = 0,
                radios =
                {
                    { id = 1, name = "CA UHF", frequency = 251.0*1000000, modulation = 0,volume = 1.0, secondaryFrequency = 243.0*1000000, freqMin = 200*1000000, freqMax = 400*1000000 },
                    { id = 2, name = "CA VHF", frequency = 124.8*1000000, modulation = 0,volume = 1.0, secondaryFrequency = 121.5*1000000, freqMin = 100*1000000, freqMax = 200*1000000  },
                    { id = 3, name = "CA FM", frequency = 30.0*1000000, modulation = 1,volume = 1.0, secondaryFrequency = 1, freqMin = 10*1000000, freqMax = 60*1000000  }
                },
                hasRadio = false,
                groundCommander = true
            }

            if SR.unicast then
                socket.try(SR.UDPSendSocket:sendto(SR.JSON:encode(_update).." \n", "127.0.0.1", 5056))
            else
                socket.try(SR.UDPSendSocket:sendto(SR.JSON:encode(_update).." \n", "239.255.50.10", 5050))
            end

        end

    end)

    if not _status then
        SR.log('ERROR: ' .. _result)
    end

    -- Call original function if it exists
    if _prevExport.LuaExportActivityNextEvent then
        _prevExport.LuaExportActivityNextEvent(tCurrent)
    end

    return tNext
end

function SR.exportRadioA10A(_data)
    _data.radios[1].name = "AN/ARC-164 UHF"
    _data.radios[1].frequency = 251.0*1000000 --225-399.975 MHZ
    _data.radios[1].modulation = 0
    _data.radios[1].secondaryFrequency = 243.0*1000000
    _data.radios[1].volume = 1.0
    _data.radios[1].freqMin = 225*1000000
    _data.radios[1].freqMax = 399.975*1000000

    _data.radios[2].name = "AN/ARC-186(V)"
    _data.radios[2].frequency = 124.8*1000000 --116,00-151,975 MHz
    _data.radios[2].modulation = 0
    _data.radios[2].secondaryFrequency = 121.5*1000000
    _data.radios[2].volume = 1.0
    _data.radios[2].freqMin = 116*1000000
    _data.radios[2].freqMax = 151.975*1000000

    _data.radios[3].name = "AN/ARC-186(V)FM"
    _data.radios[3].frequency = 30.0*1000000 --VHF/FM opera entre 30.000 y 76.000 MHz.
    _data.radios[3].modulation = 1
    _data.radios[3].volume = 1.0
    _data.radios[3].freqMin = 30*1000000
    _data.radios[3].freqMax = 76*1000000

	_data.hasRadio = false;
    _data.selected = 0

    return _data
end

function SR.exportRadioMiG29(_data)

    _data.radios[1].name = "R-862"
    _data.radios[1].frequency = 251.0*1000000 --V/UHF, frequencies are: VHF range of 100 to 149.975 MHz and UHF range of 220 to 399.975 MHz
    _data.radios[1].modulation = 0
    _data.radios[1].secondaryFrequency = 121.5*1000000
    _data.radios[1].volume = 1.0
    _data.radios[1].freqMin = 100*1000000
    _data.radios[1].freqMax = 399.975*1000000

    _data.radios[2].name = "No Radio"
    _data.radios[2].frequency = 0
    _data.radios[2].modulation = 3
    _data.radios[2].volume = 1.0

    _data.radios[3].name = "No radio"
    _data.radios[3].frequency = 0
    _data.radios[3].modulation = 3
    _data.radios[3].volume = 1

	_data.hasRadio = false;
    _data.selected = 0

    return _data
end

function SR.exportRadioSU25(_data)

    _data.radios[1].name = "R-862"
    _data.radios[1].frequency = 251.0*1000000 --V/UHF, frequencies are: VHF range of 100 to 149.975 MHz and UHF range of 220 to 399.975 MHz
    _data.radios[1].modulation = 0
    _data.radios[1].secondaryFrequency = 121.5*1000000
    _data.radios[1].volume = 1.0
    _data.radios[1].freqMin = 100*1000000
    _data.radios[1].freqMax = 399.975*1000000

    _data.radios[2].name = "R-828"
    _data.radios[2].frequency = 30.0*1000000 --20 - 60 MHz.
    _data.radios[2].modulation = 1
    _data.radios[2].volume = 1.0
    _data.radios[2].freqMin = 20*1000000
    _data.radios[2].freqMax = 59.975*1000000

    _data.radios[3].name = "No radio"
    _data.radios[3].frequency = 0
    _data.radios[3].modulation = 3 -- no radio
    _data.radios[3].volume = 1

	_data.hasRadio = false;
    _data.selected = 0

    return _data
end

function SR.exportRadioSU27(_data)

    _data.radios[1].name = "R-800"
    _data.radios[1].frequency = 251.0*1000000 --V/UHF, frequencies are: VHF range of 100 to 149.975 MHz and UHF range of 220 to 399.975 MHz
    _data.radios[1].modulation = 0
    _data.radios[1].secondaryFrequency = 121.5*1000000
    _data.radios[1].volume = 1.0
    _data.radios[1].freqMin = 100*1000000
    _data.radios[1].freqMax = 399.975*1000000

    _data.radios[2].name = "R-864"
    _data.radios[2].frequency = 3.5*1000000 --HF frequencies in the 3-10Mhz, like the Jadro
    _data.radios[2].modulation = 0
    _data.radios[2].volume = 1.0
    _data.radios[2].freqMin = 3*1000000
    _data.radios[2].freqMax = 10*1000000


    _data.radios[3].name = "No radio"
    _data.radios[3].frequency = 0
    _data.radios[3].modulation = 3
    _data.radios[3].volume = 1

	_data.hasRadio = false;
    _data.selected = 0

    return _data
end

function SR.exportRadioF15C(_data)

    _data.radios[1].name = "UHF-1"
    _data.radios[1].frequency = 251.0*1000000 --225 to 399.975MHZ
    _data.radios[1].modulation = 0
    _data.radios[1].secondaryFrequency = 243.0*1000000
    _data.radios[1].volume = 1.0
    _data.radios[1].freqMin = 225*1000000
    _data.radios[1].freqMax = 399.975*1000000

    _data.radios[2].name = "UHF-2"
    _data.radios[2].frequency = 231.0*1000000 --225 to 399.975MHZ
    _data.radios[2].modulation = 0
    _data.radios[2].freqMin = 225*1000000
    _data.radios[2].freqMax = 399.975*1000000


    _data.radios[3].name = "No radio"
    _data.radios[3].frequency = 0
    _data.radios[3].modulation = 3 -- to avoid to show a number until we can "hide the non radios"
    _data.radios[3].volume = 1

	_data.hasRadio = false;
    _data.selected = 0

    return _data
end

function SR.exportRadioUH1H(_data)

    _data.radios[1].name = "AN/ARC-51BX - UHF"
    _data.radios[1].frequency = SR.getRadioFrequency(22)
    _data.radios[1].modulation = 0
    _data.radios[1].volume = SR.getRadioVolume(0, 21,{0.0,1.0},true)

    _data.radios[2].name = "AN/ARC-134"
    _data.radios[2].frequency = SR.getRadioFrequency(20)
    _data.radios[2].modulation = 0
    _data.radios[2].volume =  SR.getRadioVolume(0, 8,{0.0,0.65},false )

    _data.radios[3].name = "AN/ARC-131"
    _data.radios[3].frequency = SR.getRadioFrequency(23)
    _data.radios[3].modulation = 1
    _data.radios[3].volume = SR.getRadioVolume(0, 37,{0.3,1.0},true)

    --guard mode for UHF Radio
    local uhfModeKnob = SR.getSelectorPosition(17,0.1)
	if uhfModeKnob == 2 and _data.radios[1].frequency > 1000 then
		_data.radios[1].secondaryFrequency = 243.0*1000000 
	end

    local _panel = GetDevice(0)

    local switch = _panel:get_argument_value(30)

    if SR.nearlyEqual(switch, 0.2, 0.03) then
        _data.selected = 2
    elseif SR.nearlyEqual(switch, 0.3, 0.03) then
        _data.selected = 0
    elseif SR.nearlyEqual(switch, 0.4, 0.03) then
        _data.selected = 1
    else
        _data.selected = -1
    end

    return _data

end


function SR.exportRadioKA50(_data)

    local _panel = GetDevice(0)

    _data.radios[1].name = "R-800L14 VHF/UHF"
    _data.radios[1].frequency = SR.getRadioFrequency(48)

    -- Get modulation mode
    local switch = _panel:get_argument_value(417)
    if SR.nearlyEqual(switch, 0.0, 0.03) then
        _data.radios[1].modulation = 1
    else
        _data.radios[1].modulation = 0
    end
    _data.radios[1].volume = 1.0 -- no volume knob??

    _data.radios[2].name = "R-828"
    _data.radios[2].frequency = SR.getRadioFrequency(49,50000)
    _data.radios[2].modulation = 1
    _data.radios[2].volume = SR.getRadioVolume(0, 372,{0.0,1.0},false)

    _data.radios[3].name = "N/A"
    _data.radios[3].frequency = 1
    _data.radios[3].modulation = 3
    _data.radios[3].volume = 1.0

    local switch = _panel:get_argument_value(428)

    if SR.nearlyEqual(switch, 0.0, 0.03) then
        _data.selected = 0
    elseif SR.nearlyEqual(switch, 0.1, 0.03) then
        _data.selected = 1
    else
        _data.selected = -1
    end

    return _data

end

function SR.exportRadioMI8(_data)

    _data.radios[1].name = "R-863"
    _data.radios[1].frequency = SR.getRadioFrequency(38)
    
    local _modulation = GetDevice(0):get_argument_value(369)
    if _modulation > 0.5 then
        _data.radios[1].modulation = 1
    else
        _data.radios[1].modulation = 0
    end
    
    _data.radios[1].volume = SR.getRadioVolume(0, 156,{0.0,1.0},false)

    _data.radios[2].name = "R-828"
    _data.radios[2].frequency = SR.getRadioFrequency(39,50000)
    _data.radios[2].modulation = 1
    _data.radios[2].volume = SR.getRadioVolume(0, 737,{0.0,1.0},false)

    _data.radios[3].name = "JADRO-1A"
    _data.radios[3].frequency = SR.getRadioFrequency(37,500)
    _data.radios[3].modulation = 0
    _data.radios[3].volume = SR.getRadioVolume(0, 743,{0.0,1.0},false)

    --guard mode for R-863 Radio
    local uhfModeKnob = SR.getSelectorPosition(153,1)
	if uhfModeKnob == 1 and _data.radios[1].frequency > 1000 then
		_data.radios[1].secondaryFrequency = 121.5*1000000 
	end

    -- Get selected radio from SPU-9
    local _switch = SR.getSelectorPosition(550,0.1)

    if _switch == 0 then
        _data.selected = 0
    elseif _switch == 1 then
        _data.selected = 2
    elseif _switch == 2 then
        _data.selected = 1
    else
        _data.selected = -1
    end

    return _data

end

function SR.exportRadioL39(_data)

    _data.radios[1].name = "R-832M"
    _data.radios[1].frequency = SR.getRadioFrequency(19)
    _data.radios[1].modulation = 0
    _data.radios[1].volume = SR.getRadioVolume(0, 289,{0.0,0.8},false)

    _data.radios[2].name = "Intercom"
    _data.radios[2].frequency =100.0
    _data.radios[2].modulation = 2 --Special intercom modulation
    _data.radios[2].volume =1.0

    _data.radios[3].name = "No Radio"
    _data.radios[3].frequency =1.0
    _data.radios[3].modulation = 3
    _data.radios[3].volume = 0

    -- Intercom button depressed
    if(SR.getButtonPosition(133) > 0.5 or SR.getButtonPosition(546) > 0.5) then
        _data.selected = 1
    elseif (SR.getButtonPosition(134) > 0.5 or SR.getButtonPosition(547) > 0.5) then
        _data.selected= 0
    else
        if SR.enableInternalPTT then
            _data.selected = -1 -- PTT Not pressed
        else
            _data.selected = 0
        end
    end

    return _data
end


function SR.exportRadioA10C(_data)

    _data.radios[1].name = "AN/ARC-164 UHF"
    _data.radios[1].frequency = SR.getRadioFrequency(54)
    _data.radios[1].modulation = 0
    _data.radios[1].volume = SR.getRadioVolume(0, 171,{0.0,1.0},false)

    _data.radios[2].name = "AN/ARC-186(V)"
    _data.radios[2].frequency =  SR.getRadioFrequency(55)
    _data.radios[2].modulation = 0
    _data.radios[2].volume = SR.getRadioVolume(0, 133,{0.0,1.0},false)

    _data.radios[3].name = "AN/ARC-186(V)FM"
    _data.radios[3].frequency =  SR.getRadioFrequency(56)
    _data.radios[3].modulation = 1
    _data.radios[3].volume = SR.getRadioVolume(0, 147,{0.0,1.0},false)

     --guard mode for UHF Radio
    local uhfModeKnob = SR.getSelectorPosition(168,0.1)
	if uhfModeKnob == 2 and _data.radios[1].frequency > 1000 then
		_data.radios[1].secondaryFrequency = 243.0*1000000 
	end

    local value = GetDevice(0):get_argument_value(239)

    local n = math.abs(tonumber(string.format("%.0f", (value - 0.4) / 0.1)))

    if n == 3 then
        _data.selected = 2
    elseif  n == 2 then
        _data.selected = 1
    elseif  n == 1 then
        _data.selected = 0
    else
        _data.selected = -1
    end

    return _data
end

function SR.exportRadioF86Sabre(_data)

    _data.radios[1].name = "AN/ARC-27"
    _data.radios[1].frequency =  SR.getRadioFrequency(26)
    _data.radios[1].modulation = 0
    _data.radios[1].volume = SR.getRadioVolume(0, 806,{0.1,0.9},false)

    _data.radios[2].name = "N/A"
    _data.radios[2].frequency = 1
    _data.radios[2].modulation = 3

    _data.radios[3].name = "N/A"
    _data.radios[3].frequency =  1
    _data.radios[3].modulation = 3

    _data.radios[2].volume = 1.0
    _data.radios[3].volume = 1.0

    _data.selected = 0

    --guard mode for UHF Radio
    local uhfModeKnob = SR.getSelectorPosition(805,0.1)
	if uhfModeKnob == 2 and _data.radios[1].frequency > 1000 then
		_data.radios[1].secondaryFrequency = 243.0*1000000 
	end

    if SR.enableInternalPTT then
        -- Check PTT
        if(SR.getButtonPosition(213)) > 0.5 then
            _data.selected = 0
        else
            _data.selected = -1
        end
    end

    return _data;
end

function SR.exportRadioMIG15(_data)

    _data.radios[1].name = "RSI-6K"
    _data.radios[1].frequency =  SR.getRadioFrequency(30)
    _data.radios[1].modulation = 0
    _data.radios[1].volume = SR.getRadioVolume(0, 126,{0.1,0.9},false)

    _data.radios[2].name = "N/A"
    _data.radios[2].frequency = 1
    _data.radios[2].modulation = 3

    _data.radios[3].name = "N/A"
    _data.radios[3].frequency =  1
    _data.radios[3].modulation = 3

    _data.radios[2].volume = 1.0
    _data.radios[3].volume = 1.0

    _data.selected = 0

    if SR.enableInternalPTT then
        -- Check PTT
        if(SR.getButtonPosition(202)) > 0.5 then
            _data.selected = 0
        else
            _data.selected = -1
        end
    end


    return _data;
end

function SR.exportRadioMIG21(_data)

    _data.radios[1].name = "R-832"
    _data.radios[1].frequency =  SR.getRadioFrequency(22)
    _data.radios[1].modulation = 0
    _data.radios[1].volume = SR.getRadioVolume(0, 210,{0.0,1.0},false)

    _data.radios[2].name = "N/A"
    _data.radios[2].frequency = 1
    _data.radios[2].modulation = 3

    _data.radios[3].name = "N/A"
    _data.radios[3].frequency =  1
    _data.radios[3].modulation = 3

    _data.radios[2].volume = 1.0
    _data.radios[3].volume = 1.0

    _data.selected = 0

    if SR.enableInternalPTT then
        if(SR.getButtonPosition(315)) > 0.5 then
            _data.selected = 0
        else
            _data.selected = -1
        end
    end

    return _data;
end

function SR.exportRadioP51(_data)

    _data.radios[1].name = "SCR522A"
    _data.radios[1].frequency =  SR.getRadioFrequency(24)
    _data.radios[1].modulation = 0
    _data.radios[1].volume = SR.getRadioVolume(0, 116,{0.0,1.0},false)

    _data.radios[2].name = "N/A"
    _data.radios[2].frequency = 1
    _data.radios[2].modulation = 3

    _data.radios[3].name = "N/A"
    _data.radios[3].frequency =  1
    _data.radios[3].modulation = 3


    _data.radios[2].volume = 1.0
    _data.radios[3].volume = 1.0

    _data.selected = 0

    if SR.enableInternalPTT then
        if(SR.getButtonPosition(44)) > 0.5 then
            _data.selected = 0
        else
            _data.selected = -1
        end
    end

    return _data;
end

function SR.exportRadioFW190(_data)

    _data.radios[1].name = "FuG 16ZY"
    _data.radios[1].frequency = SR.getRadioFrequency(15)
    _data.radios[1].modulation = 0
    _data.radios[1].volume = 1.0  --SR.getRadioVolume(0, 83,{0.0,1.0},true) Volume knob is not behaving..

    _data.radios[2].name = "N/A"
    _data.radios[2].frequency = 1
    _data.radios[2].modulation = 3

    _data.radios[3].name = "N/A"
    _data.radios[3].frequency =  1
    _data.radios[3].modulation = 3


    _data.radios[2].volume = 1.0
    _data.radios[3].volume = 1.0

    _data.selected = 0

    return _data;
end

function SR.exportRadioBF109(_data)

    _data.radios[1].name = "FuG 16ZY"
    _data.radios[1].frequency =  SR.getRadioFrequency(14)
    _data.radios[1].modulation = 0
    _data.radios[1].volume = SR.getRadioVolume(0, 130,{0.0,1.0},false)

    _data.radios[2].name = "N/A"
    _data.radios[2].frequency = 1
    _data.radios[2].modulation = 3

    _data.radios[3].name = "N/A"
    _data.radios[3].frequency =  1
    _data.radios[3].modulation = 3


    _data.radios[2].volume = 1.0
    _data.radios[3].volume = 1.0

    _data.selected = 0

    return _data;
end

function SR.exportRadioC101(_data)

    --    local _count = 0
    --
    --    local status,result;
    --
    --    while(true) do
    --        status,result = pcall(function(_c)
    --            return SR.getRadioFrequency(_c)
    --        end, _count)
    --
    --        if not status and _count < 1000 then
    --            SR.log('ERROR: ' .. result)
    --            _count = _count +1
    --            result = 0.0
    --        else
    --            break
    --        end
    --
    --    end
    local MHZ = 1000000

    _data.radios[1].name = "UHF"

    local _selector = SR.getSelectorPosition(232,0.25)

    if _selector == 1 or _selector == 2 then

        local _hundreds = SR.round(SR.getKnobPosition(0, 226,{0.1,0.3},{1,3}),0.1)*100*1000000
        local _tens = SR.round(SR.getKnobPosition(0, 227,{0.0,0.9},{0,9}),0.1)*10*1000000
        local _ones = SR.round(SR.getKnobPosition(0, 228,{0.0,0.9},{0,9}),0.1)*1000000
        local _tenth = SR.round(SR.getKnobPosition(0, 229,{0.0,0.9},{0,9}),0.1)*100000
        local _hundreth = SR.round(SR.getKnobPosition(0, 230,{0.0,0.3},{0,3}),0.1)*10000

        _data.radios[1].frequency = _hundreds+_tens+_ones+_tenth+_hundreth
    else
        _data.radios[1].frequency = 1
    end
    _data.radios[1].modulation = 0
    _data.radios[1].volume = SR.getRadioVolume(0, 234,{0.0,1.0},false)


    _data.radios[2].name = "VHF"

    local _vhfPower = SR.getSelectorPosition(413,1.0)

    if _vhfPower == 1 then

        local _tens = (SR.round(SR.getKnobPosition(0, 415,{0.1,0.4},{1,4}),0.1)/2.5) *10*1000000
        local _ones = SR.round(SR.getKnobPosition(0, 416,{0.0,0.9},{0,9}),0.1)*1000000
        local _tenth = SR.round(SR.getKnobPosition(0, 417,{0.0,0.9},{0,9}),0.1)*100000
        local _hundreth = SR.round(SR.getKnobPosition(0, 418,{0.0,0.3},{0,3}),0.1)*10000

        _data.radios[2].frequency =(MHZ*110) + _tens+_ones+_tenth+_hundreth
    else
        _data.radios[2].frequency = 1
    end
    _data.radios[2].modulation = 0
    _data.radios[2].volume = SR.getRadioVolume(0, 412,{0.0,1.0},false)

    _data.radios[3].name = "No Radio"
    _data.radios[3].frequency =  1
    _data.radios[3].modulation = 0
    _data.radios[3].volume = 1.0

    local _selector = SR.getSelectorPosition(404,0.5)

    if  _selector == 1 then
        _data.selected = 0
    elseif  _selector == 2 then
        _data.selected = 1
    else
        _data.selected = -1
    end

    return _data;
end

function SR.exportRadioHawk(_data)

    local MHZ = 1000000

    _data.radios[1].name = "UHF"

    local _selector = SR.getSelectorPosition(221,0.25)

    if _selector == 1 or _selector == 2 then

        local _hundreds = SR.getSelectorPosition(226,0.25)*100*MHZ
        local _tens = SR.round(SR.getKnobPosition(0, 227,{0.0,0.9},{0,9}),0.1)*10*MHZ
        local _ones = SR.round(SR.getKnobPosition(0, 228,{0.0,0.9},{0,9}),0.1)*MHZ
        local _tenth = SR.round(SR.getKnobPosition(0, 229,{0.0,0.9},{0,9}),0.1)*100000
        local _hundreth = SR.round(SR.getKnobPosition(0, 230,{0.0,0.3},{0,3}),0.1)*10000

        _data.radios[1].frequency = _hundreds+_tens+_ones+_tenth+_hundreth
    else
        _data.radios[1].frequency = 1
    end
    _data.radios[1].modulation = 0
    _data.radios[1].volume = 1


    _data.radios[2].name = "No Radio"
    _data.radios[2].frequency =1
    _data.radios[2].modulation = 0
    _data.radios[2].volume =1

    _data.radios[3].name = "No Radio"
    _data.radios[3].frequency =  1
    _data.radios[3].modulation = 0
    _data.radios[3].volume = 1.0


    _data.selected = 0

    return _data;
end

function SR.exportRadioM2000C(_data)

    _data.radios[1].name = "UHF"
    _data.radios[1].frequency = SR.getRadioFrequency(20)
    _data.radios[1].modulation = 0
    _data.radios[1].volume = SR.getRadioVolume(0, 706,{0.0,1.0},false)

    _data.radios[2].name = "V/UHF"
    _data.radios[2].frequency =  SR.getRadioFrequency(19)
    _data.radios[2].modulation = 0
    _data.radios[2].volume = SR.getRadioVolume(0, 707,{0.0,1.0},false)

    _data.radios[3].name = "N/A"
    _data.radios[3].frequency =  1
    _data.radios[3].modulation = 0
    _data.radios[3].volume = 0

    local _switch = SR.getButtonPosition(700)

    if _switch == 1 then
        _data.selected = 0
    else
        _data.selected = 1
    end

    --guard mode for V/UHF Radio
    local uhfModeKnob = SR.getSelectorPosition(446,0.25) -- TODO!
	if uhfModeKnob == 2 and _data.radios[2].frequency > 1000 then
		_data.radios[2].secondaryFrequency = 243.0*1000000 
	end

    return _data


end



function SR.getRadioVolume(_deviceId, _arg,_minMax,_invert)

    local _device = GetDevice(_deviceId)

    if not _minMax then
        _minMax = {0.0,1.0}
    end

    if _device then
        local _val = tonumber(_device:get_argument_value(_arg))
        local _reRanged = SR.rerange(_val,_minMax,{0.0,1.0})  --re range to give 0.0 - 1.0

        if _invert then
            return  SR.round(math.abs(1.0 - _reRanged),0.005)
        else
            return SR.round(_reRanged,0.005);
        end
    end
    return 1.0
end

function SR.getKnobPosition(_deviceId, _arg,_minMax,_mapMinMax)

    local _device = GetDevice(_deviceId)

    if _device then
        local _val = tonumber(_device:get_argument_value(_arg))
        local _reRanged = SR.rerange(_val,_minMax,_mapMinMax)

        return _reRanged
    end
    return -1
end

function SR.getSelectorPosition(_args,_step)
    local _value = GetDevice(0):get_argument_value(_args)
    local _num = math.abs(tonumber(string.format("%.0f", (_value) / _step)))

    return _num

end

function SR.getButtonPosition(_args)
    local _value = GetDevice(0):get_argument_value(_args)

    return _value

end


function SR.getRadioFrequency(_deviceId, _roundTo)
    local _device = GetDevice(_deviceId)

    if not _roundTo then
        _roundTo = 5000
    end


    if _device then
        if _device:is_on() then
            -- round as the numbers arent exact
            return SR.round(_device:get_frequency(),_roundTo)
        end
    end
    return 1
end

function SR.rerange(_val,_minMax,_limitMinMax)
    return ((_limitMinMax[2] - _limitMinMax[1]) * (_val - _minMax[1]) / (_minMax[2] - _minMax[1])) + _limitMinMax[1];

end

function SR.round(number, step)
    if number == 0 then
        return 0
    else
        return math.floor((number + step / 2) / step) * step
    end
end

function SR.nearlyEqual(a, b, diff)
    return math.abs(a - b) < diff
end