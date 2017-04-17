package edu.wisc.cs.wisdom.sdmbn.apps.testing;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import net.floodlightcontroller.core.module.FloodlightModuleContext;
import net.floodlightcontroller.core.module.FloodlightModuleException;
import net.floodlightcontroller.core.module.IFloodlightModule;
import net.floodlightcontroller.core.module.IFloodlightService;
import net.floodlightcontroller.packet.Ethernet;
import net.floodlightcontroller.packet.IPv4;

import org.openflow.protocol.OFPort;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import edu.wisc.cs.wisdom.sdmbn.ISdmbnListener;
import edu.wisc.cs.wisdom.sdmbn.ISdmbnService;
import edu.wisc.cs.wisdom.sdmbn.Middlebox;
import edu.wisc.cs.wisdom.sdmbn.Parameters.Scope;
import edu.wisc.cs.wisdom.sdmbn.PerflowKey;
import edu.wisc.cs.wisdom.sdmbn.utils.FlowUtil;
import edu.wisc.cs.wisdom.sdmbn.utils.ISdmbnApp;
import edu.wisc.cs.wisdom.sdmbn.utils.NextStepTask;
import edu.wisc.cs.wisdom.sdmbn.utils.TraceLoad;

public abstract class TestTimed 
	implements IFloodlightModule, ISdmbnListener, ISdmbnApp
{			
	// Service providers
	protected ISdmbnService sdmbnProvider;
	
	// Active middleboxes
	protected Map<String, Middlebox> middleboxes;
	protected List<Middlebox> activeMbs;
	
	private final ScheduledExecutorService scheduler = Executors.newScheduledThreadPool(1);
	
	private TraceLoad traceLoad;
	protected short traceSwitchPort;
	private String traceHost;
	private short tracePort;
	private String traceFile;
	private int traceRate;
	private int traceNumPkts;
	private int operationDelay;
	private int stopDelay;
	protected Scope scope;
	protected int numInstances;

	protected boolean stopped = false;	

	protected static Logger log = LoggerFactory.getLogger(TestTimed.class.getSimpleName());
				
	@Override
	public Collection<Class<? extends IFloodlightService>> getModuleServices() 
	{ return null; }

	@Override
	public Map<Class<? extends IFloodlightService>, IFloodlightService> getServiceImpls() 
	{ return null; }

	@Override
	public Collection<Class<? extends IFloodlightService>> getModuleDependencies() 
	{
		Collection<Class<? extends IFloodlightService >> dependencies = 
			new ArrayList<Class<? extends IFloodlightService>>();
		dependencies.add(ISdmbnService.class);
		return dependencies;
	}
	
	protected void checkForArgument(Map<String,String> config, String argument)
			throws FloodlightModuleException 
	{
		if (!config.containsKey(argument))
		{
			throw new FloodlightModuleException(String.format("Must specify argument '%s.%s'", 
					this.getClass().getName(), argument));
		}
	}
	
	protected void parseArguments(FloodlightModuleContext context) 
			throws FloodlightModuleException 
	{
		// Get general arguments
		Map<String,String> config = context.getConfigParams(this);
		
		this.checkForArgument(config, "TraceReplaySwitchPort");
		this.traceSwitchPort = Short.parseShort(config.get("TraceReplaySwitchPort"));
		log.debug(String.format("TraceReplaySwitchPort = %d", this.traceSwitchPort));
		
		this.checkForArgument(config, "TraceReplayHost");
		this.traceHost = config.get("TraceReplayHost");
		log.debug(String.format("TraceReplayHost = %s", this.traceHost));

        if (config.containsKey("TraceReplayPort"))
        {
            this.tracePort = Short.parseShort(config.get("TraceReplayPort"));
            log.debug(String.format("TraceReplayPort = %d", this.tracePort));
        }
        else
        { this.tracePort = (short)0; }
		
		this.checkForArgument(config, "TraceReplayFile");
		this.traceFile = config.get("TraceReplayFile");
		log.debug(String.format("TraceReplayFile = %s", this.traceFile));
		
		this.checkForArgument(config, "TraceReplayRate");
		this.traceRate = Integer.parseInt(config.get("TraceReplayRate"));
		log.debug(String.format("TraceReplayRate = %d", this.traceRate));
		
		this.checkForArgument(config, "TraceReplayNumPkts");
		this.traceNumPkts = Integer.parseInt(config.get("TraceReplayNumPkts"));
		log.debug(String.format("TraceReplayNumPkts = %d", this.traceNumPkts));
		
		this.checkForArgument(config, "OperationDelay");
		this.operationDelay = Integer.parseInt(config.get("OperationDelay"));		
		log.debug(String.format("OperationDelay = %d", this.operationDelay));
		
		this.checkForArgument(config, "StopDelay");
		this.stopDelay = Integer.parseInt(config.get("StopDelay"));		
		log.debug(String.format("StopDelay = %d", this.stopDelay));
		
		this.checkForArgument(config, "Scope");
		this.scope = Scope.valueOf(config.get("Scope"));
		log.debug(String.format("Scope = %s", this.scope.name()));
	}


	@Override
	public void init(FloodlightModuleContext context)
			throws FloodlightModuleException 
	{
		this.numInstances = 2;
		this.parseArguments(context);

		this.sdmbnProvider = context.getServiceImpl(ISdmbnService.class);
		
		this.middleboxes = new HashMap<String, Middlebox>();
		this.activeMbs = new ArrayList<Middlebox>();
		
        if (this.tracePort > 0)
        { this.traceLoad = new TraceLoad(this.traceHost, this.tracePort, this.traceRate, this.traceNumPkts); }
        else
        { this.traceLoad = new TraceLoad(this.traceHost, this.traceRate, this.traceNumPkts); }
	}
	
	@Override
	public void startUp(FloodlightModuleContext context) 
	{ sdmbnProvider.addSdmbnListener(this); }

	@Override
	public void operationFinished(int id) 
	{ 
		log.info("Operation ({}) finished", id);
		this.terminateOperation();
	}

	@Override
	public void operationFailed(int id) 
	{ log.error("Operation ({}) failed", id); }
	
	private synchronized void addMiddlebox(Middlebox mb)
	{
		if (this.middleboxes.containsValue(mb))
		{ return; }
		
		for (int i = 1; i <= numInstances; i++)
		{
			String mbID = "mb".concat(Integer.toString(i));
			if (!this.middleboxes.containsKey(mbID))
			{ 
				this.middleboxes.put(mbID, mb);
				break;
			}
		}
		
		if (numInstances == this.middleboxes.size())
		{ this.executeStep(0); }
	}

	@Override
	public void middleboxConnected(Middlebox mb) 
	{
		if (!this.middleboxes.containsValue(mb))
		{ this.addMiddlebox(mb); }
	}
	
	@Override
	public void middleboxDisconnected(Middlebox mb) 
	{
		if (this.middleboxes.containsValue(mb))
		{
			for (Entry<String,Middlebox> entry : this.middleboxes.entrySet())
			{
				if (entry.getValue() == mb)
				{
					this.middleboxes.remove(entry.getKey());
					return;
				}
			}
		}
	}

	@Override
	public void middleboxLocationUpdated(Middlebox mb) 
	{
		if (!middleboxes.containsValue(mb))
		{ this.addMiddlebox(mb);	}
	}
	
	public void changeForwarding(PerflowKey key, List<Middlebox> mbs)
	{
		Middlebox mb = mbs.get(0);
		key.setDlType(Ethernet.TYPE_IPv4);
		key.setNwProto(IPv4.PROTOCOL_TCP);
		FlowUtil.installForwardingRules(mb.getSwitch(), this.traceSwitchPort, 
				mb.getSwitchPort(), key);
	}
	
	@Override
	public void executeStep(int step)
	{
		int startNextAfter = 0;
		switch(step)
		{
		case 0:
			startNextAfter = 5;
			break;
		case 1:
		{
			this.initialRuleSetup();
			log.info("Initial rules installed");
			boolean started = this.traceLoad.startTrace(this.traceFile);
			if (started)
			{ log.info("Started replaying trace"); }
			else 
			{ log.error("Failed to start replaying trace"); }
			startNextAfter = this.operationDelay;
			break;
		}
		case 2:
			this.initiateOperation();
			startNextAfter = this.stopDelay; 
			break;
		case 3:
		{
			stopped = this.traceLoad.stopTrace(this.traceFile);
			if (stopped)
			{ log.info("Stopped replaying trace"); }
			else 
			{ log.error("Failed to stop replaying trace"); }
			startNextAfter = 0;
			break;
		}
		default:
			return;
		}
		this.scheduler.schedule(new NextStepTask(step+1, this), startNextAfter, 
				TimeUnit.SECONDS);
	}
	
	protected abstract void initialRuleSetup();
	
	protected abstract void initiateOperation();

	protected abstract void terminateOperation();
}
