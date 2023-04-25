#include<stdio.h>
#include<math.h>
#include<string.h>
#include<stdlib.h>
#include<fcntl.h>
#include<io.h>
#include<inttypes.h>
#include<memory.h>

#define pi 3.14159265
#define PI 3.14159265

#define FS 96000.0f
#define FC 20000.0f
#define M 500


typedef struct{
	
    //RIFF
    char ChunkID[4];//RIFF
    int ChunkSize;
    char Format[4];
    //format
    char Subchunk1ID[4];//fmt
    int Subchunk1Size;
    short AudioFormat;//一般為1 表示為PCM編碼
    short NumChannels;//聲道數量
    int SampleRate;//採樣率
    int ByteRate;//採樣率
    short BlockAlign;//每個block的平均大小
    short BitsPerSample;//每秒採樣比特率
    //data
    char Subchunk2ID[4];//data
    int Subchunk2Size;
    
} wav_header;
wav_header wavheader;

//hamming
float hamming(int N, int n){
	return 0.54 - 0.46*cos(2*PI*(float)n/(float)N);
}

//low pass
//在 time domain用sinc做low pass 
float low_pass(int m, int n){
	float wc = 2*PI*FC/FS;
	
	if(n == m){
		return wc/PI;//sinc分母等於0時 
	}
	else{
		return sinf(wc*((float)(n-m)))/PI/((float)(n-m))*hamming(2*m+1,n);//sinc取 hamming 
	}
}


int main(int argc, char **argv){

    _setmode( _fileno( stdout ), _O_BINARY );//使stdout有 binary mode
    _setmode( _fileno( stderr ), _O_BINARY );//使stderr有 binary mode

    int N;//宣告存放 sample總個數的變數N
    int i=0,j=0,k=0;
    int n=0;
    float h[2*M+1] = {0};

    FILE *fp_WavIn;
    FILE *fp_WavOut;

//read data
	fp_WavIn = fopen(argv[1],"rb");
	
	if(fp_WavIn == NULL) {
		printf("Cannot read WavIn.\n", fp_WavIn);
		return 0;
	}
		
	fp_WavOut = fopen(argv[2],"wb");
	
    fread(&wavheader, 44, 1, fp_WavIn); //將 header讀出來存放到 wavheader這個變數裡
	
    N = wavheader.Subchunk2Size / (wavheader.BlockAlign)* wavheader.NumChannels; //計算sample的總數量N
	
	short *data = malloc(sizeof(short)*N);
    fread(data,2,N,fp_WavIn); //讀 data 
	
//delete tones
	//取前0.1秒的tones 每0.1秒刪一次
    short *tones = malloc(sizeof(short)*9600);

	for(i=0;i<9600;i++){     //前面0.1秒共有4800*2個點
        tones[i] = data[i];
	}

    for(i=0;i<N/9600;i++){  //每0.1秒刪一次tone
        for(j=0;j<9600;j++){
            data[i*9600+j]  = data[i*9600+j] - tones[j];
        }
    }
    free(tones);
    fclose(fp_WavIn);

//LPF
	for(n=0;n<(2*M+1);n++){
		h[n] = low_pass(M,n);
	}	
		
//convolution
	short *output = malloc(sizeof(short)*N);
	float tmp;
	for(i=0;i<N;i++){
		tmp = 0.0;
		for(j=0;j<(2*M+1);j++){
			if((i-j)>=0){ 
				tmp = tmp + h[j]*((float)(data[i-j]));
			}	 
		}
		output[i] = (short)(roundf(tmp));
	}

//write
	fwrite(&wavheader, sizeof(wavheader), 1, fp_WavOut); //將wavHeader這個變數的內容寫進去檔案header部分
	
	for(i=0;i<N;i++){	// 將音訊資料內容寫進去wav檔案
		fwrite(output+i, sizeof(short), 1, fp_WavOut);
	}
	free(data);
	free(output);
  
	fclose(fp_WavOut);

	return 0;

}

