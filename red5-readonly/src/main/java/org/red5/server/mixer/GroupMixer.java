package org.red5.server.mixer;

import java.util.HashMap;
import java.util.Map;

import org.red5.server.api.service.IServiceCall;
import org.red5.server.net.rtmp.IRTMPHandler;
import org.red5.server.net.rtmp.RTMPConnManager;
import org.red5.server.net.rtmp.RTMPMinaConnection;
import org.red5.server.net.rtmp.event.ChunkSize;
import org.red5.server.net.rtmp.event.Invoke;
import org.red5.server.net.rtmp.message.Constants;
import org.red5.server.net.rtmp.message.Header;
import org.red5.server.net.rtmp.message.Packet;
import org.red5.server.service.Call;

public class GroupMixer {
	
	private static final String AppName = "myRed5App";
	private static final String ipAddr = "localhost"; //change to something else in the future ????
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
    	if( allInOneSessionId_ == null ) {
    		// create a connection
    		RTMPMinaConnection connAllInOne = (RTMPMinaConnection) RTMPConnManager.getInstance().createConnection(RTMPMinaConnection.class);
    		// add session to the connection
    		connAllInOne.setIoSession(null);
    		// add the handler
    		connAllInOne.setHandler(handler);
    		
    		//??? different thread , see mina threading model ???
    		//next assume the session is opened
    		handler.connectionOpened(connAllInOne);
    
    		//handle connect, createStream and publish events
    		handle3Events(connAllInOne, "__allInOne__");
            
    		// set it in MixerManager
    		allInOneSessionId_ = connAllInOne.getSessionId();
    	}
    }	
    
    public RTMPMinaConnection getAllInOneConn()
    {
    	return (RTMPMinaConnection) RTMPConnManager.getInstance().getConnectionBySessionId(allInOneSessionId_);
    }
    
    private void handle3Events(RTMPMinaConnection conn, String streamName)
    {
		///////////////////////////////////
		//handle StreamAction.CONNECT event
		//RTMP Chunk Header
		Header connectMsgHeader = new Header();
		connectMsgHeader.setDataType(Constants.TYPE_INVOKE);//invoke is command, val=20
		connectMsgHeader.setChannelId(3); //3 means invoke command
		// see RTMPProtocolDecoder::decodePacket() 
		// final int readAmount = (readRemaining > chunkSize) ? chunkSize : readRemaining;
		connectMsgHeader.setSize(1024);   //Chunk Data Length, a big enough buffersize
		connectMsgHeader.setStreamId(0);  //0 means netconnection
		connectMsgHeader.setTimerBase(0); //base+delta=timestamp
		connectMsgHeader.setTimerDelta(0);
		connectMsgHeader.setExtendedTimestamp(0); //extended timestamp
		
		//No need for PendingServiceCall only for callbacks with _result or _error
		/*
		if (call instanceof IPendingServiceCall) {
			registerPendingCall(connectMsgEvent.getTransactionId(), (IPendingServiceCall) connectMsgEvent);
		}
		*/   
		Invoke connectMsgEvent = new Invoke();
		connectMsgEvent.setHeader(connectMsgHeader);
		connectMsgEvent.setTimestamp(0);
		connectMsgEvent.setTransactionId(1);
		IServiceCall connectCall = new Call( "connect");
		connectMsgEvent.setCall(connectCall);
		
		Map<String, Object> connectParams = new HashMap<String, Object>();
		connectParams.put("app", AppName); //TODO change to something else in the future
		connectParams.put("flashVer", "FMSc/1.0");
		connectParams.put("swfUrl", "a.swf");
		connectParams.put("tcUrl", "rtmp://"+ipAddr+":1935/"+AppName); //server ip address???
		connectParams.put("fpad", false);
		connectParams.put("audioCodecs", 0x0fff); //All codecs
		connectParams.put("videoCodecs", 0x0ff); //All codecs
		connectParams.put("videoFunction", 1); //SUPPORT_VID_CLIENT_SEEK
		connectParams.put("pageUrl", "a.html");
		connectParams.put("objectEncoding", 0); //AMF0
		connectMsgEvent.setConnectionParams(connectParams);
		
		Packet connectMsg = new Packet(connectMsgHeader, connectMsgEvent);
		conn.handleMessageReceived(connectMsg);

		///////////////////////////////////
		//handle create Stream event

		//RTMP Chunk Header
		Header createStreamMsgHeader = new Header();
		createStreamMsgHeader.setDataType(Constants.TYPE_INVOKE);//invoke is command, val=20
		createStreamMsgHeader.setChannelId(3); //3 means invoke command
		// see RTMPProtocolDecoder::decodePacket() 
		// final int readAmount = (readRemaining > chunkSize) ? chunkSize : readRemaining;
		createStreamMsgHeader.setSize(1024);   //Chunk Data Length, a big enough buffersize
		createStreamMsgHeader.setStreamId(0);  //0 means netconnection
		createStreamMsgHeader.setTimerBase(0); //base+delta=timestamp
		createStreamMsgHeader.setTimerDelta(0);
		createStreamMsgHeader.setExtendedTimestamp(0); //extended timestamp
		
		Invoke createStreamMsgEvent = new Invoke();
		createStreamMsgEvent.setHeader(createStreamMsgHeader);
		createStreamMsgEvent.setTimestamp(0);
		createStreamMsgEvent.setTransactionId(2);
		IServiceCall createStreamCall = new Call( "createStream" );
		createStreamMsgEvent.setCall(createStreamCall);
		
		Packet createStreamMsg = new Packet(createStreamMsgHeader, createStreamMsgEvent);
		conn.handleMessageReceived(createStreamMsg);
		
		///////////////////////////////////
		//handle publish event

		//RTMP Chunk Header
		Header publishMsgHeader = new Header();
		publishMsgHeader.setDataType(Constants.TYPE_INVOKE);//invoke is command, val=20
		publishMsgHeader.setChannelId(3); //3 means invoke command
		// see RTMPProtocolDecoder::decodePacket() 
		// final int readAmount = (readRemaining > chunkSize) ? chunkSize : readRemaining;
		publishMsgHeader.setSize(1024);   //Chunk Data Length, a big enough buffersize
		publishMsgHeader.setStreamId(1);  //1 means the newly created stream
		publishMsgHeader.setTimerBase(0); //base+delta=timestamp
		publishMsgHeader.setTimerDelta(0);
		publishMsgHeader.setExtendedTimestamp(0); //extended timestamp
		
		Invoke publishMsgEvent = new Invoke();
		publishMsgEvent.setHeader(publishMsgHeader);
		publishMsgEvent.setTimestamp(0);
		publishMsgEvent.setTransactionId(3);
		Object [] publishArgs = new Object[2];
		publishArgs[0] = streamName;
		publishArgs[1] = "live";
		IServiceCall publishCall = new Call( "publish", publishArgs );
		publishMsgEvent.setCall(publishCall);
		
		Packet publishMsg = new Packet(publishMsgHeader, publishMsgEvent);
		conn.handleMessageReceived(publishMsg);
            		
        ///////////////////////////////////
        //handle chunksize event
        
        //RTMP Chunk Header
        Header chunkSizeMsgHeader = new Header();
        chunkSizeMsgHeader.setDataType(Constants.TYPE_CHUNK_SIZE);//invoke is command, val=1
        chunkSizeMsgHeader.setChannelId(2); //2 means protocol control message
        // see RTMPProtocolDecoder::decodePacket() 
        // final int readAmount = (readRemaining > chunkSize) ? chunkSize : readRemaining;
        chunkSizeMsgHeader.setSize(1024);   //Chunk Data Length, a big enough buffersize
        chunkSizeMsgHeader.setStreamId(1);  //1 means the newly created stream
        chunkSizeMsgHeader.setTimerBase(0); //base+delta=timestamp
        chunkSizeMsgHeader.setTimerDelta(0);
        chunkSizeMsgHeader.setExtendedTimestamp(0); //extended timestamp
        
        ChunkSize chunkSizeMsgEvent = new ChunkSize(0xffffff); //maxsize
        chunkSizeMsgEvent.setHeader(chunkSizeMsgHeader);
        chunkSizeMsgEvent.setTimestamp(0);
        
        Packet chunkSizeMsg = new Packet(chunkSizeMsgHeader, chunkSizeMsgEvent);
        conn.handleMessageReceived(chunkSizeMsg);
    }
}
