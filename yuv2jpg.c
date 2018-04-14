#include "yuv2jpg.h"
#include <stdio.h>
#include <math.h>

void ProcessUV(unsigned char* pUVBuf,unsigned char* pTmpUVBuf,int width,int height,int nStride)
{
	//原来程序
	#if 0
	int i=0;
	while(i<nStride*height){
			pUVBuf[i] = pTmpUVBuf[i/2];
			i++;
	}
	#endif
	
	int i,j;
	int count = 0;
	for(i = 0; i < height; i+=2)
	{
		for(j = 0; j < nStride; j+=2)
		{
			pUVBuf[i * width + j] = pTmpUVBuf[count];
			pUVBuf[i * width + j + 1] = pTmpUVBuf[count];
			pUVBuf[i * width + j + width] = pTmpUVBuf[count];
			pUVBuf[i * width + j + width + 1] = pTmpUVBuf[count];	
			count++;
		}
	}
}

int QualityScaling(int quality)
{
if (quality <= 0) quality = 1;
  if (quality > 100) quality = 100;
  if (quality < 50)
    quality = 5000 / quality;
  else
    quality = 200 - quality*2;

  return quality;
}

void DivBuff(unsigned char* pBuf,int width,int height,int nStride,int xLen,int yLen)
{
	int xBufs = width/xLen;
	int	yBufs= height/yLen; 
	int tmpBufLen = xBufs * xLen * yLen;   
	unsigned char* tmpBuf  = (unsigned char*)malloc(tmpBufLen);
	int i;              
	int j;
	int k; 
	int n;
	int bufOffset  = 0;
	for (i = 0; i < yBufs; ++i) 
	{
		n = 0;  
		for (j = 0; j < xBufs; ++j) 
		{   
			bufOffset = yLen * i * nStride + j * xLen; 
			for (k = 0; k < yLen; ++k) 
			{
				memcpy(tmpBuf + n,pBuf +bufOffset,xLen);       
				n += xLen;            
				bufOffset += nStride; 
			}
		}
		memcpy(pBuf +i * tmpBufLen,tmpBuf,tmpBufLen);
	} 
	free(tmpBuf); 
}

void SetQuantTable(unsigned char* std_QT,unsigned char* QT, int Q)
{
	int tmpVal = 0;
	int	i;
	for (i = 0; i < DCTBLOCKSIZE; ++i)
	{
		tmpVal = (std_QT[i] * Q + 50L) / 100L;
		
		if (tmpVal < 1)           
		{
			tmpVal = 1L;
		}
		if (tmpVal > 255)
		{
			tmpVal = 255L;
		}
		QT[FZBT[i]] = (unsigned char)tmpVal;
	} 
}

void InitQTForAANDCT(JPEGINFO *pJpgInfo)
{
	unsigned int i = 0;         
	unsigned int j = 0;
	unsigned int k = 0; 
	
	for (i = 0; i < DCTSIZE; i++)  
	{
		for (j = 0; j < DCTSIZE; j++)
		{
			pJpgInfo->YQT_DCT[k] = (float) (1.0 / ((double) pJpgInfo->YQT[FZBT[k]] *
				aanScaleFactor[i] * aanScaleFactor[j] * 8.0));       
			++k;
		}
	} 
	
	k = 0;
	for (i = 0; i < DCTSIZE; i++)  
	{
		for (j = 0; j < DCTSIZE; j++)
		{
			pJpgInfo->UVQT_DCT[k] = (float) (1.0 / ((double) pJpgInfo->UVQT[FZBT[k]] *
				aanScaleFactor[i] * aanScaleFactor[j] * 8.0));       
			++k;
		}
	} 
}

unsigned char ComputeVLI(short val)
{ 
	unsigned char binStrLen = 0;
	val = fabs(val); 

	if(val == 1)
	{
		binStrLen = 1;  
	}
	else if(val >= 2 && val <= 3)
	{
		binStrLen = 2;
	}
	else if(val >= 4 && val <= 7)
	{
		binStrLen = 3;
	}
	else if(val >= 8 && val <= 15)
	{
		binStrLen = 4;
	}
	else if(val >= 16 && val <= 31)
	{
		binStrLen = 5;
	}
	else if(val >= 32 && val <= 63)
	{
		binStrLen = 6;
	}
	else if(val >= 64 && val <= 127)
	{
		binStrLen = 7;
	}
	else if(val >= 128 && val <= 255)
	{
		binStrLen = 8;
	}
	else if(val >= 256 && val <= 511)
	{
		binStrLen = 9;
	}
	else if(val >= 512 && val <= 1023)
	{
		binStrLen = 10;
	}
	else if(val >= 1024 && val <= 2047)
	{
		binStrLen = 11;
	}
	
	return binStrLen;
}

