package edu.wisc.cs.wisdom.sdmbn.json;


public class MessageException extends Exception
{	
	private static final long serialVersionUID = 4401679485427454758L;
	
	private Message msg;
	
	public MessageException(String message, Message msg)
	{
		super(message);
		this.msg = msg;
	}
	
	public String toString()
	{ return this.getMessage() + ": " + msg.toString(); }
}
