package org.red5.server.mixer;

import java.util.BitSet;
import java.util.HashMap;
import java.util.Map;

import org.red5.logging.Red5LoggerFactory;
import org.red5.server.api.Red5;
import org.slf4j.Logger;

public class IdLookup {

	public static final int MAX_STREAM_COUNT = 32;
	//mapping from original to streamId to newly generated stream
	public class GroupMappingTableEntry {
		public int 	  mixerId; //streamId used in MixCoder
		public int	  streamId; //streamId used in RTMP protocol
	}
	private Map<String,GroupMappingTableEntry> groupMappingTable=new HashMap<String,GroupMappingTableEntry>();
	int totalInputStreams = 0;
	/**
	 * Reserved stream ids. Stream id's directly relate to individual NetStream instances.
	 */
	private volatile BitSet reservedStreams = new BitSet();
	/**
	 * Reserved mixer ids. Mixer id's directly relate to mixcoder.
	 */
	private volatile BitSet mixerStreams = new BitSet();

	private static Logger log = Red5LoggerFactory.getLogger(Red5.class);
	
	private Object syncObj = new Object();
    
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
	
    //map streamId to the 0-MAX_STREAM_COUNT streamId used in mixcoder
	private int reserveMixerId(String streamName) {
		int result = -1;
		if(streamName.equalsIgnoreCase(GroupMixer.ALL_IN_ONE_STREAM_NAME)) {
			result = MAX_STREAM_COUNT;
		} else {
    		for (int i = 0; true; i++) {
    			if (!mixerStreams.get(i)) {
    				mixerStreams.set(i);
    				result = i;
    				break;
    			}
    		}
		}
		return result;
	}
	//return a mask of mixerid
	private int getMixerMask() {
		int result = 0;
		for (int i = MAX_STREAM_COUNT-1; i>=0; i--) {
			if (mixerStreams.get(i)) {
				result |= 0x1;
			}
			if( i > 0 ) {
				result <<= 1;
			}
		}
        //log.info("======>getMixerMask value={}", result);
		return result;
	}

	private void unreserveMixerId(int mixerId) {
		if (mixerId >= 0) {
			mixerStreams.clear(mixerId);
		}
	}
	
	public int lookupStreamId(int mixerId) {
		int streamId = -1;
		synchronized( syncObj) {
    		for(String key : groupMappingTable.keySet()) {
        		GroupMappingTableEntry value = groupMappingTable.get(key);
        		if(value.mixerId == mixerId) {
        			streamId = value.streamId;
        		}
            }
		}
		return streamId;
	}
	public int lookupMixerIdAndMask(String streamName, int [] result) {
		int mixerId = -1;
		synchronized( syncObj) {
        	GroupMappingTableEntry entry = groupMappingTable.get(streamName);
        	if ( entry != null ) {     
        		mixerId = entry.mixerId;
        		result[0] = getMixerMask();
        		result[1] = totalInputStreams;
        	}
		}
		return mixerId;
	}
	
	public int createNewEntry(String streamName) {
		GroupMappingTableEntry entry = new GroupMappingTableEntry();
		synchronized( syncObj) {
        	entry.mixerId = reserveMixerId(streamName);
        	entry.streamId = reserveStreamId();
        	groupMappingTable.put(streamName, entry);
        	totalInputStreams++;
		}
		log.info("A new stream id: {}, mixer id: {} name: {} is created on thread: {}", entry.streamId, entry.mixerId, streamName, Thread.currentThread().getName());
    	return entry.streamId;
	}
	
	public int deleteEntry(String streamName) {
		int streamId = -1;
		synchronized( syncObj) {
        	GroupMappingTableEntry entry = groupMappingTable.get(streamName);
        	if ( entry != null ) {
            	unreserveStreamId(entry.streamId);
            	unreserveMixerId(entry.mixerId);
            	groupMappingTable.remove(streamName);
            	streamId = entry.streamId;
            	totalInputStreams--;
        		log.info("A old stream id: {}, mixer id: {} is deleted on thread: {}", entry.streamId, entry.mixerId, Thread.currentThread().getName());
        	}
		}
    	return streamId;
	}
}
