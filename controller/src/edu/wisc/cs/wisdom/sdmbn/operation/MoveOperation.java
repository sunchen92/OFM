package edu.wisc.cs.wisdom.sdmbn.operation;

import java.util.Arrays;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

import org.openflow.protocol.OFPort;

import net.floodlightcontroller.core.IOFSwitch;

import edu.wisc.cs.wisdom.sdmbn.Middlebox;
import edu.wisc.cs.wisdom.sdmbn.PerflowKey;
import edu.wisc.cs.wisdom.sdmbn.Parameters.Guarantee;
import edu.wisc.cs.wisdom.sdmbn.Parameters.Optimization;
import edu.wisc.cs.wisdom.sdmbn.Parameters.Scope;
import edu.wisc.cs.wisdom.sdmbn.core.ReprocessEvent;
import edu.wisc.cs.wisdom.sdmbn.core.StateChunk;
import edu.wisc.cs.wisdom.sdmbn.json.DisableEventsMessage;
import edu.wisc.cs.wisdom.sdmbn.json.EnableEventsMessage;
import edu.wisc.cs.wisdom.sdmbn.json.EventsAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetMultiflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetMultiflowMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetPerflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.GetPerflowMessage;
import edu.wisc.cs.wisdom.sdmbn.json.Message;
import edu.wisc.cs.wisdom.sdmbn.json.MessageException;
import edu.wisc.cs.wisdom.sdmbn.json.PutAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.PutMultiflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.PutPerflowAckMessage;
import edu.wisc.cs.wisdom.sdmbn.json.ReprocessMessage;
import edu.wisc.cs.wisdom.sdmbn.json.StateMessage;
import edu.wisc.cs.wisdom.sdmbn.json.StateMultiflowMessage;
import edu.wisc.cs.wisdom.sdmbn.json.StatePerflowMessage;
import edu.wisc.cs.wisdom.sdmbn.utils.PacketUtil;

public class MoveOperation extends Operation 
{
	// Arguments provided in constructor
	private Middlebox src;
	private Middlebox dst;
	private PerflowKey key;
	private Scope scope;
	private Guarantee guarantee;
	private Optimization optimization;
	private short inPort;
		
	// Operation state
    private boolean getPerflowAcked;
    private boolean getMultiflowAcked;
    private volatile int getPerflowCount;
    private volatile int getMultiflowCount;
    private volatile int putPerflowAcks;
    private volatile int putMultiflowAcks;
	private boolean bufferingEnabled;
	private boolean performedPhaseTwo;
	private byte[] lastPacket;
	private boolean immediateRelease;
	
	// State and event processing
	private ConcurrentLinkedQueue<ReprocessEvent> eventsList;
	private ConcurrentLinkedQueue<StateChunk> statesList;
    private ExecutorService threadPool;
	
	// Statistics
	private boolean isFirstStateRcvd;
	private long moveStart;
	private int srcEventCount;
	private volatile int packetCount;
	
	public MoveOperation(IOperationManager manager, Middlebox src, 
			Middlebox dst, PerflowKey key, Scope scope, Guarantee guarantee, 
			Optimization optimization, short inPort)
	{
		// Store arguments
		super(manager);
		this.src = src;
		this.dst = dst;
		this.key = key;
		this.scope = scope;
		this.guarantee = guarantee;
		this.optimization = optimization;
		this.inPort = inPort;
		
		// Initialize operation state variables
		this.getPerflowAcked = false;
        this.getMultiflowAcked = false;
        this.getPerflowCount = 0;
        this.getMultiflowCount = 0;
        this.putPerflowAcks = 0;
        this.putMultiflowAcks = 0;
		this.bufferingEnabled = false;
        this.performedPhaseTwo = false;
		this.lastPacket = null;
		this.immediateRelease = false;
		
		// Initialize state and event processing variables
		this.eventsList  = new ConcurrentLinkedQueue<ReprocessEvent>();
		this.statesList = new ConcurrentLinkedQueue<StateChunk>();
		this.threadPool = Executors.newCachedThreadPool();
		
		// Initialize statistics variables
        this.isFirstStateRcvd = false;
		this.moveStart = -1;
		this.srcEventCount = 0;
		this.packetCount = 0;
	}

		
	@Override
    public int execute()
    {
		if ((this.guarantee != Guarantee.NO_GUARANTEE) 
				&& (this.optimization == Optimization.NO_OPTIMIZATION 
						|| this.optimization == Optimization.PZ))
		{
			enableEventGeneration(this.src, Message.CONSTANT_ACTION_DROP);
			return this.getId();
		}
		else
		{ return issueGet(); }
    }
	
