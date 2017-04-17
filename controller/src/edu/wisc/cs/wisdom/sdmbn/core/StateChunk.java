package edu.wisc.cs.wisdom.sdmbn.core;

import java.util.Arrays;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ConcurrentLinkedQueue;

import edu.wisc.cs.wisdom.sdmbn.Middlebox;
import edu.wisc.cs.wisdom.sdmbn.json.PutAllflowsMessage;
import edu.wisc.cs.wisdom.sdmbn.json.PutMessage;
import edu.wisc.cs.wisdom.sdmbn.json.PutMultiflowMessage;
import edu.wisc.cs.wisdom.sdmbn.json.PutPerflowMessage;
import edu.wisc.cs.wisdom.sdmbn.json.StateAllflowsMessage;
import edu.wisc.cs.wisdom.sdmbn.json.StateMessage;
import edu.wisc.cs.wisdom.sdmbn.json.StateMultiflowMessage;
import edu.wisc.cs.wisdom.sdmbn.json.StatePerflowMessage;

/** keep track of information associated 
 *  with a specific launched connection*/
public class StateChunk implements Callable<Boolean>
{
	private int hashkey;
	private boolean ack;
	private StateMessage msg;
	private List<Middlebox> dsts;
	
	// Queue of buffered events
	private ConcurrentLinkedQueue<ReprocessEvent> eventsList;

	//Number of NFs from which we received ack for this shared statechunk
	private int numAckedInstances;
	
	public StateChunk(int hashkey) 
	{
		this.hashkey = hashkey;
		this.ack = false;
		this.eventsList  = new ConcurrentLinkedQueue<ReprocessEvent>();
		this.numAckedInstances = 0;
		this.msg = null;
		this.dsts = null;
	}
	
	public void storeStateMessage(StateMessage msg, Middlebox dst)
	{ this.storeStateMessage(msg, Arrays.asList(new Middlebox[] {dst})); }
	
	public void storeStateMessage(StateMessage msg, List<Middlebox> dsts)
	{
		this.msg = msg;
		this.dsts = dsts;
	}
	
	public Boolean call()
	{
		if (null == this.msg || null == this.dsts)
		{ return false; }
		
		PutMessage putMsg = null;
		
		if (this.msg instanceof StatePerflowMessage)
		{				
			StatePerflowMessage pfMsg = (StatePerflowMessage)this.msg;
			putMsg = new PutPerflowMessage();
			((PutPerflowMessage)putMsg).seq = pfMsg.seq;
			((PutPerflowMessage)putMsg).key = pfMsg.key;
		}
		else if (this.msg instanceof StateMultiflowMessage)
		{				
			StateMultiflowMessage mfMsg = (StateMultiflowMessage)this.msg;
			putMsg = new PutMultiflowMessage();
	        ((PutMultiflowMessage)putMsg).seq = mfMsg.seq;
	        ((PutMultiflowMessage)putMsg).key = mfMsg.key;
		}
		else if (this.msg instanceof StateAllflowsMessage)
		{ putMsg = new PutAllflowsMessage(); }
		
		if (putMsg != null)
		{
			putMsg.id = this.msg.id;
			putMsg.hashkey = this.msg.hashkey;
			putMsg.state = this.msg.state;
			
			for (Middlebox dst : this.dsts)
			{
		        try
		        { dst.getStateChannel().sendMessage(putMsg); }
		        catch(Exception e)
		        { return false; }
			}
			
			//this.msg = null;
			//this.dsts = null;
			return true;
		}
		
		return false;
	}
		
	public synchronized void receivedPutAck() 
	{
		this.ack = true;
		while (!this.eventsList.isEmpty())
		{ this.eventsList.poll().process(); }
	}
	
	public synchronized boolean isAcked() 
	{ return this.ack; }
	
	public synchronized int numAckedInst() 
	{ return this.numAckedInstances; }
	
	public void incrementAcked() 
	{ this.numAckedInstances++; }
		
	public void receivedReprocess(ReprocessEvent event) 
	{
		if (this.isAcked())
		{ event.process(); }
		else
		{ this.eventsList.add(event); }
	}
	
	public String toString()
	{ return String.format("StateChunk-0x%08X", this.hashkey); }
}



