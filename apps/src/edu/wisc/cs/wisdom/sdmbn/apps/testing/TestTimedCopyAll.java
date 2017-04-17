package edu.wisc.cs.wisdom.sdmbn.apps.testing;

import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import net.floodlightcontroller.core.module.FloodlightModuleContext;
import net.floodlightcontroller.core.module.FloodlightModuleException;
import net.floodlightcontroller.packet.Ethernet;
import net.floodlightcontroller.packet.IPv4;

import edu.wisc.cs.wisdom.sdmbn.Middlebox;
import edu.wisc.cs.wisdom.sdmbn.PerflowKey;

public class TestTimedCopyAll extends TestTimed
{
	private String consistency;
	private int interval;
	private int opCount = 0;
	private Middlebox mb1;
	private Middlebox mb2;
	private PerflowKey key;
	private int copyOpId;
	private ScheduledExecutorService worker;
	
	@Override
	protected void parseArguments(FloodlightModuleContext context)
			throws FloodlightModuleException 
	{
		super.parseArguments(context);
		
		// Get arguments specific to move
		Map<String,String> config = context.getConfigParams(this);
		
		this.checkForArgument(config, "Consistency");
		this.consistency = config.get("Consistency");
		log.debug(String.format("Consistency = %s", this.consistency));
		
		if (this.consistency.equals("EVENTUAL"))
		{
			this.checkForArgument(config, "Interval");
			this.interval = Integer.parseInt(config.get("Interval"));
			log.debug(String.format("Interval = %s", this.interval));
		}
	}
	
	@Override
	protected void initialRuleSetup()
	{
		Middlebox mb1 = middleboxes.get("mb1");
		synchronized(this.activeMbs)
		{ this.activeMbs.add(mb1); }
	
		List<Middlebox> mbs = Arrays.asList(new Middlebox[]{mb1});
		this.changeForwarding(new PerflowKey(), mbs);
	}
	
	@Override
	protected void initiateOperation()
	{
		this.mb1 = middleboxes.get("mb1");
		this.mb2 = middleboxes.get("mb2");
		synchronized(this.activeMbs)
		{ this.activeMbs.add(this.mb2); }
				
		this.key = new PerflowKey();
		this.key.setDlType(Ethernet.TYPE_IPv4);
		this.key.setNwProto(IPv4.PROTOCOL_TCP);
		log.info("Key="+this.key);
		
		if (this.consistency.equals("EVENTUAL"))
		{ initiateCopyLoop(); }
		else
		{ initiateCopy(); }
	}

	@Override
	protected void terminateOperation()
	{
		if (this.consistency.equals("EVENTUAL"))
		{
			if (this.opCount%2 == 1)
			{ this.initiateReverseCopy(); }
		}
		else
		{ return; }
	}

	private void initiateCopyLoop()
	{
		worker = 
			  Executors.newSingleThreadScheduledExecutor();
		Runnable task = new Runnable() {
			@Override
			public void run()
			{ initiateCopy(); }
		};
		worker.scheduleAtFixedRate(task, 0, this.interval, TimeUnit.SECONDS);
	}

	private void initiateCopy()
	{
		if (this.stopped)
		{
			worker.shutdown();
			return;
		}
		this.copyOpId = sdmbnProvider.copy(this.mb1, this.mb2, this.key, this.scope);
		this.opCount++;
		if (this.copyOpId >= 0)
		{ log.info("Initiated copy"); }
		else
		{ log.error("Failed to initiate copy"); }
	}
	
	private void initiateReverseCopy()
	{
		this.copyOpId = sdmbnProvider.copy(this.mb2, this.mb1, this.key, this.scope);
		this.opCount++;
		if (this.copyOpId >= 0)
		{ log.info("Initiated Reverse copy"); }
		else
		{ log.error("Failed to initiate Reverse copy"); }
	}
}