void BuildVLITable(JPEGINFO *pJpgInfo)
{
	short i   = 0;
	
	for (i = 0; i < DC_MAX_QUANTED; ++i)
	{
		pJpgInfo->pVLITAB[i] = ComputeVLI(i);
	}
	
	for (i = DC_MIN_QUANTED; i < 0; ++i)
	{
		pJpgInfo->pVLITAB[i] = ComputeVLI(i);
	}
}

int WriteSOI(unsigned char* pOut,int nDataLen)
{ 
	memcpy(pOut+nDataLen,&SOITAG,sizeof(SOITAG));
	return nDataLen+sizeof(SOITAG);
}


int WriteEOI(unsigned char* pOut,int nDataLen)
{
	memcpy(pOut+nDataLen,&EOITAG,sizeof(EOITAG));
	return nDataLen + sizeof(EOITAG);
}


int WriteAPP0(unsigned char* pOut,int nDataLen)
{
	JPEGAPP0 APP0;
	APP0.segmentTag  = 0xE0FF;
	APP0.length    = 0x1000;
	APP0.id[0]    = 'J';
	APP0.id[1]    = 'F';
	APP0.id[2]    = 'I';
	APP0.id[3]    = 'F';
	APP0.id[4]    = 0;
	APP0.ver     = 0x0101;
	APP0.densityUnit = 0x00;
	APP0.densityX   = 0x0100;
	APP0.densityY   = 0x0100;
	APP0.thp     = 0x00;
	APP0.tvp     = 0x00;
	//memcpy(pOut+nDataLen,&APP0,sizeof(APP0));
	memcpy(pOut+nDataLen,&APP0.segmentTag,2);
	memcpy(pOut+nDataLen+2,&APP0.length,2);
	memcpy(pOut+nDataLen+4,APP0.id,5);
	memcpy(pOut+nDataLen+9,&APP0.ver,2);
	*(pOut+nDataLen+11) = APP0.densityUnit;
	memcpy(pOut+nDataLen+12,&APP0.densityX,2);
	memcpy(pOut+nDataLen+14,&APP0.densityY,2);
	*(pOut+nDataLen+16) = APP0.thp;
	*(pOut+nDataLen+17) = APP0.tvp;

	return nDataLen + sizeof(APP0)-2;
	
}


int WriteDQT(JPEGINFO *pJpgInfo,unsigned char* pOut,int nDataLen)
{
	
	unsigned int i = 0;
	JPEGDQT_8BITS DQT_Y;
	DQT_Y.segmentTag = 0xDBFF;
	DQT_Y.length   = 0x4300;
	DQT_Y.tableInfo  = 0x00;
	for (i = 0; i < DCTBLOCKSIZE; i++)
	{
		DQT_Y.table[i] = pJpgInfo->YQT[i];
	}    
	//memcpy(pOut+nDataLen , &DQT_Y,sizeof(DQT_Y));
	memcpy(pOut+nDataLen,&DQT_Y.segmentTag,2);
	memcpy(pOut+nDataLen+2,&DQT_Y.length,2);
	*(pOut+nDataLen+4) = DQT_Y.tableInfo;
	memcpy(pOut+nDataLen+5,DQT_Y.table,64);

	nDataLen += sizeof(DQT_Y)-1;
	DQT_Y.tableInfo  = 0x01;
	for (i = 0; i < DCTBLOCKSIZE; i++)
	{
		DQT_Y.table[i] = pJpgInfo->UVQT[i];
	}
	//memcpy(pOut+nDataLen,&DQT_Y,sizeof(DQT_Y));
	memcpy(pOut+nDataLen,&DQT_Y.segmentTag,2);
	memcpy(pOut+nDataLen+2,&DQT_Y.length,2);
	*(pOut+nDataLen+4) = DQT_Y.tableInfo;
	memcpy(pOut+nDataLen+5,DQT_Y.table,64);
	nDataLen += sizeof(DQT_Y)-1;
	return nDataLen;
}



