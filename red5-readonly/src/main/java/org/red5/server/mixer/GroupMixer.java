package org.red5.server.mixer;

import java.util.HashMap;
import java.util.Map;

import org.red5.io.object.StreamAction;
import org.red5.server.api.service.IPendingServiceCall;
import org.red5.server.api.service.IPendingServiceCallback;
import org.red5.server.api.service.IServiceCall;
import org.red5.server.net.rtmp.IRTMPHandler;
import org.red5.server.net.rtmp.RTMPConnManager;
import org.red5.server.net.rtmp.RTMPMinaConnection;
import org.red5.server.net.rtmp.event.Invoke;
import org.red5.server.net.rtmp.event.IRTMPEvent;
import org.red5.server.net.rtmp.message.Constants;
import org.red5.server.net.rtmp.message.Header;
import org.red5.server.net.rtmp.message.Packet;
import org.red5.server.service.Call;
import org.red5.server.service.PendingCall;
import org.red5.io.object.StreamAction;

public class GroupMixer {
	
	private static final String AppName = "myRed5App";
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
    		//RTMP Chunk Header
    		Header connectMsgHeader = new Header();
    		connectMsgHeader.setDataType(Constants.TYPE_INVOKE);//invoke is command, val=20
    		connectMsgHeader.setChannelId(3); //3 means invoke command
    		connectMsgHeader.setSize(0);      //Chunk Data Length
    		connectMsgHeader.setStreamId(0);  //0 means netconnection
    		connectMsgHeader.setTimerBase(0); //base+delta=timestamp
    		connectMsgHeader.setTimerDelta(0);
    		connectMsgHeader.setExtendedTimestamp(0); //extended timestamp
    		
    		Invoke connectMsgEvent = new Invoke();
    		connectMsgEvent.setHeader(connectMsgHeader);
    		connectMsgEvent.setTimestamp(0);
    		connectMsgEvent.setTransactionId(1);
    		IServiceCall call = new Call( "connect");
    		connectMsgEvent.setCall(call);
    		
    		Map<String, Object> connectParams = new HashMap<String, Object>();
    		connectParams.put("app", AppName); //TODO change to something else in the future
    		connectParams.put("flashVer", "FMSc/1.0");
    		connectParams.put("swfUrl", "file://C:/FlvPlayer.swf");
    		connectParams.put("tcUrl", "rtmp://localhost:1935/"+AppName); //server ip address???
    		connectParams.put("fpad", false);
    		connectParams.put("audioCodecs", 0x0fff); //All codecs
    		connectParams.put("videoCodecs", 0x0ff); //All codecs
    		connectParams.put("videoFunction", 1); //SUPPORT_VID_CLIENT_SEEK
    		connectParams.put("pageUrl", "http://localhost/self.html");
    		connectParams.put("objectEncoding", 0); //AMF0 or AMF3 ???
    		connectMsgEvent.setConnectionParams(connectParams);
    		/*
    		//TODO PendingServiceCall only for callbacks???
    		if (call instanceof IPendingServiceCall) {
    			registerPendingCall(connectMsgEvent.getTransactionId(), (IPendingServiceCall) connectMsgEvent);
    		}
    		*/    		
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
