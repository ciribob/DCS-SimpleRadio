SR.io = {}
SR.io.connections = {}

SR.io.LuaSocketConnection = {
	conn = nil,
}
function SR.io.LuaSocketConnection:create(args)
	local self = SR.util.shallowCopy(SR.io.LuaSocketConnection)
	return self
end
function SR.io.LuaSocketConnection:close()
	self.conn:close()
end

SR.io.DefaultMulticastSender = {}
function SR.io.DefaultMulticastSender:create()
	local self = SR.io.LuaSocketConnection:create()
	SR.util.shallowCopy(SR.io.DefaultMulticastSender, self)
	return self
end
function SR.io.DefaultMulticastSender:init()
	self.conn = socket.udp()
	self.conn:settimeout(0)
end
function SR.io.DefaultMulticastSender:send(msg)
	socket.try(self.conn:sendto(msg, "239.255.50.10", 5050))
end


SR.io.UDPSender = {}
function SR.io.UDPSender:create(args)
	local self = SR.io.LuaSocketConnection:create()
	SR.util.shallowCopy(SR.io.UDPSender, self)
	self.port = args.port or 7777
	self.host = args.host or "127.0.0.1"
	return self
end
function SR.io.UDPSender:init()
	self.conn = socket.udp()
	self.conn:settimeout(0)
end
function SR.io.UDPSender:send(msg)
	socket.try(self.conn:sendto(msg, self.host, self.port))
end

function SR.io.send(msg)
	
	for _,_connection in pairs(SR.io.connections) do
		_connection:send(msg)
	end
	

end

SR.io.connections = {
	SR.io.DefaultMulticastSender:create()
}