unsigned short Intel2Moto(unsigned short val)
{
	unsigned char highBits = (unsigned char)(val / 256);
	unsigned char lowBits = (unsigned char)(val % 256);
	return lowBits * 256 + highBits;
}


int WriteSOF(unsigned char* pOut,int nDataLen,int width,int height)
{
	JPEGSOF0_24BITS SOF;
	SOF.segmentTag = 0xC0FF;
	SOF.length   = 0x1100;
	SOF.precision  = 0x08;
	SOF.height   = Intel2Moto((unsigned short)height);
	SOF.width    = Intel2Moto((unsigned short)width); 
	SOF.sigNum   = 0x03;
	SOF.YID     = 0x01; 
	SOF.QTY     = 0x00;
	SOF.UID     = 0x02;
	SOF.QTU     = 0x01;
	SOF.VID     = 0x03;
	SOF.QTV     = 0x01;
	SOF.HVU     = 0x11;
	SOF.HVV     = 0x11;
	SOF.HVY   = 0x11;
//	memcpy(pOut + nDataLen,&SOF,sizeof(SOF));
	memcpy(pOut+nDataLen,&SOF.segmentTag,2);
	memcpy(pOut+nDataLen+2,&SOF.length,2);
	*(pOut+nDataLen+4) = SOF.precision;
	memcpy(pOut+nDataLen+5,&SOF.height,2);
	memcpy(pOut+nDataLen+7,&SOF.width,2);
	*(pOut+nDataLen+9) = SOF.sigNum;
	*(pOut+nDataLen+10) = SOF.YID;
	*(pOut+nDataLen+11) = SOF.HVY;
	*(pOut+nDataLen+12) = SOF.QTY;
	*(pOut+nDataLen+13) = SOF.UID;
	*(pOut+nDataLen+14) = SOF.HVU;
	*(pOut+nDataLen+15) = SOF.QTU;
	*(pOut+nDataLen+16) = SOF.VID;
	*(pOut+nDataLen+17) = SOF.HVV;
	*(pOut+nDataLen+18) = SOF.QTV;
	return nDataLen + sizeof(SOF)-1;
}


int WriteByte(unsigned char val,unsigned char* pOut,int nDataLen)
{   
	pOut[nDataLen] = val;
	return nDataLen + 1;
}

