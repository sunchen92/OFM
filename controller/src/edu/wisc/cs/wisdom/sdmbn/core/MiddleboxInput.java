package edu.wisc.cs.wisdom.sdmbn.core;

import org.openflow.protocol.OFMessage;
import org.openflow.protocol.OFPacketIn;
import org.openflow.protocol.OFType;

import net.floodlightcontroller.core.FloodlightContext;
import net.floodlightcontroller.core.IFloodlightProviderService;
import net.floodlightcontroller.core.IOFMessageListener;
import net.floodlightcontroller.core.IOFSwitch;
import net.floodlightcontroller.packet.Ethernet;

public class MiddleboxInput implements IOFMessageListener
{
	private SdmbnManager sdmbnManager;
	
	public MiddleboxInput(SdmbnManager sdmbnManager)
	{ this.sdmbnManager = sdmbnManager; }
	
	@Override
	public net.floodlightcontroller.core.IListener.Command receive(
			IOFSwitch sw, OFMessage msg, FloodlightContext cntx) 
	{
		// Only care about packet-in messages
		if (msg.getType() != OFType.PACKET_IN) 
		{ 
			// Allow the next module to also process this OpenFlow message
		    return Command.CONTINUE;
		}
		OFPacketIn pi = (OFPacketIn)msg;

		// FIXME: Only filter out some packets
		// Only care about IPv4 packets
		Ethernet eth = IFloodlightProviderService.bcStore.
            get(cntx,IFloodlightProviderService.CONTEXT_PI_PAYLOAD);
		if (eth.getEtherType() != Ethernet.TYPE_IPv4)
		{
			// Allow the next module to also process this OpenFlow message
		    return Command.CONTINUE;
		}
		
		// Hand-off packet to operations
		this.sdmbnManager.packetReceived(pi.getPacketData(), sw, pi.getInPort());
		
		// Allow the next module to also process this OpenFlow message
		return Command.CONTINUE;
	}

	@Override
	public String getName() 
	{ return this.getClass().getName(); }

	@Override
	public boolean isCallbackOrderingPrereq(OFType type, String name) 
	{
		if ((type == OFType.PACKET_IN) 
				&& (name.equals(MiddleboxDiscovery.class.getName())))
		{ return true; }
		return false; 
	}

	@Override
	public boolean isCallbackOrderingPostreq(OFType type, String name) 
	{ return false; }
}
