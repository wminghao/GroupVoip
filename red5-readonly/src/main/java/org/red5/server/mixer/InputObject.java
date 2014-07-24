package org.red5.server.mixer;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import org.apache.mina.core.buffer.IoBuffer;
import org.red5.logging.Red5LoggerFactory;
import org.red5.server.api.Red5;
import org.slf4j.Logger;

public class InputObject {
	private IdLookup idLookupTable;
	public String streamName;
	public int msgType;
	public ByteBuffer flvFrame;
	public ByteBuffer flvSegment;
	public int eventTime;
	private static Logger log = Red5LoggerFactory.getLogger(Red5.class);

	private static final int kDesktopStreamSource = 1; //desktop don't need a separate audio mixing
	private static final int kMobileStreamSource = 2; //mobile stream needs a separate audio mixing
	
	public InputObject(IdLookup idLookupTable, String streamName, int msgType, IoBuffer buf, int eventTime, int dataLen) {
		this.idLookupTable = idLookupTable;
		this.streamName = streamName;
		this.msgType = msgType;

	    //see example from BufferUtils.class, TODO why???
		flvFrame = ByteBuffer.allocate(dataLen);
		flvFrame.order(ByteOrder.LITTLE_ENDIAN);  // to use little endian
		byte[] inBuf = new byte[dataLen];
		buf.get(inBuf);
		buf.flip();
		this.flvFrame.put(inBuf, 0, dataLen);
		this.flvFrame.flip();
		this.eventTime = eventTime;
		this.flvSegment = null;
	}
	
	public ByteBuffer toByteBuffer()
	{
		if( this.flvSegment != null ) {
			return this.flvSegment;
		} else {
        	int result[] = new int[2];
        	int mixerId = idLookupTable.lookupMixerIdAndMask(streamName, result);
        	if ( mixerId != -1 ) {     
        		int dataLen = flvFrame.limit();   	
        		int flvFrameLen = 11 + dataLen + 4;
        		int segHeaderLen = 8 + 6*(result[1]-1); //additional headers, excluding all-in-one message
        		int totalLen = flvFrameLen+segHeaderLen;
        		flvSegment = ByteBuffer.allocate(totalLen); //TODO direct?
        		flvSegment.order(ByteOrder.LITTLE_ENDIAN);  // to use little endian
        		flvSegment.put((byte)'S');
        		flvSegment.put((byte)'G');
        		flvSegment.put((byte)'I');
        		flvSegment.put((byte)0); //even layout
        		flvSegment.putInt(result[0]); //calc mask here
        		
    			flvSegment.put((byte)((mixerId<<3) | kDesktopStreamSource));// TODO assume all desktop channels
    			flvSegment.put((byte)0); //ignore for now
    			flvSegment.putInt(flvFrameLen);
    
    			//the actual flv frame
    			flvSegment.put((byte)msgType); //audio type
    			flvSegment.put((byte)((dataLen>>16)&0xff));//datalen
    			flvSegment.put((byte)((dataLen>>8)&0xff));//datalen
    			flvSegment.put((byte)( dataLen&0xff));//datalen
        
    			flvSegment.put((byte)((eventTime>>16)&0xff));//ts
    			flvSegment.put((byte)((eventTime>>8)&0xff));//ts
    			flvSegment.put((byte)( eventTime&0xff));//ts
    			flvSegment.put((byte)((eventTime>>24)&0xff));//ts
        
    			flvSegment.put((byte)0x0); //streamId, ignore
    			flvSegment.put((byte)0x0); //streamId, ignore
    			flvSegment.put((byte)0x0); //streamId, ignore
    
    			flvSegment.put(flvFrame);
    			flvSegment.putInt(0);//prevSize, ignore
        		
    			//the rest of segments
        		for (int i = 0; i < IdLookup.MAX_STREAM_COUNT; i++) {
        			int iMaskedVal = 1<<i;
        			if (i != mixerId && ( (result[0] & iMaskedVal) == iMaskedVal )) {
        				byte idtype = (byte)((i<<3) | kDesktopStreamSource); // TODO assume all desktop channels
        				flvSegment.put(idtype);
        				flvSegment.put((byte)0); //ignore for now
        				flvSegment.putInt(0); //no data for this stream
        			}
        		}
        		flvSegment.flip();
        		//log.info("=====>in message from {} mixerid {} type {} ts {} len {} totalLen {} on thread: {}", streamName, mixerId, (msgType==0x09)?"video":"audio", eventTime, dataLen, totalLen, Thread.currentThread().getName());
        		return flvSegment;
        	}
		}
    	return null;
	}
}