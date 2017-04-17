package edu.wisc.cs.wisdom.sdmbn.channel;

import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.channel.ChannelStateEvent;
import org.jboss.netty.channel.MessageEvent;
import org.jboss.netty.channel.SimpleChannelHandler;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import edu.wisc.cs.wisdom.sdmbn.core.SdmbnManager;
import edu.wisc.cs.wisdom.sdmbn.json.MessageException;

public abstract class BaseChannelHandler extends SimpleChannelHandler 
{
	protected SdmbnManager sdmbnManager;
	
	private static Logger log = 
		LoggerFactory.getLogger("sdmbn."+BaseChannelHandler.class.getSimpleName());
	
	public BaseChannelHandler(SdmbnManager sdmbnManager)
	{
		this.sdmbnManager = sdmbnManager;
	}
	
	@Override
	public abstract void channelConnected(ChannelHandlerContext ctx, ChannelStateEvent evt);
	
	@Override
	public void messageReceived(ChannelHandlerContext ctx, MessageEvent evt)
	{
		String line = (String)evt.getMessage();
		BaseChannel baseChannel = (BaseChannel)ctx.getAttachment();
		try
		{ baseChannel.processMessage(line); }
		catch(MessageException ex)
		{ log.error("Failed to process message: {}", ex); }
	}
}
