SR = {}

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

function SR.shallowCopy(source, dest)
	dest = dest or {}
	for k, v in pairs(source) do
		dest[k] = v
	end
	return dest
end

package.path  = package.path..";.\\LuaSocket\\?.lua"
package.cpath = package.cpath..";.\\LuaSocket\\?.dll"
  
local socket = require("socket")

local JSON = loadfile("Scripts\\JSON.lua")()
SR.JSON = JSON


--dofile(lfs.writedir()..[[Scripts\DCS-SimpleRadio\SimpleRadio-IO.lua]])

UDPSendSocket = socket.udp()
UDPSendSocket:settimeout(0)

-- Prev Export functions.
local _prevExport = {}
_prevExport.LuaExportStart = LuaExportStart
_prevExport.LuaExportStop = LuaExportStop
_prevExport.LuaExportBeforeNextFrame = LuaExportBeforeNextFrame
_prevExport.LuaExportAfterNextFrame = LuaExportAfterNextFrame

-- Lua Export Functions
LuaExportStart = function()
	
		--socket.try(UDPSendSocket:sendto("Start\n\n", "239.255.50.10", 5050))
	
	-- Chain previously-included export as necessary
	if _prevExport.LuaExportStart then
		_prevExport.LuaExportStart()
	end
end

LuaExportStop = function()
	
	--socket.try(UDPSendSocket:sendto("End\n\n", "239.255.50.10", 5050))
	-- Chain previously-included export as necessary
	if _prevExport.LuaExportStop then
		_prevExport.LuaExportStop()
	end
end

LuaExportActivityNextEvent = function(tCurrent)
    local tNext = tCurrent + 0.3

    local _status,_result = pcall(function()
        
        local _update  =
        {
            name = "",
            unit = "",
            selected = -1,

            radios =
            {
                { id = 1, name = "init", frequency = 0, modulation = 0, volume = 1.0 },
                { id = 2, name = "init", frequency = 0, modulation = 0, volume = 1.0 },
                { id = 3, name = "init", frequency = 0, modulation = 0, volume = 1.0 }
            },
            hasRadio = true,
            groundCommander = false,
        }

        local _data = LoGetSelfData()

        if _data ~= nil then


            _update.name =  _data.UnitName
            _update.unit = _data.Name
--            _update.pos.x = _data.Position.x
--            _update.pos.y = _data.Position.z

            if _update.unit == "UH-1H" then
                _update = SR.exportRadioUH1H(_update)
            elseif _update.unit == "Ka-50" then
                _update = SR.exportRadioKA50(_update)
            elseif _update.unit == "Mi-8MT" then
                _update = SR.exportRadioMI8(_update)
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
            else
                -- FC 3
                _update.radios[1].name = "FC3 UHF"
                _update.radios[1].frequency = 251.0*1000000
                _update.radios[1].modulation = 0

                _update.radios[2].name = "FC3 VHF"
                _update.radios[2].frequency = 124.8*1000000
                _update.radios[2].modulation = 0

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
                socket.try(UDPSendSocket:sendto(SR.JSON:encode(_update).." \n", "127.0.0.1", 5056))
            else
                socket.try(UDPSendSocket:sendto(SR.JSON:encode(_update).." \n", "239.255.50.10", 5050))
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
                    { id = 1, name = "CA UHF", frequency = 251.0*1000000, modulation = 0,volume = 1.0 },
                    { id = 2, name = "CA VHF", frequency = 124.8*1000000, modulation = 0,volume = 1.0 },
                    { id = 3, name = "CA FM", frequency = 30.0*1000000, modulation = 1,volume = 1.0 }
                },
                hasRadio = false,
                groundCommander = true
            }

            if SR.unicast then
                socket.try(UDPSendSocket:sendto(SR.JSON:encode(_update).." \n", "127.0.0.1", 5056))
            else
                socket.try(UDPSendSocket:sendto(SR.JSON:encode(_update).." \n", "239.255.50.10", 5050))
            end

        end

    end)

    if not _status then
        SR.log('ERROR: ' .. _result)
    end


    -- Call original function if it exists
    if _prevExport.LuaExportAfterNextFrame then
         _prevExport.LuaExportAfterNextFrame(tCurrent)
    end

    return tNext
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

    --todo Check rerange function


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
    _data.radios[2].modulation = 0
    _data.radios[2].volume = SR.getRadioVolume(0, 372,{0.0,1.0},false)

    _data.radios[3].name = "N/A"
    _data.radios[3].frequency = 1
    _data.radios[3].modulation = 1
    _data.radios[3].volume = 1.0


    -- Get selected radio from SPU-9

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
    _data.radios[1].modulation = 0
    _data.radios[1].volume = SR.getRadioVolume(0, 156,{0.0,1.0},false)

    _data.radios[2].name = "R-828"
    _data.radios[2].frequency = SR.getRadioFrequency(39,50000)
    _data.radios[2].modulation = 0
    _data.radios[2].volume = SR.getRadioVolume(0, 737,{0.0,1.0},false)

    _data.radios[3].name = "JADRO-1A"
    _data.radios[3].frequency = SR.getRadioFrequency(37)
    _data.radios[3].modulation = 0
    _data.radios[3].volume = SR.getRadioVolume(0, 743,{0.0,1.0},false)

    local _panel = GetDevice(0)
    -- Get selected radio from SPU-9

    local switch = _panel:get_argument_value(550)

    if SR.nearlyEqual(switch, 0.0, 0.03) then
        _data.selected = 0
    elseif SR.nearlyEqual(switch, 0.1, 0.03) then
        _data.selected = 2
    elseif SR.nearlyEqual(switch, 0.2, 0.03) then
        _data.selected = 1
    else
        _data.selected = -1
    end

    return _data


