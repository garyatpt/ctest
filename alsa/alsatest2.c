/*--------------------------------------------------
ALSA record and play test
Quote from: http://blog.csdn.net/ljclx1748/article/details/8606831

1. use alsamixer to adjust Capture and ADC PCM value
2. some explanation:
	sample: usually 8bits or 16bits, one sample data width.
	channel: 1-Mono. 2-Stereo
	frame: sizeof(one sample)*channels
	rate: frames per second
	period: Max. frame numbers hard ware can be handled each time.
	chunk: frames receive from/send to hard ware each time.
	buffer: N*periods
	interleaved mode:record period data frame by frame, such as  frame1(Left sample,Right sample),frame2(), ......
	uninterleaved mode: record period data channel by channel, such as period(Left sample,Left ,left...),period(right,right...),period()...
--------------------------------------------------*/

#include <asoundlib.h>
#include <stdbool.h>

snd_pcm_t *pcm_handle;
char *wave_buf;
int wave_buf_len;

bool device_open( int mode);
bool device_setparams();
bool device_capture( int dtime );
bool device_play();
char *wave_buf;
int wave_buf_len; //--buf length for storing n-second sound data
int bit_per_sample;
snd_pcm_uframes_t frames;
snd_pcm_uframes_t period_size; //length of period (frames)
int chunk_byte; //length of period (bytes)
snd_pcm_uframes_t chunk_size=32;//numbers of frames read/write to hard ware each time
unsigned int chanl_val,rate_val;
int dir;
snd_pcm_format_t format_val;
snd_pcm_hw_params_t *params;


int main(int argc,char* argv[]){
int fd;
int rc;
printf("-----------start------------\n");

//录音
if (!device_open(SND_PCM_STREAM_CAPTURE )) printf("---device_open()------\n");//return 1;
if (!device_setparams(1,8000)) printf("-----device_setparams()-----\n");//return 2;
if (!device_capture( 5 )) printf("-----device_capture()-----\n");//return 3; //录制3秒
snd_pcm_close( pcm_handle ); printf("=========== record finish!! snd_pcm_close() =====\n\n\n");

fd=open("/tmp/record.raw",O_WRONLY|O_CREAT|O_TRUNC);
rc=write(fd,wave_buf,wave_buf_len);
printf("------write to record.raw  %d bytes --------\n",rc);

//播放
if (!device_open(SND_PCM_STREAM_PLAYBACK)) return 4;
printf("-----PLAY: device_open() finish ----\n");
if (!device_setparams(1,8000)) return 5;
printf("-----PLAY: device_setarams() finish ----\n");
if (!device_play()) return 6; //-------------------------------------------------
printf("-----PLAY: device_play() finish ----\n");
snd_pcm_close( pcm_handle );
printf("-----PLAY: snd_pcm_close() finish ----\n");

free(wave_buf);

return 0;

}


//首先让我们封装一个打开音频设备的函数：
//snd_pcm_t *pcm_handle;

bool device_open(int mode){
if(snd_pcm_open (&pcm_handle,"default",mode,0) < 0)
 {
  printf("--- snd_pcm_open() fail! ----\n");
  return false; 
 }
  printf("--- snd_pcm_open() succeed! ----\n");
  return true;
  }


int bit_per_sample; //样本长度(bit)
//snd_pcm_uframes_t period_size; //周期长度(桢数)
int chunk_byte; //周期长度(字节数)
snd_pcm_hw_params_t *params; //定义参数变量


