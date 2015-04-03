package buwai.android.shell.advmptest;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;

public class MainActivity extends Activity {
	
	public static final String TAG = "debug";
	
	private Button mbtnTest;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		nativeLog();
		
		mbtnTest = (Button) findViewById(R.id.btnTest);
		mbtnTest.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View v) {
				int result = separatorTest(2);
				Log.i(TAG, "separatorTest result:" + result);
			}
		});
	}
	
	private native int separatorTest(int value);
	
	private native static void nativeLog();
	
	static {
		System.loadLibrary("advmpc");
	}
	
}
