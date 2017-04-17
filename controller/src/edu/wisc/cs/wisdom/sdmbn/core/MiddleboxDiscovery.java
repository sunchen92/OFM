package edu.wisc.cs.wisdom.sdmbn.core;

import java.util.Arrays;

import org.openflow.protocol.OFMessage;
import org.openflow.protocol.OFPacketIn;
import org.openflow.protocol.OFType;

import com.google.gson.Gson;

import edu.wisc.cs.wisdom.sdmbn.json.DiscoveryMessage;

import net.floodlightcontroller.core.FloodlightContext;
import net.floodlightcontroller.core.IFloodlightProviderService;
import net.floodlightcontroller.core.IOFMessageListener;
import net.floodlightcontroller.core.IOFSwitch;
import net.floodlightcontroller.packet.Ethernet;

public class MiddleboxDiscovery implements IOFMessageListener
{
	private static short DISCOVERY_ETHTYPE = (short) 0x88B5;
	private Gson gson;
	private SdmbnManager sdmbnManager;
	
	public MiddleboxDiscovery(SdmbnManager sdmbnManager)
	{
		this.sdmbnManager = sdmbnManager;
		gson = new Gson();
	}
	
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
		
		// Only care about middlebox discovery packets
		Ethernet eth = IFloodlightProviderService.bcStore.
            get(cntx,IFloodlightProviderService.CONTEXT_PI_PAYLOAD);
		if (eth.getEtherType() != DISCOVERY_ETHTYPE)
		{
			// Allow the next module to also process this OpenFlow message
		    return Command.CONTINUE;
		}
		
		byte[] payload = Arrays.copyOfRange(pi.getPacketData(), 14, 
				pi.getPacketData().length);
		
		String jsonstring = new String(payload);
		DiscoveryMessage discovery = gson.fromJson(jsonstring, 
				DiscoveryMessage.class);
	
		sdmbnManager.middleboxDiscovered(discovery, sw, pi.getInPort());
		
		return Command.STOP;
	}

	@Override
	public String getName() 
	{
		return this.getClass().getName();
	}

	@Override
	public boolean isCallbackOrderingPrereq(OFType type, String name) 
	{
		return false;
	}

	@Override
	public boolean isCallbackOrderingPostreq(OFType type, String name) 
	{
		return false;
	}
}
