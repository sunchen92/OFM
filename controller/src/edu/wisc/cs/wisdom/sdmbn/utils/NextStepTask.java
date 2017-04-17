package edu.wisc.cs.wisdom.sdmbn.utils;

public class NextStepTask implements Runnable
{
	int step;
	ISdmbnApp app;
	
	public NextStepTask(int step, ISdmbnApp app)
	{
		this.step = step;
		this.app = app;
	}
	
	@Override
	public void run()
	{ this.app.executeStep(step); }
}
