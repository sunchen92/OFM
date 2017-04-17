package edu.wisc.cs.wisdom.sdmbn.utils;

import java.util.List;

import edu.wisc.cs.wisdom.sdmbn.Middlebox;
import edu.wisc.cs.wisdom.sdmbn.PerflowKey;

public interface ISdmbnApp 
{
	public void executeStep(int step);
	public void changeForwarding(PerflowKey key, List<Middlebox> mbs);
}
