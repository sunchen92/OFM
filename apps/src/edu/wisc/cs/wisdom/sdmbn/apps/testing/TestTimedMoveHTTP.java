package edu.wisc.cs.wisdom.sdmbn.apps.testing;

import java.util.Arrays;
import java.util.List;
import java.util.Map;

import net.floodlightcontroller.core.module.FloodlightModuleContext;
import net.floodlightcontroller.core.module.FloodlightModuleException;
import net.floodlightcontroller.packet.Ethernet;
import net.floodlightcontroller.packet.IPv4;

import edu.wisc.cs.wisdom.sdmbn.Middlebox;
import edu.wisc.cs.wisdom.sdmbn.Parameters.Guarantee;
import edu.wisc.cs.wisdom.sdmbn.Parameters.Optimization;
import edu.wisc.cs.wisdom.sdmbn.PerflowKey;

public class TestTimedMoveHTTP extends TestTimed
{		
	private static final short TRANSPORT_PORTS[] = { 80, 443 };

	private Guarantee guarantee;
	private Optimization optimization;

	@Override
	protected void parseArguments(FloodlightModuleContext context)
			throws FloodlightModuleException 
	{
		super.parseArguments(context);
		
		// Get arguments specific to move
		Map<String,String> config = context.getConfigParams(this);
		
		this.checkForArgument(config, "Guarantee");
		this.guarantee = Guarantee.valueOf(config.get("Guarantee"));
		log.debug(String.format("Guarantee = %s", this.guarantee.name()));
		
		this.checkForArgument(config, "Optimization");
		this.optimization = Optimization.valueOf(config.get("Optimization"));
		log.debug(String.format("Optimization = %s", this.optimization.name()));
	}
	
	@Override
	protected void initialRuleSetup()
	{
		Middlebox mb1 = middleboxes.get("mb1");
		synchronized(this.activeMbs)
		{ this.activeMbs.add(mb1); }
	
		List<Middlebox> mbs = Arrays.asList(new Middlebox[]{mb1});
		for (short tp_src : TRANSPORT_PORTS)
		{
			PerflowKey key = new PerflowKey();
			key.setTpSrc(tp_src);
			key.setTpFlip(true);
			this.changeForwarding(key, mbs);
		}
	}
	
	@Override
	protected void initiateOperation()
	{
		Middlebox mb1 = middleboxes.get("mb1");
		Middlebox mb2 = middleboxes.get("mb2");
		synchronized(this.activeMbs)
		{ this.activeMbs.add(mb2); }
		
		short tp_src = TRANSPORT_PORTS[0];
		
		PerflowKey key = new PerflowKey();
		key.setDlType(Ethernet.TYPE_IPv4);
		key.setNwProto(IPv4.PROTOCOL_TCP);
		key.setTpSrc(tp_src);
		key.setTpFlip(true);
		log.info("Key="+key);
	
		int moveOpId = sdmbnProvider.move(mb1, mb2, key, this.scope, 
				this.guarantee, this.optimization, this.traceSwitchPort);
		if (moveOpId >= 0)
		{ log.info("Initiated move"); }
		else
		{ log.error("Failed to initiate move"); }
	}
	
	@Override
	protected void terminateOperation()
	{ return; }
}
