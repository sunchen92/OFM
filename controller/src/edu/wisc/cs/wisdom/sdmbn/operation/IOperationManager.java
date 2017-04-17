package edu.wisc.cs.wisdom.sdmbn.operation;

import net.floodlightcontroller.core.IOFSwitch;
import edu.wisc.cs.wisdom.sdmbn.PerflowKey;

public interface IOperationManager 
{
	public void operationFinished(Operation op);
	public void operationFailed(Operation op);
	public void installForwardingRules(IOFSwitch sw, short inPort, 
			short outPort, PerflowKey key);
	public void installForwardingRules(IOFSwitch sw, short inPort, 
			short[] outPorts, PerflowKey key);
}