end


function SR.exportRadioA10C(_data)


    _data.radios[1].name = "AN/ARC-186(V)"
    _data.radios[1].frequency =  SR.getRadioFrequency(55)
    _data.radios[1].modulation = 0
    _data.radios[1].volume = SR.getRadioVolume(0, 133,{0.0,1.0},false)


    _data.radios[2].name = "AN/ARC-164"
    _data.radios[2].frequency = SR.getRadioFrequency(54)
    _data.radios[2].modulation = 0
    _data.radios[2].volume = SR.getRadioVolume(0, 171,{0.0,1.0},false)

    _data.radios[3].name = "FM"
    _data.radios[3].frequency =  SR.getRadioFrequency(56)
    _data.radios[3].modulation = 1
    _data.radios[3].volume = SR.getRadioVolume(0, 147,{0.0,1.0},false)


    local value = GetDevice(0):get_argument_value(239)

    local n = math.abs(tonumber(string.format("%.0f", (value - 0.4) / 0.1)))

  --  socket.try(UDPSendSocket:sendto(value.. " "..n.."\n\n", "239.255.50.10", 5051))

    if n == 3 then
        _data.selected = 2
    elseif  n == 2 then
        _data.selected = 0
    elseif  n == 1 then
        _data.selected = 1
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
    _data.radios[2].modulation = 0

    _data.radios[3].name = "N/A"
    _data.radios[3].frequency =  1
    _data.radios[3].modulation = 0

    _data.radios[2].volume = 1.0
    _data.radios[3].volume = 1.0

    _data.selected = 0

    return _data;
end

function SR.exportRadioMIG15(_data)

    _data.radios[1].name = "RSI-6K"
    _data.radios[1].frequency =  SR.getRadioFrequency(30)
    _data.radios[1].modulation = 0
    _data.radios[1].volume = SR.getRadioVolume(0, 126,{0.1,0.9},false)

    _data.radios[2].name = "N/A"
    _data.radios[2].frequency = 1
    _data.radios[2].modulation = 0

    _data.radios[3].name = "N/A"
    _data.radios[3].frequency =  1
    _data.radios[3].modulation = 0

    _data.radios[2].volume = 1.0
    _data.radios[3].volume = 1.0

    _data.noMicSwitch = true;

    _data.selected = 0

    return _data;
end

function SR.exportRadioMIG21(_data)

    _data.radios[1].name = "R-828"
    _data.radios[1].frequency =  SR.getRadioFrequency(22)
    _data.radios[1].modulation = 0
    _data.radios[1].volume = SR.getRadioVolume(0, 210,{0.0,1.0},false)

    _data.radios[2].name = "N/A"
    _data.radios[2].frequency = 1
    _data.radios[2].modulation = 0

    _data.radios[3].name = "N/A"
    _data.radios[3].frequency =  1
    _data.radios[3].modulation = 0


    _data.radios[2].volume = 1.0
    _data.radios[3].volume = 1.0

    _data.selected = 0

    return _data;
end

function SR.exportRadioP51(_data)

    _data.radios[1].name = "SCR522A"
    _data.radios[1].frequency =  SR.getRadioFrequency(24)
    _data.radios[1].modulation = 0
    _data.radios[1].volume = SR.getRadioVolume(0, 116,{0.0,1.0},false)

    _data.radios[2].name = "N/A"
    _data.radios[2].frequency = 1
    _data.radios[2].modulation = 0

    _data.radios[3].name = "N/A"
    _data.radios[3].frequency =  1
    _data.radios[3].modulation = 1


    _data.radios[2].volume = 1.0
    _data.radios[3].volume = 1.0

    _data.selected = 0

    return _data;
end

function SR.exportRadioFW190(_data)

    _data.radios[1].name = "FuG 16ZY"
    _data.radios[1].frequency = SR.getRadioFrequency(15)
    _data.radios[1].modulation = 0
    _data.radios[1].volume = 1.0  --SR.getRadioVolume(0, 83,{0.0,1.0},true) Volume knob is not behaving..

    _data.radios[2].name = "N/A"
    _data.radios[2].frequency = 1
    _data.radios[2].modulation = 0

    _data.radios[3].name = "N/A"
    _data.radios[3].frequency =  1
    _data.radios[3].modulation = 0


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
    _data.radios[2].modulation = 0

    _data.radios[3].name = "N/A"
    _data.radios[3].frequency =  1
    _data.radios[3].modulation = 0


    _data.radios[2].volume = 1.0
    _data.radios[3].volume = 1.0

    _data.selected = 0

    return _data;
end


function LuaExportBeforeNextFrame()

    -- Chain previously-included export as
    -- Source DCS-Bios
    if _prevExport.LuaExportBeforeNextFrame then
        _prevExport.LuaExportBeforeNextFrame()
    end

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