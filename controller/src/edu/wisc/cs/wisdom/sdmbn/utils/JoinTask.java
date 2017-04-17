package edu.wisc.cs.wisdom.sdmbn.utils;

import java.util.Collection;

public class JoinTask implements IOperationFinishedTask
{
	Collection<Integer> joinOpIds;
	IOperationFinishedTask finishTask;
	
	public JoinTask(Collection<Integer> joinOpIds, IOperationFinishedTask finishTask)
	{
		this.joinOpIds = joinOpIds;
		this.finishTask = finishTask;
	}
	
	@Override
	public void run(int opId) 
	{
		synchronized(this.joinOpIds)
		{
			this.joinOpIds.remove(opId);
			if (0 == this.joinOpIds.size())
			{ this.finishTask.run(opId); }
		}
	}
}