bool device_setparams(int nchanl,int rate)
 {
unsigned int val;
int dir;
int rc;

snd_pcm_hw_params_t *hw_params;
if(snd_pcm_hw_params_malloc (&hw_params) >=0) //为参数变量分配空间
	printf("---- snd_pcm_hw_params_malloc(&hw_params) ----\n");
if(snd_pcm_hw_params_malloc (&params) >= 0)
	printf("----snd_pcm_hw_params_malloc(&params) ----\n");
if(snd_pcm_hw_params_any (pcm_handle, hw_params) >= 0) //参数初始化
	printf("----snd_pcm_hw_params_any(pcm_handle,hw_params)----\n");
if(snd_pcm_hw_params_set_access (pcm_handle, hw_params,SND_PCM_ACCESS_RW_INTERLEAVED) >= 0) //设置为交错模式
	printf("----snd_pcm_hw_params_set_access()----\n");
if(snd_pcm_hw_params_set_format( pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE) >= 0) //使用用16位样本
	printf("----snd_pmc_hw_params_set_format()-----\n");
//snd_pcm_hw_params_set_rate_near( pcm_handle, hw_params,48000, 0); //设置采样率为44.1KHz
val=rate;//8000;
if(snd_pcm_hw_params_set_rate_near( pcm_handle, hw_params,&val,0) >= 0) //设置采样率为44.1KHz
	printf("----snd_pcm_hw_params_set_rate_near() val=%d----\n",val);
if(snd_pcm_hw_params_set_channels( pcm_handle, hw_params, nchanl)>=0) //设置为立体声
	printf("----snd_pcm_hw_params_set_channels()-----\n");
frames=32;
if(snd_pcm_hw_params_set_period_size_near(pcm_handle,hw_params,&chunk_size,&dir)>=0)
	printf("----snd_pcm_hw_params_set_period_size_near() chunk_size=%d----\n",chunk_size);
if(snd_pcm_hw_params_get_period_size( hw_params, &period_size,0)>=0) //获取周期长度
	printf("----snd_pcm_hw_get_period_size(): %d----\n",(int)period_size);
if(snd_pcm_hw_params_get_format(hw_params,&format_val)>=0)
	printf("----snd_pcm_hw_params_get_format()----\n");
bit_per_sample = snd_pcm_format_width((snd_pcm_format_t)format_val);
printf("---bit_per_sample=%d  snd_pcm_format_width()----\n",bit_per_sample);
//获取样本长度
snd_pcm_hw_params_get_channels(hw_params,&chanl_val);
printf("----snd_pcm_hw_params_get_channels %d---\n",chanl_val);
chunk_byte = period_size*bit_per_sample*chanl_val/8;
//chunk_size = frames;//period_size; //frames
//计算周期长度（字节数(bytes) = 每周期的桢数 * 样本长度(bit) * 通道数 / 8 ）
rc=snd_pcm_hw_params( pcm_handle, hw_params); //设置参数
if(rc<0){
printf("unable to set hw parameters:%s\n",snd_strerror(rc));
exit(1);
}
printf("---snd_pcm_hw_params()-----\n");
params = hw_params; //保存参数，方便以后使用
snd_pcm_hw_params_free( hw_params); //释放参数变量空间
printf("----snd_pcm_hw_params_free()----\n");
return true;

}

//这里先使用了Alsa提供的一系列snd_pcm_hw_params_set_函数为参数变量赋值。
//最后才通过snd_pcm_hw_params将参数传递给设备。
//需要说明的是正式的开发中需要处理参数设置失败的情况，这里仅做为示例程序而未作考虑。
//设置好参数后便可以开始录音了。录音过程实际上就是从音频设备中读取数据信息并保存。

// char *wave_buf;
// int wave_buf_len;
 bool device_capture( int dtime ){
//　wave_buf_len = dtime*params.rate*bit_per_sample*params.channels/8;
snd_pcm_hw_params_get_rate(params,&rate_val,&dir); //params=hw_params
wave_buf_len=dtime*rate_val*bit_per_sample*chanl_val/8;
printf(" wave_buf_len=%d \n",wave_buf_len);
   //计算音频数据长度（秒数 * 采样率 * 桢长度）
 char *data=wave_buf=(char*)malloc(wave_buf_len);
  int r = 0;
  chunk_size=32;
  chunk_byte=chunk_size*bit_per_sample*chanl_val/8;
  printf("chunk_byte=%d\n",chunk_byte);
  while ( (data-wave_buf) <= (wave_buf_len-chunk_byte) ){ //chunk_size*bit_per_sample*chanl_val)){
	r = snd_pcm_readi( pcm_handle,data,chunk_size);  //chunk_size*bit_per_sample*read interleaved rames from a PCM
// snd_pcm_sframes_t snd_pcm_readi(snd_pcm_t *pcm,void* buffer, snd_pcm_uframes_t size)
// pcm -PCM handle    buffer-frames containing buffer  size - frames to be read
	if ( r>0 ) {
		printf(" r= %d\n",r);
		printf("data=%d  wave_buf=%d\n",data,wave_buf);
		data += chunk_byte;//--move buffer current position pointer
	}
	else
		return false;
}

return true;

}

//形参dtime用来确定录音时间，根据录音时间分配数据空间，再调用snd_pcm_readi从音频设备读取音频数据，存放到wave_buf中。
//同样的原理，我们再添加一个播放函数，向音频设备写入数据：

bool device_play(){

  char *data = wave_buf;
  int r = 0;
  chunk_size=32;
  chunk_byte=chunk_size*bit_per_sample*chanl_val/8;
  while ( (data-wave_buf) <= (wave_buf_len-chunk_byte)){
  r = snd_pcm_writei( pcm_handle, data , chunk_size); //chunk_size = frames
  if(r == -EAGAIN)continue;
  if(r < 0){
	printf("wirte error: %s\n",snd_strerror(r));
	exit(EXIT_FAILURE);
   }
  //printf("----- writei()  r=%d -----\n ",r);
  if ( r>0 ) data += chunk_byte;
  else
  return false;
  }
  return true;

}
