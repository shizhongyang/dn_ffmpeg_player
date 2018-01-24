package com.dongnaoedu.dnffmpegplayer;

import java.io.File;

import com.dongnaoedu.dnffmpegplayer.view.VideoView;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.view.Surface;
import android.view.View;
import android.view.View.OnClickListener;

public class MainActivity extends Activity {
	private MyPalyer player;
	private VideoPlayer videoPlayer;
	private VideoView videoView;
	private JasonPlayer jasonPlayer;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		player = new MyPalyer();
		videoPlayer = new VideoPlayer();
		jasonPlayer = new JasonPlayer();
		videoView = (VideoView) findViewById(R.id.video_view);

		findViewById(R.id.btn1).setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
				// String video = sp_video.getSelectedItem().toString();
				String input = new File(Environment.getExternalStorageDirectory(), "input.mp4").getAbsolutePath();
				// Surface传入到Native函数中，用于绘制
				Surface surface = videoView.getHolder().getSurface();
				videoPlayer.render(input, surface);
			}
		});
	}

	public void mDecode(View btn) {
		switch (btn.getId()) {
		case R.id.btn2:
			String input1 = new File(Environment.getExternalStorageDirectory(),"13.mp4").getAbsolutePath();
			//Surface传入到Native函数中，用于绘制
			Surface surface = videoView.getHolder().getSurface();
			jasonPlayer.play(input1,surface);
			break;
		case R.id.btn3:
			String input = new File(Environment.getExternalStorageDirectory(), "1.mp4").getAbsolutePath();
			String output = new File(Environment.getExternalStorageDirectory(), "output_yuv420p.yuv").getAbsolutePath();
			VideoUtils.decode(input, output);
			break;
		default:
			break;
		}

	}

}
