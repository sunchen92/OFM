package edu.wisc.cs.wisdom.sdmbn.apps.testing;

import java.util.Arrays;
import java.util.List;
import java.util.Map;

import net.floodlightcontroller.core.module.FloodlightModuleContext;
import net.floodlightcontroller.core.module.FloodlightModuleException;
import net.floodlightcontroller.packet.Ethernet;
import net.floodlightcontroller.packet.IPv4;

import edu.wisc.cs.wisdom.sdmbn.Middlebox;
import edu.wisc.cs.wisdom.sdmbn.Parameters.Consistency;
import edu.wisc.cs.wisdom.sdmbn.PerflowKey;

public class TestTimedShareAll extends TestTimed
{
	private Consistency consistency;
	private PerflowKey key;
	private static final short TRANSPORT_PORTS[] = { 80, 443, 8065, 3136 };
	@Override
	protected void parseArguments(FloodlightModuleContext context)
			throws FloodlightModuleException 
	{
		super.parseArguments(context);
		
		// Get arguments specific to move
		Map<String,String> config = context.getConfigParams(this);
		
		this.checkForArgument(config, "Consistency");
		this.consistency = Consistency.valueOf(config.get("Consistency"));
		log.debug(String.format("Consistency = %s", this.consistency));
		
		this.checkForArgument(config, "numInstances");
		this.numInstances = Integer.parseInt(config.get("numInstances"));
		if (this.numInstances < 2)
		{ throw new FloodlightModuleException(String.format("'%s.numInstances' cannot be less than 2 for share operation", 
				this.getClass().getName())); }
		log.debug(String.format("numInstances = %d", this.numInstances));
	}
	
	@Override
	protected void initialRuleSetup()
	{
		Middlebox mb1 = middleboxes.get("mb1");
		synchronized(this.activeMbs)
		{ this.activeMbs.add(mb1); }
	
		List<Middlebox> mbs = Arrays.asList(new Middlebox[]{mb1});
		this.changeForwarding(new PerflowKey(), mbs);
		
		//check the number of instances we are sharing
		for (int i=2; i <= this.numInstances; i++)
		{
			String mbID = "mb".concat(Integer.toString(i));
			Middlebox curMb = middleboxes.get(mbID);
			mbs.set(0, curMb);
			this.activeMbs.add(curMb);
			PerflowKey key;
			try
			{
				 key = new PerflowKey();
				 key.setTpSrc(TRANSPORT_PORTS[i-2]);
				 //key.setTpFlip(true);
				 this.changeForwarding(key, mbs);
			}
			catch (ArrayIndexOutOfBoundsException e)
			{
				log.error("add more rules for flow distribution among NFs");
			}
		}
	}
	
	@Override
	protected void initiateOperation()
	{
		this.key = new PerflowKey();
		this.key.setDlType(Ethernet.TYPE_IPv4);
		this.key.setNwProto(IPv4.PROTOCOL_TCP);
		log.info("Key="+this.key);
		
		int shareOpId = sdmbnProvider.share(this.activeMbs, key, this.scope, 
				this.consistency);
		if (shareOpId >= 0)
		{ log.info("Initiated share with strong consistency"); }
		else
		{ log.error("Failed to initiate share with strong consistency"); }
	}

	@Override
	protected void terminateOperation()
	{ return; }
}
