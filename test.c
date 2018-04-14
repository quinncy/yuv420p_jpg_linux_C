#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include "yuv2jpg.h"

int get_Y_U_V(unsigned char*rData,unsigned char* in_Y,unsigned char* in_U,unsigned char* in_V,int nStride,int height)
{
	int i = 0;
	int y_n =0;
	int u_n =0;
	int v_n =0;
	int u = 0;
	int v = 2;
	long size = nStride*height*3/2;
	//yuv422格式读取
	#if 0
	while(i<size){
		if(i%2 != 0){
			in_Y[y_n]= rData[i];
			y_n++;		
		}
		else if(i == u){
			in_U[u_n]= rData[i];
			u += 4;	
			u_n++;	
		}
		else if(i == v){
			in_V[v_n] = rData[i];
			v += 4;
			v_n++;
		}
		i++;
	}
	#endif
	//使用yuv420格式读取yu不连续放置
	#if 1
	while(i < 1280*720)
	{
		in_Y[y_n] = rData[i];
		i++;
		y_n++;
	}
	while(i < 1280*720*5/4)
	{
		in_U[u_n] = rData[i];
		i++;
		u_n++;
	}
	while(i < 1280*720*3/2)
	{
		in_V[v_n] = rData[i];
		i++;
		v_n++;
	
	}
	#endif

	//使用yuv420格式读取yu连续放置
	#if 0
	while(i < 1280*720)
	{
		in_Y[y_n] = rData[i];
		i++;
		y_n++;
	}

	while(i < 1280*720*3/2)
	{
		if(i % 2 == 0)	//u格式数据
		{
			in_U[u_n] = rData[i];
			u_n++;
		}
		else
		{
			in_V[v_n] = rData[i];
			v_n++;
		}
		i++;
	}
	#endif
	return 0;
}

int main()
{
	unsigned char* in_Y = (unsigned char*)malloc(1280*720);//
	unsigned char* in_U = (unsigned char*)malloc(1280* 720/4);//
	unsigned char* in_V = (unsigned char*)malloc(1280* 720 / 4);//
	unsigned char* pData = (unsigned char*)malloc(1280 * 720);//
	unsigned char* rData = (unsigned char*)malloc(1280*720*3/2);
	
	unsigned long dwSize = 0;
	FILE *rfp = fopen("./00001.yuv","rb");
	//FILE *rfp = fopen("/home/zhangqi/00001.yuv","rb");
	if(NULL == rfp)
		fprintf(stderr,"fopen fp error:%s\n",strerror(errno));
	fread(rData,1280*720*3/2,1,rfp);
	get_Y_U_V(rData,in_Y,in_U,in_V,1280,720);
	free(rData);

	YUV2Jpg(in_Y,in_U,in_V,1280,720,100,1280,pData,&dwSize);
	FILE *fp = fopen("2.jpg","wb");
	fwrite(pData,dwSize,1,fp);
	fclose(fp);
		
	free(in_Y);
	free(in_U);
	free(in_V);
	free(pData);

	return 0;
}