int WriteDHT(unsigned char* pOut,int nDataLen)
{
	unsigned int i = 0;
	
	JPEGDHT DHT;
	DHT.segmentTag = 0xC4FF;
	DHT.length   = Intel2Moto(19 + 12);
	DHT.tableInfo  = 0x00;
	for (i = 0; i < 16; i++)
	{
		DHT.huffCode[i] = STD_DC_Y_NRCODES[i + 1];
	} 
	//memcpy(pOut+nDataLen,&DHT,sizeof(DHT));
	memcpy(pOut+nDataLen,&DHT.segmentTag,2);
	memcpy(pOut+nDataLen+2,&DHT.length,2);
	*(pOut+nDataLen+4) = DHT.tableInfo;
	memcpy(pOut+nDataLen+5,DHT.huffCode,16);
	nDataLen += sizeof(DHT)-1;
	for (i = 0; i <= 11; i++)
	{
		nDataLen = WriteByte(STD_DC_Y_VALUES[i],pOut,nDataLen);  
	}  
	DHT.tableInfo  = 0x01;
	for (i = 0; i < 16; i++)
	{
		DHT.huffCode[i] = STD_DC_UV_NRCODES[i + 1];
	}
	//memcpy(pOut+nDataLen,&DHT,sizeof(DHT));
	memcpy(pOut+nDataLen,&DHT.segmentTag,2);
	memcpy(pOut+nDataLen+2,&DHT.length,2);
	*(pOut+nDataLen+4) = DHT.tableInfo;
	memcpy(pOut+nDataLen+5,DHT.huffCode,16);
	nDataLen += sizeof(DHT)-1;
	for (i = 0; i <= 11; i++)
	{
		nDataLen = WriteByte(STD_DC_UV_VALUES[i],pOut,nDataLen);  
	} 
	DHT.length   = Intel2Moto(19 + 162);
	DHT.tableInfo  = 0x10;
	for (i = 0; i < 16; i++)
	{
		DHT.huffCode[i] = STD_AC_Y_NRCODES[i + 1];
	}
	//memcpy(pOut+nDataLen,&DHT,sizeof(DHT));
	memcpy(pOut+nDataLen,&DHT.segmentTag,2);
	memcpy(pOut+nDataLen+2,&DHT.length,2);
	*(pOut+nDataLen+4) = DHT.tableInfo;
	memcpy(pOut+nDataLen+5,DHT.huffCode,16);
	nDataLen += sizeof(DHT)-1;
	for (i = 0; i <= 161; i++)
	{
		nDataLen = WriteByte(STD_AC_Y_VALUES[i],pOut,nDataLen);  
	}  
	DHT.tableInfo  = 0x11;
	for (i = 0; i < 16; i++)
	{
		DHT.huffCode[i] = STD_AC_UV_NRCODES[i + 1];
	}
	//memcpy(pOut + nDataLen,&DHT,sizeof(DHT)); 
	memcpy(pOut+nDataLen,&DHT.segmentTag,2);
	memcpy(pOut+nDataLen+2,&DHT.length,2);
	*(pOut+nDataLen+4) = DHT.tableInfo;
	memcpy(pOut+nDataLen+5,DHT.huffCode,16);
	nDataLen += sizeof(DHT)-1;
	for (i = 0; i <= 161; i++)
	{
		nDataLen = WriteByte(STD_AC_UV_VALUES[i],pOut,nDataLen);  
	}
	return nDataLen;
}


int WriteSOS(unsigned char* pOut,int nDataLen)
{
	JPEGSOS_24BITS SOS;
	SOS.segmentTag   = 0xDAFF;
	SOS.length    = 0x0C00;
	SOS.sigNum    = 0x03;
	SOS.YID     = 0x01;
	SOS.HTY     = 0x00;
	SOS.UID     = 0x02;
	SOS.HTU     = 0x11;
	SOS.VID     = 0x03;
	SOS.HTV     = 0x11;
	SOS.Se     = 0x3F;
	SOS.Ss     = 0x00;
	SOS.Bf     = 0x00;
	memcpy(pOut+nDataLen,&SOS,sizeof(SOS)); 
	return nDataLen+sizeof(SOS);
}


void BuildSTDHuffTab(unsigned char* nrcodes,unsigned char* stdTab,HUFFCODE* huffCode)
{
	unsigned char i     = 0;         
	unsigned char j     = 0;
	unsigned char k     = 0;
	unsigned short code   = 0; 
	
	for (i = 1; i <= 16; i++)
	{ 
		for (j = 1; j <= nrcodes[i]; j++)
		{   
			huffCode[stdTab[k]].code = code;
			huffCode[stdTab[k]].length = i;
			++k;
			++code;
		}
		code*=2;
	} 
	
	for (i = 0; i < k; i++)
	{
		huffCode[i].val = stdTab[i];  
	}
}

