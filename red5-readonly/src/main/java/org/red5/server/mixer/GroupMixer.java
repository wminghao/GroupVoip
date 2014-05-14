package org.red5.server.mixer;

import org.red5.server.net.rtmp.IRTMPHandler;
import org.red5.server.net.rtmp.RTMPConnManager;
import org.red5.server.net.rtmp.RTMPMinaConnection;
import org.red5.server.net.rtmp.event.Invoke;
import org.red5.server.net.rtmp.event.IRTMPEvent;
import org.red5.server.net.rtmp.message.Constants;
import org.red5.server.net.rtmp.message.Header;
import org.red5.server.net.rtmp.message.Packet;

public class GroupMixer {
	
	private static GroupMixer instance_;
	public String allInOneSessionId_ = null; //all-in-one mixer rtmp connection

	public GroupMixer() {
	}
	
    public static synchronized GroupMixer getInstance() {
        if(instance_ == null) {
        	instance_ = new GroupMixer();
        }
        return instance_;
    }
    
    public void tryToCreateAllInOneConn(IRTMPHandler handler)
    {
    	if( allInOneSessionId_ ==null ) {
    		// create a connection
    		RTMPMinaConnection connAllInOne = (RTMPMinaConnection) RTMPConnManager.getInstance().createConnection(RTMPMinaConnection.class);
    		// add session to the connection
    		connAllInOne.setIoSession(null);
    		// add the handler
    		connAllInOne.setHandler(handler);
    		
    		//??? different thread , see mina threading model ???
    		//next assume the session is opened
    		handler.connectionOpened(connAllInOne);
    
    		///////////////////////////////////
    		//handle StreamAction.CONNECT event
    		Header connectMsgHeader = new Header();
    		connectMsgHeader.setDataType(Constants.TYPE_INVOKE);
    		connectMsgHeader.setStreamId(0);
    		connectMsgHeader.setChannelId(2); //2 means command
    		connectMsgHeader.setExtendedTimestamp(0); //???
    		
    		IRTMPEvent connectMsgEvent = new Invoke();
    		connectMsgEvent.setHeader(connectMsgHeader);
    		connectMsgEvent.setTimestamp(0);
    		
    		
    		Packet connectMsg = new Packet(connectMsgHeader, connectMsgEvent);
    		connAllInOne.handleMessageReceived(connectMsg);
    
    		///////////////////////////////////
    		//handle create Stream event
    		Packet createStreamMsg = null;
    		connAllInOne.handleMessageReceived(createStreamMsg);
    
    		///////////////////////////////////
    		//handle publish Stream event
    		Packet publishMsg = null;
    		connAllInOne.handleMessageReceived(publishMsg);
    		
    		// set it in MixerManager
    		allInOneSessionId_ = connAllInOne.getSessionId();
    	}
    }	
    
    public RTMPMinaConnection getAllInOneConn()
    {
    	return (RTMPMinaConnection) RTMPConnManager.getInstance().getConnectionBySessionId(allInOneSessionId_);
    }
}
