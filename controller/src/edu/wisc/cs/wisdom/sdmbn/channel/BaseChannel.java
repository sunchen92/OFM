package edu.wisc.cs.wisdom.sdmbn.channel;

import org.jboss.netty.channel.Channel;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.gson.Gson;

import edu.wisc.cs.wisdom.sdmbn.Middlebox;
import edu.wisc.cs.wisdom.sdmbn.core.SdmbnManager;
import edu.wisc.cs.wisdom.sdmbn.json.Message;
import edu.wisc.cs.wisdom.sdmbn.json.MessageException;

public abstract class BaseChannel
{
	protected Channel channel;
	protected SdmbnManager sdmbnManager;
	protected Gson gson;
	protected String host;
	protected int pid;
	protected Middlebox mb;
	
	protected static Logger log = 
		LoggerFactory.getLogger("sdmbn."+BaseChannel.class.getSimpleName());
	
	public BaseChannel(Channel channel, SdmbnManager sdmbnManager)
	{
		this.channel = channel;
		this.sdmbnManager = sdmbnManager;
		this.gson = new Gson();
		host = null;
	}
	
	public void setMiddlebox(Middlebox mb)
	{ this.mb = mb; }
		
	public String getIp()
	{
		String ip = channel.getRemoteAddress().toString();
		ip = (ip.substring(1)).split(":")[0];
		return ip;
	}
	
	public String getHost()
	{ return this.host; }
	
	public int getPid()
	{ return this.pid; }
	
	public String toString()
	{
		String localPort = channel.getLocalAddress().toString(); 
		localPort = (localPort.substring(1)).split(":")[1];
		return this.host+"."+this.pid+":"+localPort;
	}
	
	protected abstract void processMessage(String line) throws MessageException;
	public abstract void sendMessage(Message msg) throws MessageException;
}