void FDCT(float* lpBuff)
{
	float tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
	float tmp10, tmp11, tmp12, tmp13;
	float z1, z2, z3, z4, z5, z11, z13;
	float* dataptr;
	int ctr;
	

	dataptr = lpBuff;
	for (ctr = DCTSIZE-1; ctr >= 0; ctr--)
	{
		tmp0 = dataptr[0] + dataptr[7];
		tmp7 = dataptr[0] - dataptr[7];
		tmp1 = dataptr[1] + dataptr[6];
		tmp6 = dataptr[1] - dataptr[6];
		tmp2 = dataptr[2] + dataptr[5];
		tmp5 = dataptr[2] - dataptr[5];
		tmp3 = dataptr[3] + dataptr[4];
		tmp4 = dataptr[3] - dataptr[4];
		

		tmp10 = tmp0 + tmp3; 
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;
		
		dataptr[0] = tmp10 + tmp11; /* phase 3 */
		dataptr[4] = tmp10 - tmp11;
		
		z1 = (float)((tmp12 + tmp13) * (0.707106781)); /* c4 */
		dataptr[2] = tmp13 + z1; /* phase 5 */
		dataptr[6] = tmp13 - z1;
		

		tmp10 = tmp4 + tmp5; /* phase 2 */
		tmp11 = tmp5 + tmp6;
		tmp12 = tmp6 + tmp7;
		
		z5 = (float)((tmp10 - tmp12) * ( 0.382683433)); /* c6 */
		z2 = (float)((0.541196100) * tmp10 + z5); /* c2-c6 */
		z4 = (float)((1.306562965) * tmp12 + z5); /* c2+c6 */
		z3 = (float)(tmp11 * (0.707106781)); /* c4 */
		
		z11 = tmp7 + z3; 
		z13 = tmp7 - z3;
		
		dataptr[5] = z13 + z2; /* phase 6 */
		dataptr[3] = z13 - z2;
		dataptr[1] = z11 + z4;
		dataptr[7] = z11 - z4;
		
		dataptr += DCTSIZE; 
	}
	
	dataptr = lpBuff;
	for (ctr = DCTSIZE-1; ctr >= 0; ctr--)
	{
		tmp0 = dataptr[DCTSIZE*0] + dataptr[DCTSIZE*7];
		tmp7 = dataptr[DCTSIZE*0] - dataptr[DCTSIZE*7];
		tmp1 = dataptr[DCTSIZE*1] + dataptr[DCTSIZE*6];
		tmp6 = dataptr[DCTSIZE*1] - dataptr[DCTSIZE*6];
		tmp2 = dataptr[DCTSIZE*2] + dataptr[DCTSIZE*5];
		tmp5 = dataptr[DCTSIZE*2] - dataptr[DCTSIZE*5];
		tmp3 = dataptr[DCTSIZE*3] + dataptr[DCTSIZE*4];
		tmp4 = dataptr[DCTSIZE*3] - dataptr[DCTSIZE*4];
		

		tmp10 = tmp0 + tmp3;
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;
		
		dataptr[DCTSIZE*0] = tmp10 + tmp11; /* phase 3 */
		dataptr[DCTSIZE*4] = tmp10 - tmp11;
		
		z1 = (float)((tmp12 + tmp13) * (0.707106781)); /* c4 */
		dataptr[DCTSIZE*2] = tmp13 + z1; /* phase 5 */
		dataptr[DCTSIZE*6] = tmp13 - z1;
		
		tmp10 = tmp4 + tmp5; /* phase 2 */
		tmp11 = tmp5 + tmp6;
		tmp12 = tmp6 + tmp7;
		
		z5 = (float)((tmp10 - tmp12) * (0.382683433)); /* c6 */
		z2 = (float)((0.541196100) * tmp10 + z5); /* c2-c6 */
		z4 = (float)((1.306562965) * tmp12 + z5); /* c2+c6 */
		z3 = (float)(tmp11 * (0.707106781)); /* c4 */
		
		z11 = tmp7 + z3;  /* phase 5 */
		z13 = tmp7 - z3;
		
		dataptr[DCTSIZE*5] = z13 + z2; /* phase 6 */
		dataptr[DCTSIZE*3] = z13 - z2;
		dataptr[DCTSIZE*1] = z11 + z4;
		dataptr[DCTSIZE*7] = z11 - z4;
		
		++dataptr;  
	}
}


int WriteBitsStream(JPEGINFO *pJpgInfo,unsigned short value,unsigned char codeLen,unsigned char* pOut,int nDataLen)
{ 
	char posval;//bit position in the bitstring we read, should be<=15 and >=0 
	posval=codeLen-1;
	printf("posval is %d\n",posval);
	while (posval>=0)
	{
		if (value & mask[posval])
		{
			pJpgInfo->bytenew|=mask[pJpgInfo->bytepos];
		}
		posval--;
		pJpgInfo->bytepos--;
		if (pJpgInfo->bytepos<0) 
		{ 
			if (pJpgInfo->bytenew==0xFF)
			{
				nDataLen = WriteByte(0xFF,pOut,nDataLen);
				nDataLen = WriteByte(0,pOut,nDataLen);
			}
			else
			{
				nDataLen = WriteByte(pJpgInfo->bytenew,pOut,nDataLen);
			}
			pJpgInfo->bytepos=7;
			pJpgInfo->bytenew=0;
		}
	}
	return nDataLen;
}

