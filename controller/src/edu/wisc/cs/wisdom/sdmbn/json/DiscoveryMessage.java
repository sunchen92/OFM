package edu.wisc.cs.wisdom.sdmbn.json;

public class DiscoveryMessage extends Message
{
	public String host;
	public int pid;
	
	public DiscoveryMessage()
	{ super(Message.RESPONSE_SYN); }
	
	public DiscoveryMessage(AllFieldsMessage msg) throws MessageException
	{
		super(msg);
		if (!msg.type.equals(Message.RESPONSE_SYN))
		{
			throw new MessageException(
					String.format("Cannot construct %s from message of type %s",
							this.getClass().getSimpleName(), msg.type), msg); 
		}
		this.host = msg.host;
		this.pid = msg.pid;
	}
	
	public String toString()
	{ return "{id="+id+",type=\""+type+"\",host=\""+host+"\",pid="+pid+"}"; }
}
