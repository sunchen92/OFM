package edu.wisc.cs.wisdom.sdmbn.core;

import java.net.InetSocketAddress;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.Executors;

import org.jboss.netty.bootstrap.ServerBootstrap;
import org.jboss.netty.channel.ChannelFactory;
import org.jboss.netty.channel.ChannelPipeline;
import org.jboss.netty.channel.ChannelPipelineFactory;
import org.jboss.netty.channel.Channels;
import org.jboss.netty.channel.socket.nio.NioServerSocketChannelFactory;
import org.jboss.netty.handler.codec.frame.LengthFieldBasedFrameDecoder;
import org.jboss.netty.handler.codec.string.StringDecoder;
import org.jboss.netty.handler.codec.string.StringEncoder;
import org.openflow.protocol.OFType;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import edu.wisc.cs.wisdom.sdmbn.Parameters.Consistency;
import edu.wisc.cs.wisdom.sdmbn.Parameters.Guarantee;
import edu.wisc.cs.wisdom.sdmbn.Parameters.Optimization;
import edu.wisc.cs.wisdom.sdmbn.ISdmbnListener;
import edu.wisc.cs.wisdom.sdmbn.ISdmbnService;
import edu.wisc.cs.wisdom.sdmbn.Middlebox;
import edu.wisc.cs.wisdom.sdmbn.PerflowKey;
import edu.wisc.cs.wisdom.sdmbn.Parameters.Scope;
import edu.wisc.cs.wisdom.sdmbn.channel.BaseChannel;
import edu.wisc.cs.wisdom.sdmbn.channel.EventChannelHandler;
import edu.wisc.cs.wisdom.sdmbn.channel.StateChannelHandler;
import edu.wisc.cs.wisdom.sdmbn.channel.EventChannel;
import edu.wisc.cs.wisdom.sdmbn.channel.StateChannel;
import edu.wisc.cs.wisdom.sdmbn.json.DiscoveryMessage;
import edu.wisc.cs.wisdom.sdmbn.operation.CopyOperation;
import edu.wisc.cs.wisdom.sdmbn.operation.IOperationManager;
import edu.wisc.cs.wisdom.sdmbn.operation.MoveConfigOperation;
import edu.wisc.cs.wisdom.sdmbn.operation.MoveOperation;
import edu.wisc.cs.wisdom.sdmbn.operation.Operation;
import edu.wisc.cs.wisdom.sdmbn.operation.ShareStrictOperation;
import edu.wisc.cs.wisdom.sdmbn.operation.ShareStrongOperation;
import edu.wisc.cs.wisdom.sdmbn.utils.FlowUtil;
import net.floodlightcontroller.core.IFloodlightProviderService;
import net.floodlightcontroller.core.IOFSwitch;
import net.floodlightcontroller.core.module.FloodlightModuleContext;
import net.floodlightcontroller.core.module.IFloodlightModule;
import net.floodlightcontroller.core.module.IFloodlightService;

