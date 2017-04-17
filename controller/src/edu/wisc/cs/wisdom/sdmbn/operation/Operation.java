package edu.wisc.cs.wisdom.sdmbn.operation;

import java.util.concurrent.ConcurrentHashMap;

import net.floodlightcontroller.core.IOFSwitch;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import edu.wisc.cs.wisdom.sdmbn.Middlebox;
import edu.wisc.cs.wisdom.sdmbn.core.StateChunk;
import edu.wisc.cs.wisdom.sdmbn.json.DelPerflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.DelMultiflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.EventsAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetPerflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetMultiflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetAllflowsAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetConfigAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.PutPerflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.PutMultiflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.PutAllflowsAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.PutConfigAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.ReprocessMessage;
import edu.wisc.cs.wisdom.sdmbn.json.StatePerflowMessage;
import edu.wisc.cs.wisdom.sdmbn.json.StateMultiflowMessage;
import edu.wisc.cs.wisdom.sdmbn.json.StateAllflowsMessage;
import edu.wisc.cs.wisdom.sdmbn.json.StateConfigMessage;

public abstract class Operation 
{
	protected static Logger log = LoggerFactory.getLogger("sdmbn.Operation");
	
	protected IOperationManager manager;
	private int id;
	private volatile boolean finished;
	private volatile boolean failed;
	private ConcurrentHashMap<Integer,StateChunk> stateChunks;
	
	private static int nextOperationId = 1;
	
	public Operation(IOperationManager manager)
	{
		this.manager = manager;
		this.id = nextOperationId++;
		this.finished = false;
		this.failed = false;
		this.stateChunks = new ConcurrentHashMap<Integer,StateChunk>();
	}
	
	public int getId()
	{ return this.id; }
	
	public boolean isFinished()
	{ return this.finished; }
	
	public boolean isFailed()
	{ return this.failed; }
	
	public void finish()
	{
		this.finished = true;
		manager.operationFinished(this);
	}
	
	public void fail()
	{
		this.failed = true;
		manager.operationFailed(this); 
	}
	
	protected StateChunk obtainStateChunk(int hashkey)
	{
		StateChunk chunk;
		synchronized(this.stateChunks) 
		{
			if (stateChunks.containsKey(hashkey))
			{ chunk = this.stateChunks.get(hashkey); }
			else
			{
				chunk = new StateChunk(hashkey);
				this.stateChunks.put(hashkey, chunk);
			}
		}
		return chunk;
	}
	
	protected void flushStateChunks()
	{ this.stateChunks.clear(); }
	
	public abstract int execute();
	
	
	public void receiveStatePerflow(StatePerflowMessage msg) {}
	public void receiveStateMultiflow(StateMultiflowMessage msg) {}
	public void receiveStateAllflows(StateAllflowsMessage msg) {}
	public void receiveStateConfig(StateConfigMessage msg) {}
	public void receiveGetPerflowAck(GetPerflowAckMessage msg) {}
	public void receiveGetMultiflowAck(GetMultiflowAckMessage msg) {}
	public void receiveGetAllflowsAck(GetAllflowsAckMessage msg) {}
	public void receiveGetConfigAck(GetConfigAckMessage msg) {}
	public void receivePutPerflowAck(PutPerflowAckMessage msg) {}
	public void receivePutMultiflowAck(PutMultiflowAckMessage msg) {}
	public void receivePutAllflowsAck(PutAllflowsAckMessage msg) {}
	public void receivePutConfigAck(PutConfigAckMessage msg) {}
	public void receiveDelPerflowAck(DelPerflowAckMessage msg) {}
    public void receiveDelMultiflowAck(DelMultiflowAckMessage msg) {}
	public void receiveEventsAck(EventsAckMessage msg, Middlebox from) {}
	public void receiveReprocess(ReprocessMessage msg, Middlebox from) {}
	public void receivePacket(byte[] packet, IOFSwitch inSwitch, short inPort) {}
}