	private int issueGet()
	{
		GetPerflowMessage getPerflow = null;
		GetMultiflowMessage getMultiflow = null;
		if (Scope.PERFLOW == this.scope || Scope.PF_MF == this.scope)
		{
			boolean raiseEvents = true;
			if (this.guarantee == Guarantee.NO_GUARANTEE 
					|| this.optimization == Optimization.NO_OPTIMIZATION 
					|| this.optimization == Optimization.PZ)
			{  raiseEvents = false; } 	
			getPerflow = new GetPerflowMessage(this.getId(), this.key, raiseEvents);
    		try
    		{ this.src.getStateChannel().sendMessage(getPerflow); }
    		catch(MessageException e)
    		{ return -1; }
		}
		if (Scope.MULTIFLOW == this.scope || Scope.PF_MF == this.scope)
		{
			getMultiflow = new GetMultiflowMessage(this.getId(), this.key);
    		try
    		{ this.src.getStateChannel().sendMessage(getMultiflow); }
    		catch(MessageException e)
    		{ return -1; }
		}
		
		if (null == getPerflow && null == getMultiflow)
		{
			log.error("Scope needs to be 'PERFLOW', 'MULTIFLOW' or 'PF_MF'");
			return -1; 
		}

        return this.getId();
    }

	@Override
	public void receiveStatePerflow(StatePerflowMessage msg) 
	{ this.receiveStateMessage(msg); }

    @Override
    public void receiveStateMultiflow(StateMultiflowMessage msg) 
    { this.receiveStateMessage(msg); }
    
    private void receiveStateMessage(StateMessage msg)
    {
    	if (!this.isFirstStateRcvd) 
    	{
    		this.moveStart = System.currentTimeMillis();
        	log.debug("Move Start ({})", this.getId());
        	this.isFirstStateRcvd = true;
    	}
    	
        // Add state chunk to list of state associated with this operation
        StateChunk chunk = obtainStateChunk(msg.hashkey);
    	chunk.storeStateMessage(msg, this.dst);
       
        if (optimization == Optimization.NO_OPTIMIZATION 
        		|| optimization == Optimization.LL)
        { this.statesList.add(chunk); }
        else
        {
        	this.threadPool.submit(chunk);
        	//chunk.call();
        }
    }

    @Override
    public void receivePutPerflowAck(PutPerflowAckMessage msg) 
    {
        this.putPerflowAcks++;
        this.receivePutAck(msg);
    }
    
    @Override
    public void receivePutMultiflowAck(PutMultiflowAckMessage msg) 
    {
    	this.putMultiflowAcks++;
        this.receivePutAck(msg);
    }
    
    private void receivePutAck(PutAckMessage msg)
    {
        StateChunk chunk = obtainStateChunk(msg.hashkey);
        
        // Indicate state chunk has been acked
        chunk.receivedPutAck();
        checkIfStateMoved();
    }

	@Override
	public void receiveReprocess(ReprocessMessage msg, Middlebox from) 
	{	
		//FIXME for order preserving move with early_release optimization, we need to maintain a global
		//linked map of event, state_chunk. When we receive put ack for a state_chunk, we should release events
		//from the head upto the point where we encounter an event which does not belong to this state chunk.
		//Currently, we release all events pertaining to the acked state_chunk irrespective of whether there are
		//other events in the queue before these events
		if (this.guarantee == Guarantee.NO_GUARANTEE)
		{
			log.warn("Event received for Move operation without any guarantees");
	        assert false; // Should not receive any events for NONE(no_guarantees) option
		}
		else if (from == this.src)
		{
			this.srcEventCount++;
			ReprocessEvent event = new ReprocessEvent(msg, this.dst);
			event.eventId = this.srcEventCount;
			log.debug(String.format("[CONT_ENTER] %d : entering controller", event.eventId));
			
			if (this.immediateRelease)
			{
				//irrespective of the optimization, this implies
				//the state transfer is complete
				//so go ahead and process the event directly
				//no need to check which particular flow it belongs to
				this.threadPool.submit(event);
				//event.process();
				return;
			}
			if (this.optimization != Optimization.PZ_LL_ER)
			{
				//No early release optimization
				//No need of mapping the events to state chunks
				//put them all in one queue
				this.eventsList.add(event);
			}
			else
			{
				//early release optimization has been requested for
				StateChunk stateChunk = obtainStateChunk(msg.hashkey);
				//the event is now associated with corresponding state chunk
				if (stateChunk.isAcked())
				{ this.threadPool.submit(event); }
				else
				{ stateChunk.receivedReprocess(event); }
			}
		}
		else
		{
			//This part is for order preserving move
			//we have received event from destination, check if this is for "last packet" and if yes, disable events
			if (null == this.lastPacket)
			{ return; }
			byte[] packet = ReprocessEvent.base64_decode(msg.packet);
			packet = PacketUtil.setFlag(packet, PacketUtil.FLAG_NONE);
			if (Arrays.equals(this.lastPacket, packet))
			{ 
				// Disable events at destination
				disableEventGeneration(this.dst);
			}
		}
	}
	