public class SdmbnManager implements IFloodlightModule, ISdmbnService, 
		IOperationManager
{
	private final static int MAX_LINE_LENGTH = 1024 * 512;
	
	// Providers
	private IFloodlightProviderService floodlightProvider;
	
	private ConcurrentHashMap<String, Middlebox> middleboxes;
	private ConcurrentHashMap<Integer, Operation> operations;
	private ConcurrentLinkedQueue<ISdmbnListener> listeners;
	private MiddleboxDiscovery discovery;
	private MiddleboxInput input;
	
	private short statePort;
	private ServerBootstrap stateServerBootstrap;
	private short eventPort;
	private ServerBootstrap eventServerBootstrap;
			
	protected static Logger log = LoggerFactory.getLogger("sdmbn.SdmbnManager");
		    
	public Collection<Class<? extends IFloodlightService>> getModuleServices() 
	{
		Collection<Class<? extends IFloodlightService>> services =
			new ArrayList<Class<? extends IFloodlightService>>();
		services.add(ISdmbnService.class);
		return services;
	}

	@Override
	public Map<Class<? extends IFloodlightService>, IFloodlightService> getServiceImpls() 
	{
		Map<Class<? extends IFloodlightService>, IFloodlightService> services = 
			new HashMap<Class<? extends IFloodlightService>, IFloodlightService>();
		services.put(ISdmbnService.class, this);
		return services;
	}

	@Override
	public Collection<Class<? extends IFloodlightService>> getModuleDependencies()
	{
		Collection<Class<? extends IFloodlightService>> dependencies =
			new ArrayList<Class<? extends IFloodlightService>>();
		dependencies.add(IFloodlightProviderService.class);
		return dependencies;
	}

	@Override
	public void init(FloodlightModuleContext context)
	{
		log.info("Initializing SDMBN");
		middleboxes = new ConcurrentHashMap<String, Middlebox>();
		
		Map<String,String> config = context.getConfigParams(this);
		
		statePort = Short.parseShort(config.get("stateport"));
        ChannelFactory stateChannelFactory = new NioServerSocketChannelFactory(
        		Executors.newCachedThreadPool(), Executors.newCachedThreadPool());
        final StateChannelHandler stateChannelHandler = new StateChannelHandler(this);
        stateServerBootstrap = new ServerBootstrap(stateChannelFactory);
        stateServerBootstrap.setPipelineFactory(new ChannelPipelineFactory(){
        	public ChannelPipeline getPipeline() { 
        		return Channels.pipeline(
        				new LengthFieldBasedFrameDecoder(MAX_LINE_LENGTH,0,4,0,4),
        				new StringDecoder(),
        				stateChannelHandler,
        				new StringEncoder());
        	}
        });    
        stateServerBootstrap.setOption("child.tcpNoDelay", true);
        stateServerBootstrap.setOption("child.keepAlive", true);
        
        eventPort = Short.parseShort(config.get("eventport"));
        ChannelFactory eventChannelFactory = new NioServerSocketChannelFactory(
        		Executors.newCachedThreadPool(), Executors.newCachedThreadPool());
        final EventChannelHandler eventChannelHandler = new EventChannelHandler(this);
        eventServerBootstrap = new ServerBootstrap(eventChannelFactory);
        eventServerBootstrap.setPipelineFactory(new ChannelPipelineFactory(){
        	public ChannelPipeline getPipeline() { 
        		return Channels.pipeline(
        				new LengthFieldBasedFrameDecoder(MAX_LINE_LENGTH,0,4,0,4),
        				new StringDecoder(),
        				eventChannelHandler,
        				new StringEncoder());
        	}
        });    
        eventServerBootstrap.setOption("child.tcpNoDelay", true);
        eventServerBootstrap.setOption("child.keepAlive", true);
		
		operations = new ConcurrentHashMap<Integer, Operation>();
		listeners = new ConcurrentLinkedQueue<ISdmbnListener>();
		discovery = new MiddleboxDiscovery(this);
		input = new MiddleboxInput(this);
		
		floodlightProvider = context.getServiceImpl(IFloodlightProviderService.class);
	}
	
	@Override
	public void startUp(FloodlightModuleContext context) 
	{
		// Listen for packet in messages
		floodlightProvider.addOFMessageListener(OFType.PACKET_IN, discovery);
		floodlightProvider.addOFMessageListener(OFType.PACKET_IN, input);
        
		// Start server channels to listen for channels for state and events
		stateServerBootstrap.bind(new InetSocketAddress(statePort));
		eventServerBootstrap.bind(new InetSocketAddress(eventPort));
	}
	
	private Middlebox obtainMiddlebox(String host, int pid)
	{
		Middlebox mb;
		synchronized(this.middleboxes) 
		{
			String id = Middlebox.constructId(host, pid);
			if (middleboxes.containsKey(id))
			{ mb = middleboxes.get(id); }
			else
			{
				mb = new Middlebox(host, pid);
				middleboxes.put(id, mb);
			}
		}
		return mb;
	}

	public void channelConnected(BaseChannel channel)
	{
		Middlebox mb = obtainMiddlebox(channel.getHost(), channel.getPid());
		
		boolean connected = false;
		synchronized(mb)
		{
			channel.setMiddlebox(mb);
			if (channel instanceof StateChannel)
			{ mb.setStateChannel((StateChannel)channel); }
			else if (channel instanceof EventChannel)
			{ mb.setEventChannel((EventChannel)channel); }
		
			// If both channels are now set, then we should notify of
			// middlebox connect
			if (mb.hasEventChannel() && mb.hasStateChannel())
			{ connected = true;}
		}
		
		if (connected)
		{
			Iterator<ISdmbnListener> iterator = listeners.iterator();
			while (iterator.hasNext())
			{
				ISdmbnListener listener = iterator.next();
				listener.middleboxConnected(mb);
			}
		}
	}
	
	public void channelDisconnected(BaseChannel channel)
	{
		Middlebox mb = obtainMiddlebox(channel.getHost(), channel.getPid());
		
		boolean disconnected = false;
		synchronized(mb)
		{
			if (channel instanceof StateChannel)
			{ mb.setStateChannel(null); }
			else if (channel instanceof EventChannel)
			{ mb.setEventChannel(null); }
		
			// If we unset one of the channels, then we should notify of
			// middlebox disconnect
			if ((mb.hasStateChannel() && !mb.hasEventChannel())
					|| (!mb.hasStateChannel() && mb.hasEventChannel()))
			{ disconnected = true; }
		}
		
		if (disconnected)
		{
			Iterator<ISdmbnListener> iterator = listeners.iterator();
			while (iterator.hasNext())
			{
				ISdmbnListener listener = iterator.next();
				try
				{ listener.middleboxDisconnected(mb); }
				catch(Exception e)
				{ e.printStackTrace(); }
			}
		}
	}
	
	public void middleboxDiscovered(DiscoveryMessage discovery,
			IOFSwitch sw, short switchPort)
	{
		Middlebox mb = obtainMiddlebox(discovery.host, discovery.pid);
		
		log.debug(String.format("Discovery from %s.%d at %s.%d",
				discovery.host, discovery.pid, sw.getStringId(), switchPort));
		
		boolean locationUpated = false;
		synchronized(mb)
		{
			// If the switch and/or port changed, then we should notify of
			// middlebox location update
			if (mb.getSwitch() != sw || mb.getSwitchPort() != switchPort)
			{ 
				mb.setSwitch(sw, switchPort);
				locationUpated = true; 
			}
		}			
		
		if (locationUpated)
		{
			Iterator<ISdmbnListener> iterator = listeners.iterator();
			while (iterator.hasNext())
			{
				ISdmbnListener listener = iterator.next();
				try
				{ listener.middleboxLocationUpdated(mb); }
				catch(Exception e)
				{ e.printStackTrace(); }
			}
		}
	}
		
	public Operation lookupOperation(int id)
	{ return operations.get(id); }
	
	public int move(Middlebox src, Middlebox dst, PerflowKey key, Scope scope,
			Guarantee guarantee, Optimization optimization, short inPort)
	{
		switch (scope)
		{
			case PERFLOW:
			case MULTIFLOW:
			case PF_MF:
				break;
			default:
				log.error("Move can only be performed for scopes: PERFLOW, MULTIFLOW, PF_MF");
				return -1;
		}
		switch (optimization)
		{
			case NO_OPTIMIZATION:
			case PZ:
					break;
			case LL:
			case PZ_LL:
				if (guarantee == Guarantee.NO_GUARANTEE)
				{
					log.error("Late Locking optimization cannot be applied to a move with NO_GUARANTEE since no events are generated");
					return -1;
				}
				break;
			case PZ_LL_ER:
				//early release optimization has been requested; check if it can be executed
				if (scope == Scope.PF_MF)
				{
					log.error("Early Release optimization cannot be executed when the move operation involves multiple scopes");
					return -1;
				}
				if (guarantee == Guarantee.NO_GUARANTEE)
				{
					log.error("Late Locking and Early Release optimizations cannot be applied to a move with NO_GUARANTEE since no events are generated");
					return -1;
				}
				break;
			default:
				log.error("Unrecognized Optimization code. Allowed are NO_OPTIMIZATION, PZ, LL, PZ_LL, PZ_LL_ER");
				return -1;
		}
		// Create operation
		MoveOperation op = new MoveOperation(this, src, dst, key, scope, guarantee, optimization, inPort);
		this.operations.put(op.getId(), op);
		
		// Execute operation and return id for operation
		return op.execute();
	}
	
	public int moveConfig(Middlebox src, Middlebox dst, PerflowKey key)
	{
		// Create operation
		MoveConfigOperation op = new MoveConfigOperation(this, src, dst, key);
		this.operations.put(op.getId(), op);
		
		// Execute operation and return id for operation
		return op.execute();	
	}
	
	public int copy(Middlebox src, Middlebox dst, PerflowKey key, Scope scope)
	{
		// Create operation
		CopyOperation op = new CopyOperation(this, src, dst, key, scope);
		log.info("Copy States ({})", op.getId());
		this.operations.put(op.getId(), op);
		
		// Execute operation and return id for operation
		return op.execute();	
	}
	
	public int share(List<Middlebox> instances, PerflowKey key, Scope scope, Consistency consistency)
	{ return this.share(instances, key, scope, consistency, (short)-1);	}
	
	public int share(List<Middlebox> instances, PerflowKey key, Scope scope, Consistency consistency, short inPort)
	{
		// Create operation
		if (consistency == Consistency.STRONG)
		{ 
			ShareStrongOperation op = new ShareStrongOperation(this, instances, key, scope);
			log.info("Share States ({})", op.getId());
			this.operations.put(op.getId(), op);
			// Execute operation and return id for operation
			return op.execute();
		}
		else if (consistency == Consistency.STRICT)
		{ 
			ShareStrictOperation op = new ShareStrictOperation(this, instances, key, scope, inPort);
			log.info("Share States ({})", op.getId());
			this.operations.put(op.getId(), op);
			// Execute operation and return id for operation
			return op.execute();
		}
		else
		{ 
			log.error("Consistency needs to be either 'STRONG' or 'STRICT'"); 
			return -1;
		}
	}
	
	@Override
	public void addSdmbnListener(ISdmbnListener listener) 
	{ this.listeners.add(listener); }

	@Override
	public void operationFinished(Operation op) 
	{
		Iterator<ISdmbnListener> iterator = listeners.iterator();
		while (iterator.hasNext())
		{
			ISdmbnListener listener = iterator.next();
			try
			{ listener.operationFinished(op.getId()); }
			catch(Exception e)
			{ e.printStackTrace(); }
		}
	}

	@Override
	public void operationFailed(Operation op) 
	{
		Iterator<ISdmbnListener> iterator = listeners.iterator();
		while (iterator.hasNext())
		{
			ISdmbnListener listener = iterator.next();
			try
			{ listener.operationFailed(op.getId()); }
			catch(Exception e)
			{ e.printStackTrace(); }
		}
	}
	
	@Override
	public void installForwardingRules(IOFSwitch sw, short inPort, 
			short outPort, PerflowKey key)
	{ FlowUtil.installForwardingRules(sw, inPort, outPort, key); }
	
	@Override
	public void installForwardingRules(IOFSwitch sw, short inPort, 
			short[] outPorts, PerflowKey key)
	{ FlowUtil.installForwardingRules(sw, inPort, outPorts, key); }
	
	public void packetReceived(byte[] packet, IOFSwitch inSwitch, short inPort)
	{		
		// FIXME: Identify correct operation(s) to notify
		//OFMatch match = new OFMatch();
		//match.loadFromPacket(pi.getPacketData(), pi.getInPort());
		//PerflowKey key = PerflowKey.fromOFMatch(match);
		
		// FIXME: Do not send to every operation
		for (Operation op : this.operations.values())
		{ op.receivePacket(packet, inSwitch, inPort); }
	}
}

