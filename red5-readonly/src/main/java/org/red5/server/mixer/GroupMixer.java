package org.red5.server.mixer;

import java.util.HashMap;
import java.util.Map;

import org.red5.logging.Red5LoggerFactory;
import org.red5.server.api.Red5;
import org.red5.server.net.rtmp.IRTMPHandler;
import org.red5.server.net.rtmp.RTMPConnManager;
import org.red5.server.net.rtmp.RTMPMinaConnection;
import org.red5.server.net.rtmp.event.AudioData;
import org.red5.server.net.rtmp.event.ChunkSize;
import org.red5.server.net.rtmp.event.Invoke;
import org.red5.server.net.rtmp.event.Notify;
import org.red5.server.net.rtmp.event.VideoData;
import org.red5.server.net.rtmp.message.Constants;
import org.red5.server.net.rtmp.message.Header;
import org.red5.server.net.rtmp.message.Packet;
import org.red5.server.service.PendingCall;
import org.slf4j.Logger;

import java.util.BitSet;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ArrayBlockingQueue;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class GroupMixer implements Runnable, SegmentParser.Delegate {

	public static final String MIXED_STREAM_PREFIX = "__mixed__";
	private static final String ALL_IN_ONE_STREAM_NAME = "allinone";
	private static final String AppName = "myRed5App";//TODO appName change to a room or something
	private static final String ipAddr = "localhost"; //TODO change to something else in the future
	private static GroupMixer instance_;
	private String allInOneSessionId_ = null; //all-in-one mixer rtmp connection
	private int totalInputStreams = 0; //total inputStreams, not including all-in-one stream
	private static Logger log = Red5LoggerFactory.getLogger(Red5.class);
	
	//mapping from original to streamId to newly generated stream
	private class GroupMappingTableEntry {
		public int 	  mixerId; //streamId used in MixCoder
		public int	  streamId; //streamId used in RTMP protocol
	}
	private Map<String,GroupMappingTableEntry> groupMappingTable=new HashMap<String,GroupMappingTableEntry>();
	private ProcessPipe mixerPipe_ = new ProcessPipe(this);
	
	/**
	 * Reserved stream ids. Stream id's directly relate to individual NetStream instances.
	 */
	private volatile BitSet reservedStreams = new BitSet();
	/**
	 * Reserved mixer ids. Mixer id's directly relate to mixcoder.
	 */
	private volatile BitSet mixerStreams = new BitSet();
	
	//n events in the blocking queue                                                                                                                                                                     
	private class GroupMixerAsyncEvent{

        public GroupMixerAsyncEvent(int eventId, String paramStr, ByteBuffer flvFrame, int len) {
            this.eventId  = eventId;
            this.paramStr = paramStr;
            this.flvFrame = flvFrame;
            this.flvFrameLen = len;
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
                case GroupMixerAsyncEvent.CREATESTREAM_REQ:
                {
                    eventName = "Stream is created";
                    break;
                }
                case GroupMixerAsyncEvent.DELETESTREAM_REQ:
                {
                    eventName = "Stream is deleted";
                    break;
                }
                case GroupMixerAsyncEvent.SHUTDOWN_REQ:
                {
                    eventName = "Everything is shutdown";
                    break;
                }
            }
            return "event is " + eventName + " param1=" + paramStr + " param2=" + flvFrameLen;
        }

        private int eventId;
        private String paramStr; //either it's the streamName or streamId (!= streamId in mixcoder from 0-32)
        private ByteBuffer flvFrame; //FLVFrame raw data
        private int flvFrameLen; //frame len

        public static final int CREATESTREAM_REQ      	= 0;
        public static final int DELETESTREAM_REQ      	= 1;
        public static final int MESSAGEINPUT_REQ 	  	= 2;
        public static final int MESSAGEOUTPUT_REQ      	= 3;
        public static final int SHUTDOWN_REQ 		   	= 100;
    }
    
    private BlockingQueue<GroupMixerAsyncEvent> asyncEventQueue = new ArrayBlockingQueue<GroupMixerAsyncEvent>(100);
    
	private GroupMixer() {
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
    		RTMPMinaConnection connAllInOne = (RTMPMinaConnection) RTMPConnManager.getInstance().createConnection(RTMPMinaConnection.class, false);
    		// add session to the connection
    		connAllInOne.setIoSession(null);
    		// add the handler
    		connAllInOne.setHandler(handler);
    		
    		// set it in MixerManager
    		allInOneSessionId_ = connAllInOne.getSessionId();
    		
    		//??? different thread , see mina threading model ???
    		//next assume the session is opened
    		handler.connectionOpened(connAllInOne);
    
    		//handle connect, createStream and publish events
    		handleConnectEvent(connAllInOne);
            

    		//start the thread immediately
        	Thread t = new Thread(this, "GroupMixerThread");
        	t.start();
        	
        	//kick off createStream event
        	createMixedStream(ALL_IN_ONE_STREAM_NAME);
        	
    		log.info("Created all In One connection with sessionId {} on thread: {}", allInOneSessionId_, Thread.currentThread().getName());
    	}
    }	
    
    public void createMixedStream(String streamName)
    {
    	addEvent(GroupMixerAsyncEvent.CREATESTREAM_REQ, streamName, null, 0);
    }    
    public void deleteMixedStream(String streamName)
    {
    	addEvent(GroupMixerAsyncEvent.DELETESTREAM_REQ, streamName, null, 0);
    }
    public void inputMessage(String streamName, boolean bIsVideo, ByteBuffer buf, int eventTime)
    {	
		int dataLen = buf.limit();
		int flvFrameLen = 11 + dataLen + 4;
		
		ByteBuffer flvFrame;
        if( buf.isDirect() ) {
        	flvFrame = ByteBuffer.allocateDirect(flvFrameLen);
        } else {
        	flvFrame = ByteBuffer.allocate(flvFrameLen);
        }
		
		flvFrame.order(ByteOrder.LITTLE_ENDIAN);  // to use little endian
		flvFrame.put((byte)(bIsVideo?0x09:0x08)); //audio type
		flvFrame.put((byte)((dataLen>>16)&0xff));//datalen
		flvFrame.put((byte)((dataLen>>8)&0xff));//datalen
		flvFrame.put((byte)( dataLen&0xff));//datalen

		flvFrame.put((byte)((eventTime>>16)&0xff));//ts
		flvFrame.put((byte)((eventTime>>8)&0xff));//ts
		flvFrame.put((byte)( eventTime&0xff));//ts
		flvFrame.put((byte)((eventTime>>24)&0xff));//ts

		flvFrame.put((byte)0x0); //streamId, ignore
		flvFrame.put((byte)0x0); //streamId, ignore
		flvFrame.put((byte)0x0); //streamId, ignore

	    // Flip and read from the original.
	    final ByteBuffer readOnlyCopy = buf.asReadOnlyBuffer();
	    readOnlyCopy.flip();
		flvFrame.put(readOnlyCopy);
		flvFrame.putInt(0);//prevSize, ignore
        addEvent(GroupMixerAsyncEvent.MESSAGEINPUT_REQ, streamName, flvFrame, flvFrameLen);
    	
        log.info("=====>input message from {} type {} ts {} on thread: {}", streamName, bIsVideo?"video":"audio", eventTime,  Thread.currentThread().getName());
    }

    public void onFrameParsed(int mixerId, ByteBuffer frame, int len)
    {
    	String streamName = null;
    	if( mixerId == 32) {
    		streamName = ALL_IN_ONE_STREAM_NAME;
    	} else {
        	for(String key : groupMappingTable.keySet()) {
        		GroupMappingTableEntry value = groupMappingTable.get(key);
        		if(value.mixerId == mixerId) {
        			streamName = key;
        		}
            }
    	}
    	log.info("=====>onFrameParsed mixerId {} len {} streamName {}", mixerId, len, streamName );
    	if ( streamName != null ) {
    		ByteBuffer flvFrame = ByteBuffer.allocate(len);
    		flvFrame.order(ByteOrder.LITTLE_ENDIAN);  // to use little endian

    	    final ByteBuffer readOnlyCopy = frame.asReadOnlyBuffer();
    	    readOnlyCopy.flip();
    	    readOnlyCopy.limit(len);
    		flvFrame.put(readOnlyCopy);
    		flvFrame.flip();
    		addEvent(GroupMixerAsyncEvent.MESSAGEOUTPUT_REQ, streamName, flvFrame, len);
    	}
    }
    
    public void shutdown() 
    {
    	addEvent(GroupMixerAsyncEvent.SHUTDOWN_REQ, null, null, 0);
    }
    
    private void handleShutdown()
    {
    	//TODO close all-in-one RTMPConnections and all its associated assets
    }

    private void handleInputFlvFrame(String streamName, ByteBuffer flvFrame, int rawDataLen)
    {
    	GroupMappingTableEntry entry = groupMappingTable.get(streamName);
    	if ( entry != null ) {        	
    		int flvFrameLen = 11 + rawDataLen + 4;
    		int segHeaderLen = 8 + 6*totalInputStreams; //additional headers
    		ByteBuffer flvSegment = ByteBuffer.allocate(flvFrameLen+segHeaderLen); //TODO direct?
    		flvSegment.put((byte)'S');
    		flvSegment.put((byte)'G');
    		flvSegment.put((byte)'I');
    		flvSegment.put((byte)0); //even layout
    		flvSegment.putInt(getMixerMask()); //calc mask here
    		
    		for (int i = 0; i < 32; i++) {
    			if (mixerStreams.get(i)) {
    				byte idtype = (byte)((i<<3) | 0x02);// TODO assume all mobile channels
    				flvSegment.put(idtype);
    				flvSegment.put((byte)0); //ignore for now
    				
    				if( entry.mixerId == i) {
        				flvSegment.putInt(flvFrameLen);
        				flvSegment.put(flvFrame.array(), 0, rawDataLen); //TODO optimization here
    				} else {
        				flvSegment.putInt(0); //no data for this stream
    				}
    			}
    		}
    		flvSegment.flip();
        	mixerPipe_.handleSegInput(flvSegment);
    	}
    }
    private void handleOutputFlvFrame(String streamName, ByteBuffer flvFrameBuffer, int flvFrameLen)
    {
		GroupMappingTableEntry value = groupMappingTable.get(streamName);
		if( value != null ) {
    		int streamId = value.streamId;
    		
    		byte[] flvFrame = flvFrameBuffer.array();
    		int curIndex = 0;
        	if(flvFrame[0] =='F' && flvFrame[1]=='L' && flvFrame[2] == 'V') {
        		curIndex+=13;
        	}        		
    		while( curIndex < flvFrameLen ) {
        		int msgType = flvFrame[curIndex];
            	int msgSize = ((((int)flvFrame[curIndex+1])&0xff)<<16) | ((((int)flvFrame[curIndex+2])&0xff)<<8) | ((int)(flvFrame[curIndex+3])&0xff);
            	int msgTimestamp = ((((int)flvFrame[curIndex+4])&0xff)<<16) | ((((int)flvFrame[curIndex+5])&0xff)<<8) | ((int)(flvFrame[curIndex+6])&0xff) | ((((int)flvFrame[curIndex+7])&0xff)<<24);
        		log.info("=====>out message from {} msgType {} msgSize {} ts {} streamId {} on thread: {}", streamName, msgType, msgSize, msgTimestamp, streamId, Thread.currentThread().getName());
            	
        		curIndex += 11;
        		
        		//RTMP Chunk Header
        		Header msgHeader = new Header();
        		msgHeader.setDataType((byte)msgType);//invoke is command, val=20
        		msgHeader.setChannelId(3); //channel TODO does it really matter since we consume it internally.
        		// see RTMPProtocolDecoder::decodePacket() 
        		// final int readAmount = (readRemaining > chunkSize) ? chunkSize : readRemaining;
        		msgHeader.setSize(msgSize);   //Chunk Data Length, a big enough buffersize
        		msgHeader.setStreamId(streamId);  //streamid
        		msgHeader.setTimerBase(0); //base+delta=timestamp
        		msgHeader.setTimerDelta(msgTimestamp);
        		msgHeader.setExtendedTimestamp(0); //extended timestamp
        		
            	RTMPMinaConnection conn = getAllInOneConn();
            	switch(msgType) {
            		case Constants.TYPE_AUDIO_DATA:
            		{
            			AudioData msgEvent = new AudioData();
            			msgEvent.setHeader(msgHeader);
            			msgEvent.setTimestamp(msgTimestamp);
            			msgEvent.setDataRemaining(flvFrame, curIndex, msgSize);   
            			msgEvent.setSourceType(Constants.SOURCE_TYPE_LIVE);       			
            			
            			Packet msg = new Packet(msgHeader, msgEvent);
            			conn.handleMessageReceived(msg);
            			
            			break;
            		}
            		case Constants.TYPE_VIDEO_DATA:
            		{
            			VideoData msgEvent = new VideoData();
            			msgEvent.setHeader(msgHeader);
            			msgEvent.setTimestamp(msgTimestamp);
            			msgEvent.setDataRemaining(flvFrame, curIndex, msgSize);     
            			msgEvent.setSourceType(Constants.SOURCE_TYPE_LIVE);   
            			
            			Packet msg = new Packet(msgHeader, msgEvent);
            			conn.handleMessageReceived(msg);
            			break;
            		}
        
            		case Constants.TYPE_STREAM_METADATA:
            		{
            			Notify msgEvent = new Notify();
            			msgEvent.setHeader(msgHeader);
            			msgEvent.setTimestamp(msgTimestamp);
            			msgEvent.setDataRemaining(flvFrame, curIndex, msgSize);    
            			msgEvent.setSourceType(Constants.SOURCE_TYPE_LIVE);
            			
            			Packet msg = new Packet(msgHeader, msgEvent);
            			conn.handleMessageReceived(msg);
            			break;
            		}
            	}
            	curIndex += (msgSize+4); //4 bytes unused
    		}
		}
    }
    private void handleCreateMixedStream(String streamName)
    {
    	RTMPMinaConnection conn = getAllInOneConn();
    	GroupMappingTableEntry entry = new GroupMappingTableEntry();
    	entry.mixerId = getMixerId();
    	entry.streamId = handleCreatePublishEvents(conn, MIXED_STREAM_PREFIX+streamName);
    	groupMappingTable.put(streamName, entry);
    	totalInputStreams++;
		log.info("A new stream id: {}, mixer id: {} is created on thread: {}", entry.streamId, entry.mixerId, Thread.currentThread().getName());
    }
    
    private void handleDeleteMixedStream(String streamName)
    {
    	GroupMappingTableEntry entry = groupMappingTable.get(streamName);
    	if ( entry != null ) {        	
        	RTMPMinaConnection conn = getAllInOneConn();
        	handleDeleteEvent(conn, entry.streamId, entry.mixerId);
        	groupMappingTable.remove(streamName);
        	totalInputStreams--;
    		log.info("A old stream id: {}, mixer id: {} is deleted on thread: {}", entry.streamId, entry.mixerId, Thread.currentThread().getName());
    	}
    }
    
    public RTMPMinaConnection getAllInOneConn()
    {
    	return (RTMPMinaConnection) RTMPConnManager.getInstance().getConnectionBySessionId(allInOneSessionId_);
    }
    
    private void handleConnectEvent(RTMPMinaConnection conn)
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
		PendingCall connectCall = new PendingCall( "connect");
		Invoke connectMsgEvent = new Invoke(connectCall);
		connectMsgEvent.setHeader(connectMsgHeader);
		connectMsgEvent.setTimestamp(0);
		connectMsgEvent.setTransactionId(1);
		connectMsgEvent.setCall(connectCall);
		
		Map<String, Object> connectParams = new HashMap<String, Object>();
		connectParams.put("app", AppName);
		connectParams.put("flashVer", "FMSc/1.0");
		connectParams.put("swfUrl", "a.swf");
		connectParams.put("tcUrl", "rtmp://"+ipAddr+":1935/"+AppName); //server ip address
		connectParams.put("fpad", false);
		connectParams.put("audioCodecs", 0x0fff); //All codecs
		connectParams.put("videoCodecs", 0x0ff); //All codecs
		connectParams.put("videoFunction", 1); //SUPPORT_VID_CLIENT_SEEK
		connectParams.put("pageUrl", "a.html");
		connectParams.put("objectEncoding", 0); //AMF0
		connectMsgEvent.setConnectionParams(connectParams);
		
		Packet connectMsg = new Packet(connectMsgHeader, connectMsgEvent);
		conn.handleMessageReceived(connectMsg);    	

		log.info("A new connection event sent on thread: {}", Thread.currentThread().getName());
    }
    
    private int handleCreatePublishEvents(RTMPMinaConnection conn, String streamName)
    {
    	int nextStreamId = reserveStreamId();
    	
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

		PendingCall createStreamCall = new PendingCall( "createStream" );
		Invoke createStreamMsgEvent = new Invoke(createStreamCall);
		createStreamMsgEvent.setHeader(createStreamMsgHeader);
		createStreamMsgEvent.setTimestamp(0);
		createStreamMsgEvent.setTransactionId(2);
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
		publishMsgHeader.setStreamId(nextStreamId);  // the newly created stream id
		publishMsgHeader.setTimerBase(0); //base+delta=timestamp
		publishMsgHeader.setTimerDelta(0);
		publishMsgHeader.setExtendedTimestamp(0); //extended timestamp

		Object [] publishArgs = new Object[2];
		publishArgs[0] = streamName;
		publishArgs[1] = "live";
		PendingCall publishCall = new PendingCall( "publish", publishArgs );
		Invoke publishMsgEvent = new Invoke(publishCall);
		publishMsgEvent.setHeader(publishMsgHeader);
		publishMsgEvent.setTimestamp(0);
		publishMsgEvent.setTransactionId(3);
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
        chunkSizeMsgHeader.setStreamId(nextStreamId);  //the newly created stream
        chunkSizeMsgHeader.setTimerBase(0); //base+delta=timestamp
        chunkSizeMsgHeader.setTimerDelta(0);
        chunkSizeMsgHeader.setExtendedTimestamp(0); //extended timestamp
        
        ChunkSize chunkSizeMsgEvent = new ChunkSize(0xffffff); //maxsize
        chunkSizeMsgEvent.setHeader(chunkSizeMsgHeader);
        chunkSizeMsgEvent.setTimestamp(0);
        
        Packet chunkSizeMsg = new Packet(chunkSizeMsgHeader, chunkSizeMsgEvent);
        conn.handleMessageReceived(chunkSizeMsg);
        
		log.info("A new stream with id {} is created on thread: {}", nextStreamId, Thread.currentThread().getName());
		
        return nextStreamId;
    }

    private void handleDeleteEvent(RTMPMinaConnection conn, int streamId, int mixerId)
    {
    	unreserveStreamId(streamId);
    	unreserveMixerId(mixerId);    	
		///////////////////////////////////
		//handle delete Stream event

		//RTMP Chunk Header
		Header deleteStreamMsgHeader = new Header();
		deleteStreamMsgHeader.setDataType(Constants.TYPE_INVOKE);//invoke is command, val=20
		deleteStreamMsgHeader.setChannelId(3); //3 means invoke command
		// see RTMPProtocolDecoder::decodePacket() 
		// final int readAmount = (readRemaining > chunkSize) ? chunkSize : readRemaining;
		deleteStreamMsgHeader.setSize(1024);   //Chunk Data Length, a big enough buffersize
		deleteStreamMsgHeader.setStreamId(0);  //0 means netconnection
		deleteStreamMsgHeader.setTimerBase(0); //base+delta=timestamp
		deleteStreamMsgHeader.setTimerDelta(0);
		deleteStreamMsgHeader.setExtendedTimestamp(0); //extended timestamp

		Object [] deleteArgs = new Object[1];
		deleteArgs[0] = streamId;
		PendingCall deleteStreamCall = new PendingCall( "deleteStream", deleteArgs );
		Invoke deleteStreamMsgEvent = new Invoke(deleteStreamCall);
		deleteStreamMsgEvent.setHeader(deleteStreamMsgHeader);
		deleteStreamMsgEvent.setTimestamp(0);
		deleteStreamMsgEvent.setTransactionId(0);
		deleteStreamMsgEvent.setCall(deleteStreamCall);
		
		Packet deleteStreamMsg = new Packet(deleteStreamMsgHeader, deleteStreamMsgEvent);
		conn.handleMessageReceived(deleteStreamMsg);

		log.info("A old stream with id {} is deleted on thread: {}", streamId, Thread.currentThread().getName());
    }
    
    private void addEvent(int eventId, String paramStr, ByteBuffer flvFrame, int len) {
        try {
            GroupMixerAsyncEvent event = new GroupMixerAsyncEvent(eventId, paramStr, flvFrame, len);
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

                log.info("GroupMix processEvent ="+event.getName());
                
                switch(event.eventId) {
                    case GroupMixerAsyncEvent.CREATESTREAM_REQ:
                    {
                    	//handle create stream event
                    	handleCreateMixedStream(event.paramStr);
                        break;
                    }
                    case GroupMixerAsyncEvent.DELETESTREAM_REQ:
                    {
                    	//handle delete stream event
                    	handleDeleteMixedStream(event.paramStr);
                        break;
                    }
                    case GroupMixerAsyncEvent.MESSAGEINPUT_REQ:
                    {
                    	handleInputFlvFrame(event.paramStr, event.flvFrame, event.flvFrameLen);
                        break;
                    }
                    case GroupMixerAsyncEvent.MESSAGEOUTPUT_REQ:
                    {
                    	handleOutputFlvFrame(event.paramStr, event.flvFrame, event.flvFrameLen);
                        break;
                    }
                }
            }
            if( event.eventId == GroupMixerAsyncEvent.SHUTDOWN_REQ ) {
            	//handle shutdown here
            	handleShutdown();
            }
            
        } catch (Exception ex) {
        	log.error("GroupMixer run Generic error: "+ex.toString());
        }
        log.info("GroupMix thread exits");
    }
    
    //map streamId to the actual streamId in StreamService class
    private int reserveStreamId() {
		int result = -1;
		for (int i = 0; true; i++) {
			if (!reservedStreams.get(i)) {
				reservedStreams.set(i);
				result = i;
				break;
			}
		}
		return result + 1;
	}

    private void unreserveStreamId(int streamId) {
		if (streamId > 0) {
			reservedStreams.clear(streamId - 1);
		}
	}
	
    //map streamId to the 0-32 streamId used in mixcoder
	private int getMixerId() {
		int result = -1;
		for (int i = 0; true; i++) {
			if (!mixerStreams.get(i)) {
				mixerStreams.set(i);
				result = i;
				break;
			}
		}
		return result;
	}
	//return a mask of mixerid
	private int getMixerMask() {
		int result = 0;
		for (int i = 0; i < 32; i++) {
			if (mixerStreams.get(i)) {
				result |= 0x1;
			}
			result <<= 1;
		}
		return result;
	}

	private void unreserveMixerId(int mixerId) {
		if (mixerId >= 0) {
			mixerStreams.clear(mixerId);
		}
	}
}