	@Override
    public void receiveGetPerflowAck(GetPerflowAckMessage msg) 
    {
		log.debug("Completed state per-flow state get");
        this.getPerflowAcked = true;
        this.getPerflowCount = msg.count;
        if (optimization == Optimization.NO_OPTIMIZATION || optimization == Optimization.LL)
        {
        	if (this.scope == Scope.PF_MF && this.getMultiflowAcked == false)
        	{ return; }        	
        	releaseBufferedStates();
        }
        else
        { checkIfStateMoved(); }
    }

    @Override
    public void receiveGetMultiflowAck(GetMultiflowAckMessage msg) 
    {
    	log.debug("Completed state multi-flow state get");
        this.getMultiflowAcked = true;
        this.getMultiflowCount = msg.count;
        if (optimization == Optimization.NO_OPTIMIZATION || optimization == Optimization.LL)
        {
        	if (this.scope == Scope.PF_MF && this.getPerflowAcked == false)
        	{ return; }
        	releaseBufferedStates();
        }
        else
        { checkIfStateMoved(); }
    }
    
    private void releaseBufferedStates()
    {
    	if (!this.statesList.isEmpty())
		{
			//if we are here, this operation does not use early release optimization
			// release the globally held events
			try 
			{ this.threadPool.invokeAll(this.statesList); }
			catch (InterruptedException e) 
			{
				log.error("Failed to release all events");
				this.fail();
				return;
			}
		}
    }
    
	private void checkIfStateMoved()
	{
		boolean completed = false;
		switch (this.scope)
		{
			case PERFLOW:
				completed = (this.getPerflowAcked 
						&& (this.putPerflowAcks == this.getPerflowCount));
				break;
			case MULTIFLOW:
				completed = (this.getMultiflowAcked 
						&& (this.putMultiflowAcks == this.getMultiflowCount));
				break;
			case PF_MF:
				completed = (this.getPerflowAcked && this.getMultiflowAcked 
						&& (this.putPerflowAcks == this.getPerflowCount) 
						&& (this.putMultiflowAcks == this.getMultiflowCount));
				break;
		}
		
		if (completed)
		{
			log.debug("Completed all state transfer");
			if (this.guarantee == Guarantee.ORDER_PRESERVING)
			{ enableEventGeneration(this.dst, Message.CONSTANT_ACTION_BUFFER); }
			else
			{ this.updateForwardingToDst(); }
			
			/*while (!this.eventsList.isEmpty())
			{ 
				//if we are here, this operation does not use early release optimization
				//release the globally held events
				this.eventsList.poll().process();
			}*/
			
			if (!this.eventsList.isEmpty())
			{
				//if we are here, this operation does not use early release optimization
				// release the globally held events
				if (this.guarantee == Guarantee.ORDER_PRESERVING)
				{
					
					while(!this.eventsList.isEmpty())
					{
						ReprocessEvent event = this.eventsList.remove();
						Future<Boolean> future = this.threadPool.submit(event);
						try 
						{ future.get(); }
						catch (Exception e) 
						{
							log.error("Failed to release event");
							this.fail();
							return;
						}
					}
				}
				else
				{
					try 
					{ this.threadPool.invokeAll(this.eventsList); }
					catch (InterruptedException e) 
					{
						log.error("Failed to release all events");
						this.fail();
						return;
					}
				}
			}
			
	        if (this.guarantee != Guarantee.ORDER_PRESERVING)
	        {
				log.debug("End Move ({})", this.getId());
	            this.finish();
	        }
			this.immediateRelease = true;
		}
	}
	