int WriteBits(JPEGINFO *pJpgInfo,HUFFCODE huffCode,unsigned char* pOut,int nDataLen)
{  
	return WriteBitsStream(pJpgInfo,huffCode.code,huffCode.length,pOut,nDataLen); 
}

int WriteBits2(JPEGINFO *pJpgInfo,SYM2 sym,unsigned char* pOut,int nDataLen)
{
	return WriteBitsStream(pJpgInfo,sym.amplitude,sym.codeLen,pOut,nDataLen); 
}

double mypow(double x,double y){
	int i=0;
	double sum=1;
	for(i=1;i<=(int)y;i++)
		sum *=x;
	return sum;
}

SYM2 BuildSym2(short value)
{
	SYM2 Symbol;  
	
	Symbol.codeLen = ComputeVLI(value);           
	Symbol.amplitude = 0;
	if (value >= 0)
	{
		Symbol.amplitude = value;
	}
	else
	{
		double tmp = mypow(2,Symbol.codeLen);
		Symbol.amplitude = (short)(tmp-1) + value; 
	}
	
	return Symbol;
}


void RLEComp(short* lpbuf,ACSYM* lpOutBuf,unsigned char *resultLen)
{  
	unsigned char zeroNum     = 0;
	unsigned int EOBPos      = 0;
	unsigned char MAXZEROLEN = 15;
	unsigned int i        = 0;    
	unsigned int j        = 0;
	
	EOBPos = DCTBLOCKSIZE - 1;
	for (i = EOBPos; i > 0; i--) 
	{
		if (lpbuf[i] == 0) 
		{
			--EOBPos; 
		}
		else     
		{
			break;                   
		}
	}
	
	for (i = 1; i <= EOBPos; i++) 
	{
		if (lpbuf[i] == 0 && zeroNum < MAXZEROLEN)
		{
			++zeroNum;   
		}
		else
		{   
			lpOutBuf[j].zeroLen = zeroNum; 
			lpOutBuf[j].codeLen = ComputeVLI(lpbuf[i]);
			lpOutBuf[j].amplitude = lpbuf[i];      
			zeroNum = 0;     
			++(*resultLen);        
			++j;       
		}
	} 
}


int ProcessDU(JPEGINFO *pJpgInfo,float* lpBuf,float* quantTab,HUFFCODE* dcHuffTab,HUFFCODE* acHuffTab,short* DC,unsigned char* pOut,int nDataLen)
{
	unsigned char i    = 0;        
	unsigned int j    = 0;
	short diffVal = 0;              
	unsigned char acLen  = 0;               
	short sigBuf[DCTBLOCKSIZE];             
	ACSYM acSym[DCTBLOCKSIZE];             
	FDCT(lpBuf);                
	
	for (i = 0; i < DCTBLOCKSIZE; i++)        
	{          
		sigBuf[FZBT[i]] = (short)((lpBuf[i] * quantTab[i] + 16384.5) - 16384);
	}
	
	diffVal = sigBuf[0] - *DC;
	*DC = sigBuf[0];
	
	if (diffVal == 0)
	{  
		nDataLen = WriteBits(pJpgInfo,dcHuffTab[0],pOut,nDataLen);  
	}
	else
	{   
		nDataLen = WriteBits(pJpgInfo,dcHuffTab[pJpgInfo->pVLITAB[diffVal]],pOut,nDataLen);  
		nDataLen = WriteBits2(pJpgInfo,BuildSym2(diffVal),pOut,nDataLen);    
	}
	
	for (i = 63; (i > 0) && (sigBuf[i] == 0); i--)
	{
		
	}
	if (i == 0)             
	{
		nDataLen = WriteBits(pJpgInfo,acHuffTab[0x00],pOut,nDataLen);       
	}
	else
	{ 
		RLEComp(sigBuf,&acSym[0],&acLen);   
		for (j = 0; j < acLen; j++)          
		{   
			if (acSym[j].codeLen == 0)  
			{   
				nDataLen = WriteBits(pJpgInfo,acHuffTab[0xF0],pOut,nDataLen);           
			}
			else
			{
				nDataLen = WriteBits(pJpgInfo,acHuffTab[acSym[j].zeroLen * 16 + acSym[j].codeLen],pOut,nDataLen);
				nDataLen = WriteBits2(pJpgInfo,BuildSym2(acSym[j].amplitude),pOut,nDataLen);    
			}   
		}
		if (i != 63)         
		{
			nDataLen = WriteBits(pJpgInfo,acHuffTab[0x00],pOut,nDataLen);
		}
	}

	return nDataLen;
}


