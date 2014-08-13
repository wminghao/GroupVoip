import java.nio.ByteBuffer;
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.Vector;

//Tool to map timestamp back to 0
//
//The way to cut a flv file: 
// 1. Cut a portion of file out, starting from a keyframe.
//    flvtool2 -C -i 74580 -o 340000 abc_32.flv
// 2. Use hexfiend to inject the metadata into the file, modify the timestamp to be the same as the first non-metadata frame
// 3. Use FLVTimestampMapper to run through the file to map the timestamp.

public class FLVTimestampMapper {
    private static final int SCAN_HEADER_TAG = 0;
    private static final int SCAN_REMAINING_TAG = 1;
    private int scanState_ = SCAN_HEADER_TAG;
    ByteBuffer curBuf_ = ByteBuffer.allocate(1<<20); //1 Meg of memory
    byte[] dataSizeStr_ = new byte[4];
    private int curFlvTagSize_;
    private int curFlvTagTimestamp_;
    private int curLen_ = 0;
    private int adjustPTS_ = 0;
    private BufferedOutputStream outputFile_ = null;

    public FLVTimestampMapper(BufferedOutputStream output) {
    	outputFile_ = output;
    }
    
    public void readData(byte[] src, int srcLen) {
    	int srcIndex = 0; //index into data byte array
	while( srcLen > 0 ) {
	    switch( scanState_ ) {
	    case SCAN_HEADER_TAG:
		{
		    if ( curLen_ < 11 ) {
                        int cpLen = Math.min(srcLen, 11-curLen_);
                        curBuf_.put(src, srcIndex, cpLen); //concatenate the string                                                                                                                 
                        srcLen -= cpLen;
                        srcIndex += cpLen; //advance
                        curLen_ += cpLen;
                    }
		    if ( curLen_ >= 11 ) {
			curFlvTagSize_ = (curBuf_.array()[1] & 0xFF) << 16 |
			    (curBuf_.array()[2] & 0xFF) << 8  |
			    (curBuf_.array()[3] & 0xFF) ;
			curFlvTagSize_ += 4; //add previousTagLen
	                
			curFlvTagTimestamp_ = (curBuf_.array()[4] & 0xFF) << 16 |
			    (curBuf_.array()[5] & 0xFF) << 8  |
			    (curBuf_.array()[6] & 0xFF);

			//adjust the timestamp and write it back
			if( adjustPTS_ == 0 ) {
			    adjustPTS_ = curFlvTagTimestamp_;
			}
			curFlvTagTimestamp_ -= adjustPTS_;
			curBuf_.array()[4] = (byte)((curFlvTagTimestamp_ >> 16) & 0xff);
			curBuf_.array()[5] = (byte)((curFlvTagTimestamp_ >> 8) & 0xff);
			curBuf_.array()[6] = (byte)( curFlvTagTimestamp_ & 0xff);
	                
			scanState_ = SCAN_REMAINING_TAG;
                    	System.out.println("---got the frame info: first byte="+ curBuf_.array()[0] +" curFlvTagTimestamp_="+ curFlvTagTimestamp_ +" curFlvTagSize_="+curFlvTagSize_);
		    }
		    break;
		}
	    case SCAN_REMAINING_TAG:
		{
                    if ( curLen_ < (curFlvTagSize_+11) ) {
                        int cpLen = Math.min(srcLen, curFlvTagSize_+11-curLen_);
                        curBuf_.put(src, srcIndex, cpLen); //concatenate the string     
                        
                        srcLen -= cpLen;
                        srcIndex += cpLen;
                        curLen_+=cpLen;
                    }
                    if ( curLen_ >= (curFlvTagSize_+11) ) {
                    	System.out.println("---read the frame, curFlvTagTimestamp_="+ curFlvTagTimestamp_ +" curFlvTagSize_="+ curFlvTagSize_);
                    	//read the actual buffer
                        if( curFlvTagSize_ > 0 ) {
			    curBuf_.flip();     
			    try {
				//write it to a file
				outputFile_.write(curBuf_.array(), 0, curFlvTagSize_);                 	
			    } catch (IOException ex) {
				System.out.println("IOxception: "+ex);
			    }
                        }
			curBuf_.clear();
			curLen_ = 0;
			curFlvTagSize_  = 0; //reset and go to the first state
			scanState_ = SCAN_HEADER_TAG;
		    }
		    break;
		}
	    }
	}
    }

    // method main(): ALWAYS the APPLICATION entry point
    public static void main (String[] args) {
	System.out.println ("FLVTimestampMapper!");
	if( args.length != 3 ) {
	    return;
	}
	String inputFilePath = args[1];
	String outputFilePath = args[2];
	try {
	    InputStream input = null;
	    BufferedOutputStream out = null;
	    try {
		File file = new File(inputFilePath);
	        out = new BufferedOutputStream(new FileOutputStream(outputFilePath));
		FLVTimestampMapper flvParser = new FLVTimestampMapper(out);
		System.out.println("File input size: "+file.length());

		byte[] result = new byte[4096];
		input = new BufferedInputStream(new FileInputStream(file));
		int fileLen = (int) file.length();

		//first write the header
		input.read(result, 0, 13);
		out.write(result, 0, 13);
		int bytesTotal = 13;

		while( bytesTotal < fileLen ) {
		    int bytesToRead = 0;
		    while(bytesToRead < result.length && (bytesToRead+bytesTotal)<fileLen){
			int bytesRemaining = result.length - bytesToRead;
			//input.read() returns -1, 0, or more :
			int bytesRead = input.read(result, bytesToRead, bytesRemaining);
			if (bytesRead > 0){
			    bytesToRead += bytesRead;
			}
		    }
		    flvParser.readData(result, bytesToRead); //send to flv parser
		    bytesTotal += bytesToRead;
		    		    
		    System.out.println("Total bytes read: "+bytesTotal+", len "+fileLen);
		}
	    } catch (IOException ex) {
		System.out.println("IOxception: "+ex);
	    } catch (Exception ex) {
		System.out.println("General exception: "+ex);
	    } finally {
		System.out.println("Closing input stream.");
		input.close();
	    }
	    out.close();
	}
	catch (FileNotFoundException ex) {
	    System.out.println("File not found: " + ex);
	}
	catch (IOException ex) {
	    System.out.println("Other exception: " + ex);
	}
    }
}
