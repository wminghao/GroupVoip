package org.red5.server.mixer;

import java.util.HashMap;
import java.util.Map;

import org.red5.logging.Red5LoggerFactory;
import org.red5.server.api.Red5;
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
import org.slf4j.Logger;

import java.util.List;
import java.util.Iterator;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ArrayBlockingQueue;

public class GroupMixer implements Runnable {
	
	private static final String AppName = "myRed5App";
	private static final String ipAddr = "localhost"; //change to something else in the future ????
	private static GroupMixer instance_;
	private String allInOneSessionId_ = null; //all-in-one mixer rtmp connection
	private static Logger log = Red5LoggerFactory.getLogger(Red5.class);
	
	//2 events in the blocking queue                                                                                                                                                                     
    public class GroupMixerAsyncEvent{

        public GroupMixerAsyncEvent(int eventId, int param1, int param2) {
            this.eventId = eventId;
            this.param1 = param1;
            this.param2 = param2;
        }

        public String getName() {
            String eventName = "";
            switch(eventId) {
            case GroupMixerAsyncEvent.MESSAGEINPUT_REQ:
                {
                    eventName = "Input message to processpipe";
                    break;
                }
            case GroupMixerAsyncEvent.MESSAGEOUTPUT_REQ:
                {
                    eventName = "Output message to processpipe";
                    break;
                }
            }
            return "event is " + eventName + " param1=" + param1 + " param2=" + param2;
        }

        private int eventId;
        private int param1; //streamNumber = streamId in mixcoder
        private int param2; //FLVFrame raw data

        public static final int MESSAGEINPUT_REQ 	   = 0;
        public static final int MESSAGEOUTPUT_REQ      = 1;
        public static final int SHUTDOWN_REQ 		   = 2;
    }
    private BlockingQueue<GroupMixerAsyncEvent> asyncEventQueue = new ArrayBlockingQueue<GroupMixerAsyncEvent>(100);

	public GroupMixer() {
		//start the thread immediately
    	Thread t = new Thread(this, "GroupMixerThread");
    	t.start();
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

    		log.info("Created all In One connection on thread: {}", Thread.currentThread().getName());
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
    
    public void addEvent(int eventId, int param1, int param2) {
        try {
            GroupMixerAsyncEvent event = new GroupMixerAsyncEvent(eventId, param1, param2);
            log.info("GroupMixer addEvent ="+event.getName());
            asyncEventQueue.put(event);
        } catch (InterruptedException iex) {
        	log.error("GroupMixer addEvent Interrupted error: "+iex.toString());
        } catch (Exception ex) {
        	log.error("GroupMixer addEvent Generic error: "+ex.toString());
        }
    }

    @Override
    public void run() {
        log.info("GroupMix thread enters");
        try {
        	GroupMixerAsyncEvent event;
            while ((event = asyncEventQueue.take()).eventId != GroupMixerAsyncEvent.SHUTDOWN_REQ) {

                log.info("AVRecorder processEvent ="+event.getName());
                
                switch(event.eventId) {
                case GroupMixerAsyncEvent.MESSAGEINPUT_REQ:
                    {
                    	//TODO input event
                        break;
                    }
                case GroupMixerAsyncEvent.MESSAGEOUTPUT_REQ:
                    {
                    	//TODO handle output event
                        break;
                    }
                }
            }
        } catch (Exception ex) {
        	log.error("GroupMixer run Generic error: "+ex.toString());
        }
        log.info("GroupMix thread exits");
    }
}