int ProcessData(JPEGINFO *pJpgInfo,unsigned char* lpYBuf,unsigned char* lpUBuf,unsigned char* lpVBuf,int width,int height,int myBufLen, int muBufLen,int mvBufLen,unsigned char* pOut,int nDataLen)
{ 
	size_t yBufLen = strlen(lpYBuf);    
	size_t uBufLen = strlen(lpUBuf);           
	size_t vBufLen = strlen(lpVBuf);      
	float dctYBuf[DCTBLOCKSIZE];          
	float dctUBuf[DCTBLOCKSIZE];        
	float dctVBuf[DCTBLOCKSIZE];        
	unsigned int mcuNum   = 0;           
	short yDC   = 0;          
	short uDC   = 0;        
	short vDC   = 0;       
	unsigned char yCounter  = 0;     
	unsigned char uCounter  = 0;
	unsigned char vCounter  = 0;
	unsigned int i    = 0;                      
	unsigned int j    = 0;                 
	unsigned int k    = 0;
	unsigned int p    = 0;
	unsigned int m    = 0;
	unsigned int n    = 0;
	unsigned int s    = 0; 
	mcuNum = (height * width * 3)/(DCTBLOCKSIZE * 3);         
	for (p = 0;p < mcuNum; p++)       
	{
		yCounter = 1;//MCUIndex[SamplingType][0];   
		uCounter = 1;//MCUIndex[SamplingType][1];
		vCounter = 1;//MCUIndex[SamplingType][2];
		for (; i < yBufLen; i += DCTBLOCKSIZE)
		{
			for (j = 0; j < DCTBLOCKSIZE; j++)
			{
				dctYBuf[j] = (float)(lpYBuf[i + j] - 128);
			}   
			if (yCounter > 0)
			{    
				--yCounter;
				nDataLen = ProcessDU(pJpgInfo,dctYBuf,pJpgInfo->YQT_DCT,pJpgInfo->STD_DC_Y_HT,pJpgInfo->STD_AC_Y_HT,&yDC,pOut,nDataLen);
			}
			else
			{
				break;
			}
		}  

		//------------------------------------------------------------------  
		for (; m < uBufLen; m += DCTBLOCKSIZE)
		{
			for (n = 0; n < DCTBLOCKSIZE; n++)
			{
				dctUBuf[n] = (float)(lpUBuf[m + n] - 128);
			}    
			if (uCounter > 0)
			{    
				--uCounter;
				nDataLen = ProcessDU(pJpgInfo,dctUBuf,pJpgInfo->UVQT_DCT,pJpgInfo->STD_DC_UV_HT,pJpgInfo->STD_AC_UV_HT,&uDC,pOut,nDataLen);
			}
			else
			{
				break;
			}
		}  
		//-------------------------------------------------------------------  
		for (; s < vBufLen; s += DCTBLOCKSIZE)
		{
			for (k = 0; k < DCTBLOCKSIZE; k++)
			{
				dctVBuf[k] = (float)(lpVBuf[s + k] - 128);
			}
			if (vCounter > 0)
			{
				--vCounter;
				nDataLen = ProcessDU(pJpgInfo,dctVBuf,pJpgInfo->UVQT_DCT,pJpgInfo->STD_DC_UV_HT,pJpgInfo->STD_AC_UV_HT,&vDC,pOut,nDataLen);
			}
			else
			{
				break;
			}
		}  
	} 
	return nDataLen;
}

