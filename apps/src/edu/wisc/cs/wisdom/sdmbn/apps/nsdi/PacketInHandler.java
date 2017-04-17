package edu.wisc.cs.wisdom.sdmbn.apps.nsdi;

import java.util.ArrayList;

import org.openflow.protocol.OFFlowMod;
import org.openflow.protocol.OFMatch;
import org.openflow.protocol.OFMessage;
import org.openflow.protocol.OFPacketIn;
import org.openflow.protocol.OFPacketOut;
import org.openflow.protocol.OFType;
import org.openflow.protocol.action.OFAction;
import org.openflow.protocol.action.OFActionOutput;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import edu.wisc.cs.wisdom.sdmbn.Middlebox;

import net.floodlightcontroller.core.FloodlightContext;
import net.floodlightcontroller.core.IOFMessageListener;
import net.floodlightcontroller.core.IOFSwitch;
import net.floodlightcontroller.packet.Ethernet;
import net.floodlightcontroller.packet.IPv4;

public class PacketInHandler implements IOFMessageListener 
{
	private static final short TIMEOUT_PERMANENT = 0;
	private static final short TIMEOUT_IDLE_FLOW = 20;
	
	protected static Logger log = LoggerFactory.getLogger(PacketInHandler.class);
	
	private FullScenarioNSDI app;
	
	public PacketInHandler(FullScenarioNSDI app)
	{ this.app = app; }
	
	/**
	 * Get a name for the listener.
	 * @return a name for the listener.
	 */
	@Override
	public String getName() 
	{ return this.getClass().getName(); }

	/**
	 * Specify whether another listener must be called beforehand.
	 * @return false (ordering for this listener does not matter)
	 */
	@Override
	public boolean isCallbackOrderingPrereq(OFType type, String name) 
	{ return false; }

	/**
	 * Specify whether another listener must be called afterwards.
	 * @return false (ordering for this listener does not matter)
	 */
	@Override
	public boolean isCallbackOrderingPostreq(OFType type, String name) 
	{ return false; }

	/**
	 * Process a packet-in message.
	 * @param sw the switch that generated the message
	 * @param msg the openflow message
	 * @param cntx a context object for passing meta-data between listeners
	 * @return Command.CONTINUE (another listener can also process the message)
	 */
	@Override
	public net.floodlightcontroller.core.IListener.Command receive(IOFSwitch sw, OFMessage msg, FloodlightContext cntx) 
	{
		// Ignore messages which are not packet-in messages
		if (!(msg instanceof OFPacketIn))
		{ return Command.CONTINUE; }
		OFPacketIn packetIn = (OFPacketIn)msg;
		
		// Ignore packets which are not IP		
        OFMatch match = new OFMatch();
        match.loadFromPacket(packetIn.getPacketData(), packetIn.getInPort());
        if (match.getDataLayerType() != Ethernet.TYPE_IPv4)
        { return Command.CONTINUE; }

       	// Get ids to which this flow should be sent
        Middlebox ids = this.app.getNextActiveIds();
        
        // Setup match criteria
        OFMatch forwardMatch = new OFMatch();
        forwardMatch.loadFromPacket(packetIn.getPacketData(), packetIn.getInPort());
        OFMatch reverseMatch = forwardMatch.clone();
        reverseMatch.setDataLayerDestination(forwardMatch.getDataLayerSource());
        reverseMatch.setDataLayerSource(forwardMatch.getDataLayerDestination());
        reverseMatch.setNetworkDestination(forwardMatch.getNetworkSource());
        reverseMatch.setNetworkSource(forwardMatch.getNetworkDestination());
        if (forwardMatch.getNetworkProtocol() == IPv4.PROTOCOL_TCP || forwardMatch.getNetworkProtocol() == IPv4.PROTOCOL_UDP)
		{
			reverseMatch.setTransportDestination(forwardMatch.getTransportSource());
			reverseMatch.setTransportSource(forwardMatch.getTransportDestination());
		}
        
        // Prepare flow mod message to install reverse rule
		OFFlowMod flowMod = new OFFlowMod();
		flowMod.setType(OFType.FLOW_MOD);
		flowMod.setCommand(OFFlowMod.OFPFC_ADD);
		flowMod.setMatch(reverseMatch);
		flowMod.setLength((short)(OFFlowMod.MINIMUM_LENGTH + OFActionOutput.MINIMUM_LENGTH));
		flowMod.setBufferId(OFPacketOut.BUFFER_ID_NONE);
		flowMod.setHardTimeout(TIMEOUT_PERMANENT);
		flowMod.setIdleTimeout(TIMEOUT_IDLE_FLOW);
		ArrayList<OFAction> actions = new ArrayList<OFAction>();
        actions.add(new OFActionOutput(ids.getSwitchPort()));
        flowMod.setActions(actions);
        
        // Install reverse rule  
        try 
        { sw.write(flowMod, null); }
        catch (Exception e)
        { log.error(String.format("Could not install reverse rule %s in switch %s", flowMod, sw.getStringId())); }
        
        // Prepare flow mod message to install forward rule
        flowMod.setMatch(forwardMatch);
        
        // Install forward rule  
        try 
        { 
        	sw.write(flowMod, null);
        	sw.flush();
        }
        catch (Exception e)
        { log.error(String.format("Could not install forward rule %s in switch %s", flowMod, sw.getStringId())); }
        
        // Prepare packet out message to install send packet
        OFPacketOut send = new OFPacketOut();
		send.setType(OFType.PACKET_OUT);
        send.setActions(actions);
        send.setActionsLength((short)(OFActionOutput.MINIMUM_LENGTH));
		send.setLength((short)(OFPacketOut.MINIMUM_LENGTH + send.getActionsLength()));
		send.setPacketData(packetIn.getPacketData());
    	send.setLength((short)(send.getLength() + packetIn.getPacketData().length));
            
    	// Send packet
        try 
        {
        	sw.write(send, null);
        	sw.flush();
        }
        catch (Exception e)
        { log.error(String.format("Could not send packet out %s on switch %s", send, sw.getStringId())); }
        
        return Command.CONTINUE;
	}

}
