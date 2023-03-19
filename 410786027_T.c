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
    short AudioFormat;//�@�묰1 ��ܬ�PCM�s�X
    short NumChannels;//�n�D�ƶq
    int SampleRate;//�ļ˲v
    int ByteRate;//�ļ˲v
    short BlockAlign;//�C��block�������j�p
    short BitsPerSample;//�C��ļˤ�S�v
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
//�b time domain��sinc��low pass 
float low_pass(int m, int n){
	float wc = 2*PI*FC/FS;
	
	if(n == m){
		return wc/PI;//sinc��������0�� 
	}
	else{
		return sinf(wc*((float)(n-m)))/PI/((float)(n-m))*hamming(2*m+1,n);//sinc�� hamming 
	}
}


int main(int argc, char **argv){

    _setmode( _fileno( stdout ), _O_BINARY );//��stdout�� binary mode
    _setmode( _fileno( stderr ), _O_BINARY );//��stderr�� binary mode

    int N;//�ŧi�s�� sample�`�Ӽƪ��ܼ�N
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
	
    fread(&wavheader, 44, 1, fp_WavIn); //�N headerŪ�X�Ӧs��� wavheader�o���ܼƸ�
	
    N = wavheader.Subchunk2Size / (wavheader.BlockAlign)* wavheader.NumChannels; //�p��sample���`�ƶqN
	
	short *data = malloc(sizeof(short)*N);
    fread(data,2,N,fp_WavIn); //Ū data 
	
//delete tones
	//���e0.1��tones �C0.1��R�@��
    short *tones = malloc(sizeof(short)*9600);

	for(i=0;i<9600;i++){     //�e��0.1��@��4800*2���I
        tones[i] = data[i];
	}

    for(i=0;i<N/9600;i++){  //�C0.1��R�@��tone
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
	fwrite(&wavheader, sizeof(wavheader), 1, fp_WavOut); //�NwavHeader�o���ܼƪ����e�g�i�h�ɮ�header����
	
	for(i=0;i<N;i++){	// �N���T��Ƥ��e�g�i�hwav�ɮ�
		fwrite(output+i, sizeof(short), 1, fp_WavOut);
	}
	free(data);
	free(output);
  
	fclose(fp_WavOut);

	return 0;

}

