package edu.wisc.cs.wisdom.sdmbn;

import net.floodlightcontroller.core.IOFSwitch;
import edu.wisc.cs.wisdom.sdmbn.channel.EventChannel;
import edu.wisc.cs.wisdom.sdmbn.channel.StateChannel;

public class Middlebox 
{
	private String host;
	private int pid;
	private IOFSwitch sw;
	private short switchPort;
	private StateChannel stateChannel;
	private EventChannel eventChannel;
	
	public Middlebox(String host, int pid)
	{
		this.host = host;
		this.pid = pid;
		this.sw = null;
		this.switchPort = -1;
		this.stateChannel = null;
		this.eventChannel = null;
	}
	
	public String getHost()
	{ return this.host; }
	
	public int getPid()
	{ return this.pid; }

	public void setSwitch(IOFSwitch sw, short switchPort)
	{
		this.sw = sw;
		this.switchPort = switchPort;
	}
	
	public IOFSwitch getSwitch()
	{ return this.sw; }
	
	public short getSwitchPort()
	{ return this.switchPort; }
	
	public String getId()
	{ return constructId(this.host,this.pid); }
	
	public static String constructId(String host, int pid)
	{ return host+"."+pid;	}
	
	public void setStateChannel(StateChannel stateChannel)
	{ this.stateChannel = stateChannel; }
	
	public StateChannel getStateChannel()
	{ return this.stateChannel; }
	
	public boolean hasStateChannel()
	{ return (this.stateChannel != null); }
	
	public void setEventChannel(EventChannel eventChannel)
	{ this.eventChannel = eventChannel; }
	
	public EventChannel getEventChannel()
	{ return this.eventChannel; }
	
	public boolean hasEventChannel()
	{ return (this.eventChannel != null); }
	
	public boolean hasSwitch()
	{ return (this.sw != null); }
	
	public boolean isFullyConnected()
	{
		return (this.hasStateChannel() && this.hasEventChannel()
				&& this.hasSwitch());
	}
	
	public String getManagementIP()
	{ 
		if (this.stateChannel != null)
		{ return this.stateChannel.getIp(); }
		if (this.eventChannel != null)
		{ return this.eventChannel.getIp(); }
		return null;
	}
	
	@Override
	public String toString()
	{ return this.getId(); }
}