int YUV2Jpg(unsigned char* in_Y,unsigned char* in_U,unsigned char* in_V,int width,int height,int quality,int nStride,unsigned char* pOut,unsigned long *pnOutSize)
{
	unsigned char* pYBuf;
	unsigned char* pUBuf;
	unsigned char* pVBuf;
	int nYLen = nStride  * height;
	//int nUVLen = nStride  * height / 4;
	int	nDataLen;
	JPEGINFO * JpgInfo;
	JpgInfo = (JPEGINFO *)malloc(sizeof(JPEGINFO));

	printf("sizeof(JPEGAPP0)=\n");
	printf("sizeof(JPEGDQT_8BITS)=\n");
	printf("sizeof(JPEGSOF0_24BITS)=\n");
	printf("sizeof(JPEGDHT)=%u\n",sizeof(JPEGDHT));
	printf("10\n");
	printf("sizeof(JPEGinfo)=%u\n",sizeof(JPEGINFO));
  
	memset(JpgInfo,0,sizeof(JPEGINFO));
	//printf("sizeof(JPEGINFO)=%d\n",sizeof(JPEGINFO));
	//JpgInfo.bytenew = 0;
	//JpgInfo.bytepos = 7;
	JpgInfo->bytenew = 0;
	JpgInfo->bytepos = 7;
	printf("------------------------------6\n");
	pYBuf = (unsigned char*)malloc(nYLen);
	memset(pYBuf,0,nYLen);
	//printf("nYLen=%d\n",nYLen);
	memcpy(pYBuf,in_Y,nYLen);
	printf("-----------------------------7\n");
	pUBuf = (unsigned char*)malloc(nYLen);
	pVBuf = (unsigned char*)malloc(nYLen);
	printf("---------------------------------8\n");
	//memcpy(pUBuf,in_U,nUVLen);
	//memcpy(pVBuf,in_V,nUVLen);
	//memset(pUBuf,0,nYLen/4);
	//memset(pVBuf,0,nYLen/4);
	
	ProcessUV(pUBuf,in_U,width,height,nStride);
	ProcessUV(pVBuf,in_V,width,height,nStride);
	printf("-------------------------------5\n");
	DivBuff(pYBuf,width,height,nStride,DCTSIZE,DCTSIZE);
	DivBuff(pUBuf,width,height,nStride,DCTSIZE,DCTSIZE);
	DivBuff(pVBuf,width,height,nStride,DCTSIZE,DCTSIZE);
	quality = QualityScaling(quality);
	SetQuantTable(std_Y_QT,JpgInfo->YQT, quality);
	SetQuantTable(std_UV_QT,JpgInfo->UVQT,quality); 
	
	InitQTForAANDCT(JpgInfo);
	JpgInfo->pVLITAB=JpgInfo->VLI_TAB +2048;  
	BuildVLITable(JpgInfo);          
	
	nDataLen = 0;
	printf("---------------------------2\n");
	nDataLen = WriteSOI(pOut,nDataLen); 
	nDataLen = WriteAPP0(pOut,nDataLen);
	nDataLen = WriteDQT(JpgInfo,pOut,nDataLen);
	nDataLen = WriteSOF(pOut,nDataLen,width,height);
	nDataLen = WriteDHT(pOut,nDataLen);
	nDataLen = WriteSOS(pOut,nDataLen);
	
	printf("------------------------------1\n");
	BuildSTDHuffTab(STD_DC_Y_NRCODES,STD_DC_Y_VALUES,JpgInfo->STD_DC_Y_HT);
	BuildSTDHuffTab(STD_AC_Y_NRCODES,STD_AC_Y_VALUES,JpgInfo->STD_AC_Y_HT);
	BuildSTDHuffTab(STD_DC_UV_NRCODES,STD_DC_UV_VALUES,JpgInfo->STD_DC_UV_HT);
	BuildSTDHuffTab(STD_AC_UV_NRCODES,STD_AC_UV_VALUES,JpgInfo->STD_AC_UV_HT);
	printf("-----------------------------3\n");
	nDataLen = ProcessData(JpgInfo,pYBuf,pUBuf,pVBuf,width,height,nYLen,nYLen,nYLen,pOut,nDataLen);  
	nDataLen = WriteEOI(pOut,nDataLen);
	printf("------------------------------4\n");
	free(JpgInfo);
	free(pYBuf);
	free(pUBuf);
	free(pVBuf);
	*pnOutSize = nDataLen;
	return 0;
}