	private void enableEventGeneration(Middlebox mb, String action)
	{
		EnableEventsMessage enableEvents = new EnableEventsMessage();
		enableEvents.id = this.getId();
		enableEvents.key = this.key;
		enableEvents.action = action;
		try
		{ mb.getStateChannel().sendMessage(enableEvents); }
		catch(MessageException e)
		{ 
			log.error(String.format("[Move %d] Failed to request enable events on %s", this.getId(), mb.getId())); 
			this.fail();
		}
		log.debug("Requested enable events on {}", mb.getId());
	}
	
	
	private void disableEventGeneration(Middlebox mb)
	{
		DisableEventsMessage disableEvents = new DisableEventsMessage();
		disableEvents.id = this.getId();
		disableEvents.key = this.key;
		try
		{ this.dst.getStateChannel().sendMessage(disableEvents); }
		catch(MessageException e)
		{ 
			log.error(String.format("[Move %d] Failed to request disable events on %s", this.getId(), mb.getId()));
			this.fail();
		}
		log.info("Requested disable events on {}", mb.getId());
	}

	@Override
	public void receiveEventsAck(EventsAckMessage msg, Middlebox from)
	{
		if (from == this.src)
		{ 
			issueGet();
			return;
		}
		// If buffering was not enabled, then this must be the ACK for the
		// request to enable it
		if (!this.bufferingEnabled)
		{
			this.bufferingEnabled = true;
			
			// Perform phase 1 of the route update
			short[] outPorts = new short[1];
			outPorts[0] = OFPort.OFPP_CONTROLLER.getValue();
			// FIXME: We cannot send to controller and port because HP switches suck
			outPorts[1] = this.src.getSwitchPort();
			this.manager.installForwardingRules(this.dst.getSwitch(), 
					this.inPort, outPorts, this.key);
			log.debug(String.format("[Move %d] Performed route update phase 1", this.getId()));
		}
		else
		{
			//This is the ack for disable events at destination
			// FIXME: Is it safe to assume we are done?
			log.info("End Move");
			log.info("#events = " + this.srcEventCount);
			log.info("#packets = " + this.packetCount);

			long moveEnd = System.currentTimeMillis();
			long moveTime = moveEnd - this.moveStart;
			log.info(String.format("[MOVE_TIME] elapse=%d, start=%d, end=%d",
					moveTime, this.moveStart, moveEnd));

			this.finish();
		}
	}
	
	@Override
	public void receivePacket(byte[] packet, IOFSwitch inSwitch, short inPort) 
	{
		// Ignore packets from unexpected switch ports
		if ((this.inPort != inPort) || (this.src.getSwitch() != inSwitch))
		{
			log.debug(String.format("[Move %d] Received packet from wrong switch port", this.getId()));
			return; 
		}
		
		this.packetCount++;
		//if(this.lastPacket == null) {
			this.lastPacket = PacketUtil.setFlag(packet, PacketUtil.FLAG_NONE);
		//}
		log.debug(String.format("[Move %d] Received packet that was sent to src", this.getId()));
		
		// FIXME: We cannot send to src & controller because HP switches
		// suck, so we must send to controller and then to src
		/*OFPacketOut send = new OFPacketOut();
		ArrayList<OFAction> actions = new ArrayList<OFAction>();
        OFAction outputTo = new OFActionOutput(this.src.getSwitchPort());
        actions.add(outputTo);
        send.setActions(actions);
        send.setActionsLength((short)OFActionOutput.MINIMUM_LENGTH);
        send.setLength((short)(OFPacketOut.MINIMUM_LENGTH + send.getActionsLength()));
        send.setBufferId(OFPacketOut.BUFFER_ID_NONE);
        send.setPacketData(packet);
        send.setLength((short)(send.getLength() + packet.length));
        try
        {
        	this.src.getSwitch().write(send, null);
        	this.src.getSwitch().flush();
        }
        catch (Exception e)
        {
        	log.error(String.format("[Move %d] Could not flush packet to src", this.getId()));
        	this.fail();
        	return;
        }*/
		
		// Perform phase 2 of the route update once we get one packet
		if (!this.performedPhaseTwo && this.packetCount >= 1)
		{
			this.updateForwardingToDst();
            this.performedPhaseTwo = true;
			log.debug(String.format("[Move %d] Performed route update phase 2", this.getId()));
		}
	}
	
	private void updateForwardingToDst()
	{
		this.manager.installForwardingRules(this.dst.getSwitch(), 
				this.inPort, this.dst.getSwitchPort(), this.key);
		log.debug(String.format("[Move %d] Performed route update to send to dst", this.getId()));
	}
}
