package org.red5.server.mixer;

import java.nio.ByteBuffer;

public class KaraokeGenerator {
	private Delegate delegate_;
	private String karaokeFilePath_;
	
    public interface Delegate {
        public void onKaraokeFrameParsed(ByteBuffer frame, int len);
    }

	public KaraokeGenerator(KaraokeGenerator.Delegate delegate, String karaokeFilePath){
		this.delegate_ = delegate;
		this.karaokeFilePath_ = karaokeFilePath;
	}
}
