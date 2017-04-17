package edu.wisc.cs.wisdom.sdmbn.json;

public abstract class StateMessage extends Message 
{
	public int hashkey;
	public String state;
	
	public StateMessage(String type)
	{ super(type); }
	
	public StateMessage(AllFieldsMessage msg)
	{
		super(msg); 
		this.hashkey = msg.hashkey;
		this.state = msg.state;
	}
	
	public abstract String toString();
}
