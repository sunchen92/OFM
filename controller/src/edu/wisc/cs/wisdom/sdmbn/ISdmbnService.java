package edu.wisc.cs.wisdom.sdmbn;

import java.util.List;

import edu.wisc.cs.wisdom.sdmbn.Parameters.*;

import net.floodlightcontroller.core.module.IFloodlightService;

public interface ISdmbnService  extends IFloodlightService 
{	
	/*
	 * Moves state corresponding to 'key' and 'scope' from one Middlebox 'src' to 'dst'
	 * with 'guarantee' required. 
	 * inPort is the port on which the switch is receiving traffic.
	 * Scope: PERFLOW, MULTIFLOW, ALLFLOWS, PM, PA, MA, PMA
	 * Guarantee: NONE, LOSS_FREE, ORDER_PRESERVING
	 * Optimization: PARALLELIZE, EARLY_RELEASE, LATE_LOCK, PE, PL, EL, PEL
	 */
	public int move(Middlebox src, Middlebox dst, PerflowKey key, Scope scope,
			Guarantee guarantee, Optimization optimization, short inPort);
	
	/*
	 * Copies state corresponding to 'key' and 'scope' from Middlebox 'src' to 'dst'
	 * Scope: PERFLOW, MULTIFLOW, ALLFLOWS, PM, PA, MA, PMA
	 */
	public int copy(Middlebox src, Middlebox dst, PerflowKey key, Scope scope);
	
	/*
	 * Shares state corresponding to 'key' and 'scope' between 'list<Middlebox>' with strong or strict consistency.
	 * inPort is the port on which the switch is receiving traffic
	 * Scope: PERFLOW, MULTIFLOW, ALLFLOWS, PM, PA, MA, PMA
	 * Consistency: STRONG, STRICT
	 */
	public int share(List<Middlebox> instances, PerflowKey key,
			Scope scope, Consistency consistency);
	public int share(List<Middlebox> instances, PerflowKey key,
			Scope scope, Consistency consistency, short inPort);
	
	/*
	 * Moves config from one middlebox to another
	 */
	public int moveConfig(Middlebox src, Middlebox dst, PerflowKey key);
		
	/*
	 * Registers listener to receive SDMBN events
	 */
	public void addSdmbnListener(ISdmbnListener listener);

}
