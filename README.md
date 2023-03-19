## Filtering by DFT (time domain)
Remove two tones and signals above 20kHz.
### Dataset
1. Ascience-Fast-Piano.wav : music without noise  
2. Ascience-Fast-Piano-Add-Tones.wav : music polluted by 2 tones(one tone frequency > 15000Hz, one tone frequency < 100Hz.) *Hint :  In about 0.2 seconds of the beginning of the music, there is only noise of two tones.  

### 1. Header
```js
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
```
#### Wav file header format :
(bits)  
0-3   : ChunkID 識別碼"RIFF"  
4-7   : ChunkSize 檔案大小-8(bytes)  
8-11  : Format "WAVE"字樣  
12-15 : Subchunk1ID "fmt"字樣  
16-19 : Subchunk1Size 固定為16  
20-21 : AudioFormat 1(表無壓縮)  
22-23 : NumChannels Mono=1, Stero=2  
24-27 : SampleRate 每秒取樣次數  
28-31 : ByteRate 每秒資料量(in byte)  
32-33 : BlockAlign 每次取樣所產生的byte數  
34-35 : BitsPerSample 8或16  
36-39 : Subchunk2ID "data"字樣  
40-43 : Subchunk2Size 音訊資料大小(in bytes)  
after 44 : data 音訊資料  

### 2. Read data
1.  讀取header，計算sample的總數量N  
2.  fread讀取data  
```js
fp_WavIn = fopen(argv[1],"rb");

if(fp_WavIn == NULL) {
   printf("Cannot read WavIn.\n", fp_WavIn);
   return 0;
}
		
fp_WavOut = fopen(argv[2],"wb");
	
fread(&wavheader, 44, 1, fp_WavIn); //將 header讀出來存放到 wavheader這個變數裡
N = wavheader.Subchunk2Size / (wavheader.BlockAlign)* wavheader.NumChannels; //計算sample的總數量N
short * data = malloc(sizeof(short)*N);

fread(data,2,N,fp_WavIn); //讀data 
```    

### 3. Remove tones
1.  取前0.1s的資料存進陣列tones[ ]，前0.1s共有9600個點(一個聲道4800個點，雙聲道*2)
2.  音檔的資料每9600個點刪除一次tones，也就是每0.1s刪一次
```js
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
```

### 4. LPF
1.  在time domain做LPF
2.  rect(f)經由IDFT轉換會變成sinc(t)
3.  音檔的data跟取hamming的sinc做convolution
4.  輸出的資料要轉回short
#### (1) Some define
```js
float hamming(int N, int n){
	return 0.54 - 0.46*cos(2*PI*(float)n/(float)N);
}
```
```js
float low_pass(int m, int n){
	float wc = 2*PI*FC/FS;
  if(n == m){
    return wc/PI;//sinc分母等於0時 
  }
  else{
      return sinf(wc*((float)(n-m)))/PI/((float)(n-m))*hamming(2*m+1,n);//sinc取 hamming 
  }
}
```
#### (2) LPF  
```js
for(n=0;n<(2*M+1);n++){
    h[n] = low_pass(M,n);
}
```
#### (3) Convolution
```js
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
```

### 5. Write data
1.  先寫入header，再寫入音訊資料  
2.  close file  
```js
fwrite(&wavheader, sizeof(wavheader), 1, fp_WavOut); //將wavHeader這個變數的內容寫進去檔案header部分
for(i=0;i<N;i++){	// 將音訊資料內容寫進去wav檔案
   fwrite(output+i, sizeof(short), 1, fp_WavOut);
   }
   
   free(data);
   free(output);
   fclose(fp_WavOut);
   
   return 0;
```







