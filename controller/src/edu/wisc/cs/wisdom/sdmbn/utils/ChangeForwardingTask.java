package edu.wisc.cs.wisdom.sdmbn.utils;

import java.util.List;

import edu.wisc.cs.wisdom.sdmbn.Middlebox;
import edu.wisc.cs.wisdom.sdmbn.PerflowKey;

public class ChangeForwardingTask implements IOperationFinishedTask 
{
	private PerflowKey key;
	private List<Middlebox> mbs;
	ISdmbnApp app;
	
	public ChangeForwardingTask(PerflowKey key, List<Middlebox> mbs, ISdmbnApp app)
	{
		this.key = key;
		this.mbs = mbs;
		this.app = app;
	}
	
	@Override
	public void run(int opId)
	{ this.app.changeForwarding(key, mbs); }
}
