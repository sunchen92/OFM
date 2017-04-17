package edu.wisc.cs.wisdom.sdmbn.channel;

import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.channel.ChannelStateEvent;
import org.jboss.netty.channel.ExceptionEvent;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import edu.wisc.cs.wisdom.sdmbn.core.SdmbnManager;

public class EventChannelHandler extends BaseChannelHandler
{	
	private static Logger log = 
		LoggerFactory.getLogger("sdmbn."+EventChannelHandler.class.getSimpleName());
	
	public EventChannelHandler(SdmbnManager sdmbnManager)
	{ super(sdmbnManager); }
	
	@Override
	public void channelConnected(ChannelHandlerContext ctx, ChannelStateEvent e)
	{
		EventChannel eventChannel = new EventChannel(ctx.getChannel(), 
				this.sdmbnManager);
		ctx.setAttachment(eventChannel);
	}
	
	@Override
	public void exceptionCaught(ChannelHandlerContext ctx, ExceptionEvent evt) 
	{
		log.error("Channel exception");
		evt.getCause().printStackTrace();
		evt.getChannel().close();
	}
}

