package edu.wisc.cs.wisdom.sdmbn.utils;

import java.io.IOException;
import java.io.PrintWriter;
import java.net.InetSocketAddress;
import java.net.Socket;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Interface to a service that replays traffic traces using tcpreplay. 
 * @author Aaron Gember
 */
public class TraceLoad 
{
	protected static Logger log = LoggerFactory.getLogger(TraceLoad.class);
	
	/** Commands the service accepts */
	private static final String ACTION_START = "start";
	private static final String ACTION_STOP = "stop";
	
	private static final short REQUEST_SERVER_PORT = (short)8080;
	
	/** Address of the service */
	private InetSocketAddress serverAddr;
	
	/** Socket connection to the service */
	private Socket sock;
	
	/** The rate at which packets should be replayed */
	private int rate;
	
	/** The number of packets to be replayed**/
	private int numPkts;
	
	/**
	 * Create a new configuration for a trace replay.
	 * @param ip the IP address of the trace replay service
	 * @param rate the rate at which packets should be replayed
     * @param numPkts the maximum number of packets to replay
	 */
	public TraceLoad(String ip, int rate, int numPkts)
    { this(ip, REQUEST_SERVER_PORT, rate, numPkts); }

    /**
	 * Create a new configuration for a trace replay.
	 * @param ip the IP address of the trace replay service
     * @param port the TCP port of the trace replay service
	 * @param rate the rate at which packets should be replayed
     * @param numPkts the maximum number of packets to replay
	 */
	public TraceLoad(String ip, short port, int rate, int numPkts)
	{
		this.rate = rate;
		this.numPkts = numPkts;
		this.sock = new Socket();
		this.serverAddr = new InetSocketAddress(ip, port);
	}
	
	/**
	 * Connects to the trace replay service and issues commands.
	 * @param action the command to issue
	 * @param trace the trace to which the command applies
	 * @return true if the command was successful, otherwise false
	 */
	private boolean issueCommand(String action, String trace)
	{
		if (!this.sock.isConnected())
		{
			try 
			{ this.sock.connect(this.serverAddr);}
			catch (IOException e) 
			{
				log.error("Failed to connect to traceload sever");
				return false;
			}
		}
		
		try 
		{
			PrintWriter out = new PrintWriter(this.sock.getOutputStream());
			out.println(String.format("%s %s %d %d", action, trace, this.rate, this.numPkts));
			out.flush();
		}
		catch (IOException e)
		{
			log.error(String.format("Failed to issue command '%s %s %d %d'", action, trace, this.rate, this.numPkts));
			return false;
		}
		
		return true;
	}
	
	/**
	 * Starts replaying a particular traffic trace.
	 * @param trace the trace to start replaying
	 * @return true if the replay was successfully started, otherwise false
	 */
	public boolean startTrace(String trace)
	{ return this.issueCommand(ACTION_START, trace); }
	
	/**
	 * Stops replaying a particular traffic trace.
	 * @param trace the trace to stop replaying
	 * @return true if the replay was successfully stopped, otherwise false
	 */
	public boolean stopTrace(String trace)
	{ return this.issueCommand(ACTION_STOP, trace); }
}
