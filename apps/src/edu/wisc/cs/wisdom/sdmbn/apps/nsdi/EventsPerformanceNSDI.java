package edu.wisc.cs.wisdom.sdmbn.apps.nsdi;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import net.floodlightcontroller.core.IFloodlightProviderService;
import net.floodlightcontroller.core.module.FloodlightModuleContext;
import net.floodlightcontroller.core.module.FloodlightModuleException;
import net.floodlightcontroller.core.module.IFloodlightModule;
import net.floodlightcontroller.core.module.IFloodlightService;
import net.floodlightcontroller.packet.Ethernet;
import net.floodlightcontroller.packet.IPv4;
import net.floodlightcontroller.staticflowentry.IStaticFlowEntryPusherService;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import edu.wisc.cs.wisdom.sdmbn.ISdmbnListener;
import edu.wisc.cs.wisdom.sdmbn.ISdmbnService;
import edu.wisc.cs.wisdom.sdmbn.Middlebox;
import edu.wisc.cs.wisdom.sdmbn.PerflowKey;
import edu.wisc.cs.wisdom.sdmbn.utils.ChangeForwardingTask;
import edu.wisc.cs.wisdom.sdmbn.utils.FlowUtil;
import edu.wisc.cs.wisdom.sdmbn.utils.ISdmbnApp;
import edu.wisc.cs.wisdom.sdmbn.utils.IOperationFinishedTask;
import edu.wisc.cs.wisdom.sdmbn.utils.NextStepTask;
import edu.wisc.cs.wisdom.sdmbn.utils.TraceLoad;
import edu.wisc.cs.wisdom.sdmbn.apps.TestbedConstants;

public class EventsPerformanceNSDI 
	implements IFloodlightModule, ISdmbnListener
{
	private static final short SENDER_PORT = TestbedConstants.PORT_ARTAUD;
	private static final String SENDER_IP = "10.0.1.89";
	
	private static final int TRACE_RATE = 500;
		
	// Service providers
	private IFloodlightProviderService floodlightProvider;
	private IStaticFlowEntryPusherService staticFlowEntryPusher;
	private ISdmbnService sdmbnProvider;
	
	// Active middleboxes
	private Map<String, Middlebox> middleboxes;
		
	// Tasks to run when operations finish
	private Map<Integer, IOperationFinishedTask> operationFinishTasks;
	
	private TraceLoad traceLoad;
	
	protected static Logger log = LoggerFactory.getLogger(EventsPerformanceNSDI.class);
				
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
		dependencies.add(IFloodlightProviderService.class);
		dependencies.add(IStaticFlowEntryPusherService.class);
		dependencies.add(ISdmbnService.class);
		return dependencies;
	}

	@Override
	public void init(FloodlightModuleContext context)
			throws FloodlightModuleException 
	{
		this.floodlightProvider = context.getServiceImpl(IFloodlightProviderService.class);
		this.staticFlowEntryPusher = context.getServiceImpl(IStaticFlowEntryPusherService.class);	
		this.sdmbnProvider = context.getServiceImpl(ISdmbnService.class);
		
		this.middleboxes = new HashMap<String, Middlebox>();
		this.operationFinishTasks = new HashMap<Integer,IOperationFinishedTask>();
		
		//-1 is the numpkts argument; it signifies all pkts need to be replayed
		this.traceLoad = new TraceLoad(SENDER_IP, TRACE_RATE, -1);
	}

	@Override
	public void startUp(FloodlightModuleContext context) 
	{
		sdmbnProvider.addSdmbnListener(this);
	}

	@Override
	public void operationFinished(int id) 
	{
		log.info("Operation ({}) finished", id);
		synchronized(this.operationFinishTasks)
		{
			if (this.operationFinishTasks.containsKey(id))
			{
				this.operationFinishTasks.get(id).run(id);
				this.operationFinishTasks.remove(id);
			}
		}
	}
	
	@Override
	public void operationFailed(int id) 
	{ log.error("Operation ({}) failed", id); }
	
	private void addMiddlebox(Middlebox mb)
	{
		if (middleboxes.values().contains(mb))
		{ return; }
		
		if (mb.getId().contains("beckett"))
		{ middleboxes.put("ids1", mb); }
		else if (mb.getId().contains("cocteau"))
		{ middleboxes.put("ids2", mb); }
		
		if (2 == middleboxes.size())
		{
			this.initialRuleSetup();
			this.traceLoad.startTrace(TestbedConstants.TRACE_WEB);
			this.moveAll();
		}
	}

	@Override
	public void middleboxConnected(Middlebox mb) 
	{
		if (!middleboxes.keySet().contains(mb))
		{ addMiddlebox(mb);	}
	}
	
	@Override
	public void middleboxDisconnected(Middlebox mb) 
	{ }

	@Override
	public void middleboxLocationUpdated(Middlebox mb) 
	{
		if (!middleboxes.keySet().contains(mb))
		{ addMiddlebox(mb);	}
	}
	
	public void changeForwarding(PerflowKey key, List<Middlebox> mbs)
	{
		Middlebox ids = mbs.get(0);
		key.setDlType(Ethernet.TYPE_IPv4);
		key.setNwProto(IPv4.PROTOCOL_TCP);
		FlowUtil.installForwardingRule("toIds", ids.getSwitch(), SENDER_PORT, 
				ids.getSwitchPort(), key, (short)1000, staticFlowEntryPusher);
	}
		
	private void initialRuleSetup()
	{
		log.info("Start initial rule setup");
		Middlebox ids1 = middleboxes.get("ids1");
		
		this.changeForwarding(new PerflowKey(), 
				Arrays.asList(new Middlebox[]{ids1}));
	}
	
	private void moveAll()
	{
		log.info("Start move all");
		Middlebox ids1 = middleboxes.get("ids1");
		Middlebox ids2 = middleboxes.get("ids2");
					
		PerflowKey key = new PerflowKey();
		key.setDlType(Ethernet.TYPE_IPv4);
		key.setNwProto(IPv4.PROTOCOL_TCP);
		//sdmbnProvider.moveState(ids1, ids2, key);
		log.info("Initiated move");
	}
}

