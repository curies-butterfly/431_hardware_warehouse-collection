
#include "FFT.h"
#include "Data_Frame.h"
#include "stdio.h"
//#include "std.h"
//#include "log.h"
#include "SCI.h"
#include "string.h"
#include "main.h"
/******************************宏定义***********************************************************/

#define Jian50 1
#define pi 3.141593 // float小数点后6位
//#define NL 256  // SampCnt为合成信号点数，与N的值必须是一致的，即SampCnt=N
#define F1 50  //三个输入信号频率
#define F2 200
#define F3 0
#define SampRate 12800//采样速率 单位Hz
/********************************************************************************************/
const float Lkg_in[42] =  { 0,12,22,30,41,50,52,60,70,80,90,102,114,126,132,138,153,162,171,185,193,213,224,236,244,257,271,286,296,312,323,330,345,358,370,380,392,400,410,424,430,450 }; //real value
const float Lkg_out[42] = { 22,23,23,28,41,90,98,124,162,196,244,310,380,400,466,490,537,555,570,592,598,637,658,668,670,674,685,696,705,712,713,725,730,734,741,743,745,747,754,750,750,751 };
/**************************************结构体定义************************************************/
//Uint16  SampleTable[NL];
struct Complex		// 定义复数结构体
{
   float real,imag;
};

struct Complex Wn;//定义旋转因子
struct Complex Vn;//每一级第一个旋转因子虚部为0，实部为1
struct Complex T;//存放旋转因子与X(k+B)的乘积

//float Realin[NL]={0};// 采样输入的实数
float output[SampCnt]={0};// 输出的FFT幅值（复数的模）
struct Complex Sample_I[SampCnt];// 电流采样输入的实数转化为复数
struct Complex Sample_U[SampCnt];// 电压采样输入的实数转化为复数

struct Complex MUL(struct Complex a,struct Complex b)//定义复乘
{
   struct Complex c;
   c.real=a.real*b.real-a.imag*b.imag;
   c.imag=a.real*b.imag+a.imag*b.real;
   return(c);
}
/********************************************************************************************/

/**************************************变量定义************************************************/
float  Input[SampCnt];//输入的信号序列
float  I_Array1[SampCnt];//电流数组
int ARC_Index = 1;
//float  I_Array14[SampCnt/1];//x/电流数组
float  U_Array1[SampCnt];//电压数组
float  Sample_Leakge_Current_1[SampCnt]; //剩余电流数组
char   IsADC1=0x1; //ADC采集处理完成标志位

float  I_Array2[SampCnt];//电流数组
float  U_Array2[SampCnt];//电压数组
float  Sample_Leakge_Current_2[SampCnt]; //剩余电流数组
char   IsADC2=0x0; //ADC采集处理完成标志位

float  I_Array3[SampCnt];//电流数组
float  U_Array3[SampCnt];//电压数组
float  Sample_Leakge_Current_3[SampCnt]; //剩余电流数组
char   IsADC3=0x0; //ADC采集处理完成标志位
float  I_Array_Old1[SampCnt];//上一次电流数组
float  I_Array_Old2[SampCnt];//上一次电流数组
float  I_Array_Old3[SampCnt];//上一次电流数组
//float  U_Array_Old[SampCnt];//电压数组
float  Diff_I_Buff[SampCnt];//差分电流数组

char ArcTHBase = 7; //电弧基础值 暂用于初始化使用
Uint16 LeakgeI1,LeakgeI2,LeakgeI3;   //剩余电流值
float P_total1=0,P_total2=0,P_total3=0;       //总功率
float P_change1=0,P_change2=0,P_change3=0;    //单功率
float P_change1_old=0,P_change2_old=0,P_change3_old=0;    //上一次的单功率
float P_total_new1_CRR=0,P_total_new2_CRR=0,P_total_new3_CRR=0;
Uint16 Correction_value_1 = 0,Correction_value_2 = 0,Correction_value_3 = 0;  //校正值
Uint16 ZeroCV1 = 0,ZeroCV2 = 0,ZeroCV3 = 0;  //零点位置
Uint16 Arc_Cnt_Base1 = 0,Arc_Cnt_Base2 = 0,Arc_Cnt_Base3 = 0; //arc计数值
Uint16 SingleArcTH1 = 8,SingleArcTH2 = 8,SingleArcTH3 = 8;  //电弧单周期阈值数值
/*电流微分*/
//float P_total_buff_1=0,P_total_buff_2=0,P_total_buff_3=0;    //总功率—buff
//float P_change_buff_1=0,P_change_buff_2=0,P_change_buff_3=0;    //单功率-buff
char Current_Diff_Start_1=0;//开始计算电流速率标志
char Current_Diff_Start_2=0;//开始计算电流速率标志
char Current_Diff_Start_3=0;//开始计算电流速率标志
float Power_Buff1[Diff_Samples_Num]={0,};//功率数组
float Power_Buff2[Diff_Samples_Num]={0,};//功率数组
float Power_Buff3[Diff_Samples_Num]={0,};//功率数组
int Power_Diff1 = 0,Power_Diff2 = 0,Power_Diff3 = 0;
Uint16 CurtDiffPeriod = 50;  //unit：ms 电流采样单周期 共Diff_Samples_Num个周期
Uint16 Diff_Cnt = 0; //电流计数值
/*diff*/
//LCD
char HBCode1[]="000000",HBCode2[]="000000",HBCode3[]="000000";//用电器编码,5+1,末尾1接入,0移除
float UA1=0,UA2=0,UA3=0;
float PwrFactor1=0,PwrFactor2=0,PwrFactor3=0; //功率因子
char  PhDiff1=0,PhDiff2=0,PhDiff3=0;          //相位差
float gTypPara1[TypParCnt],gTypPara2[TypParCnt],gTypPara3[TypParCnt]; //识别参数
float gTypParaMax[TypParCnt];//识别参数30个最大幅值数组
char  FreID[TypParCnt];//识别参数30个最大频点数组

Uint16 ArcCnt1=0,ArcCnt2=0,ArcCnt3=0;		   //电弧产生次数
Uint16 Arc_timeFlag1=0,Arc_timeFlag2=0,Arc_timeFlag3=0; //电弧计数标志 1：开始计数 2：计数到10s  3：计数到30s

extern Uint16 SampleTable[SampCnt];  //定义接收BUF的SIZE
/********************************************************************************************/
#pragma CODE_SECTION(Freq_Analysis1,"ramfuncs");
#pragma CODE_SECTION(Freq_Analysis2,"ramfuncs");
#pragma CODE_SECTION(Freq_Analysis3,"ramfuncs");
#pragma CODE_SECTION(FFT,"ramfuncs");
#pragma CODE_SECTION(ModelComplex,"ramfuncs");
#pragma CODE_SECTION(addSum,"ramfuncs");
#pragma CODE_SECTION(RMS,"ramfuncs");
#pragma CODE_SECTION(LAN_Deal,"ramfuncs");

void FFT(struct Complex *xin,int N)//输入为复数指针*xin，做N点FFT
{
   int L=0; // 级间运算层
   int J=0; // 级内运算层
   int K=0,KB=0; // 蝶形运算层
   int M=1,Nn=0;// N=2^M
   float B=0; // 蝶形运算两输入数据间隔
   /* 以下是为倒序新建的局部变量*/
   int LH=0,J2=0,N1=0,I,K2=0;
   struct Complex T;
   /*以下是倒序*/
   LH=N/2; // LH=N/2
   J2=LH;
   N1=N-2;
   for(I=1;I<=N1;I++)
    {
     if(I<J2)
	 {
       T=xin[I];
       xin[I]=xin[J2];
       xin[J2]=T;
	 }
	 K2=LH;
	 while(J2>=K2)
	  {
        J2-=K2;
        K2=K2/2;// K2=K2/2
	  }
	  J2+=K2;
    }
   /* 以下为计算出M */
   Nn=N;
   while(Nn!=2)// 计算出N的以2为底数的幂M
   {
     M++;
	 Nn=Nn/2;
   }

   /* 蝶形运算 */
   for(L=1;L<=M;L++)  // 级间
	{
	  B=pow(2,(L-1));
      Vn.real=1;
	  Vn.imag=0;
      Wn.real=cos(pi/B);
      Wn.imag=-sin(pi/B);
	  for(J=0;J<B;J++)   // 级内
	   {
		 for(K=J;K<N;K+=2*B)  // 蝶形因子运算
		  {
            KB=K+B;
            T=MUL(xin[KB],Vn);
            xin[KB].real=xin[K].real-T.real;//原址运算，计算结果存放在原来的数组中
            xin[KB].imag=xin[K].imag-T.imag;
             xin[K].real=xin[K].real+T.real;
             xin[K].imag=xin[K].imag+T.imag;
		  }
		  Vn=MUL(Wn,Vn);// 旋转因子做复乘相当于指数相加，得到的结果
		  // 和J*2^（M-L）是一样的，因为在蝶形因子运算
		 // 层中M与L都是不变的，唯一变x化的是级内的J
		 // 而且J是以1为步长的，如J*W等效于W+W+W...J个W相加
		}
	 }
}

/********************************
功能：计算复数的模
形参：*Sample指向需要取模的复数结构体
      N为取模点数
	  *output存放取模后数值的数组
*********************************/
void ModelComplex(struct Complex *Sample,int N,float *output)
{
   int i;
   for(i=0;i<N;i++)
    {
     output[i]=sqrt(Sample[i].real*Sample[i].real+Sample[i].imag*Sample[i].imag)*2/N;
	}
}

/*void FilterDC(struct Complex *ADC,int N)//去除数据中的直流成分，否则直流分量将很大
{
   int i;
   float sum=0;
   for(i=0;i<N;i++)
    { sum+=ADC[i].real;}
   sum=sum/N;
   for(i=0;i<N;i++)
    { ADC[i].real-=sum;}
}*/

//累加和（参数：起始下标，结束下标，数组指针）
float addSum(Uint16 n, Uint16 N, float* ptr)
{
	float sum=0;
	int i;

	for(i=n; i<N+1; i++)
	{
		sum += ptr[i];
	}

	return sum;
}

/*****************************************************************************
函数功能:求电流有效值
参        数:采样电压数组
返  回  值:电流有效值（单位A）
*****************************************************************************/
float RMS(float* ptr)
{
	Uint16 i;
	float a[SampCnt];
	float sum;

	for(i=0; i<SampCnt-1; i++)
	{
		a[i] = ptr[i]*ptr[i] + ptr[i+1]*ptr[i+1];
	}

	sum = addSum(0, SampCnt-2, a); //累加和
	sum = sum/(2*(SampCnt-1));

	return sqrt(sum);	//开平方
}

//电弧处理函数
Uint16 LAN_Deal(float *Arry,char CH)
{
	int i;
	int arc_state = 0;
	int count_arcc = 0;
	Uint16 Handler_ADC[SampCnt];  //电弧处理数组
	/*for(i=0; i<SampCnt/ARC_Index; i++)
	{
		I_Array14[i] = Arry[i*ARC_Index];
	}*/
	switch (CH)
	{
	case 1:
		if(SingleArcTH1>9)
			ARC_Index = 4;
		else
			ARC_Index = 1;
		break;
	case 2:
		if(SingleArcTH2>9)
			ARC_Index = 4;
		else
			ARC_Index = 1;
	case 3:
		if(SingleArcTH3>9)
			ARC_Index = 4;
		else
			ARC_Index = 1;
	default:
		break;
	}
	for(i=0; i<SampCnt/ARC_Index; i++)
	{
		/*if(fabs(Arry[i]) < 0.6)
			Handler_ADC[i] = 0;
		else if(fabs(Arry[i]) > 1)
			Handler_ADC[i] = 1;
		if(i<SampCnt-1)
			Handler_ADC[i+1] = Handler_ADC[i]; //施密特触发方式*/
		if(fabs(Arry[i*ARC_Index]) < 0.6)   // TEST 改成0.6->0.4打弧好打一些，房间未测试
			Handler_ADC[i] = 0;
		else
			Handler_ADC[i] = 1;
	}
	for(i=0; i<SampCnt/ARC_Index; i++)
	{
		switch(arc_state)
		{
			case 0:
				if(Handler_ADC[i] == 0)
				{
					arc_state = 1;
				}
				else
				{
					arc_state = 0;
				}
				break;
			case 1:
				if(Handler_ADC[i] == 1)
				{
					arc_state = 2;
				}
				else
				{
					arc_state = 1;
				}
				break;
			case 2:
				if(Handler_ADC[i] == 0)
				{
					count_arcc += 1;
					arc_state = 0;
				}
				else
				{
					arc_state = 2;
				}
				break;
			default :
				count_arcc = 0;
				break;
		}
	}
	//count_arcc = count_arcc>4?4:count_arcc;
	//count_arcc = count_arcc -2;
	//return count_arcc>0?count_arcc:0;//一个周期最多算1个电弧

	//count_arcc = count_arcc>4?4:count_arcc;
	switch (CH)
	{
	case 1:
		if(count_arcc >= SingleArcTH1) //单周期的010序列值大于阈值，电弧波数量+1;
			return 1;
		break;
	case 2:
		if(count_arcc >= SingleArcTH2)
			return 1;
		break;
	case 3:
		if(count_arcc >= SingleArcTH3)
			return 1;
		break;
	default:
		break;
	}
	//return count_arcc;
	return 0;
}

/*************************************
 * 获取频谱值、相位
 *************************************/
//#pragma CODE_SECTION(Freq_Analysis1,"ramfuncs");
u8 ArcCalStart1 = 0; //电弧计数开始
unsigned char NumOfPacksLess1 = 0; //小包数量15-30
unsigned char NumOfPacksMore1 = 0; //小包数量30-50
#define RefreshPowerTH 30
char RefreshPowerFlag1 = 0;
char RefreshPowerFlag2 = 0;
char RefreshPowerFlag3 = 0;
void Freq_Analysis1()
{
	Uint16  i,ArcCnt;
	static char flag1 = 0;// 防止功率波动标志位
	static char arcflag1 = 0;// 计算电弧次数标志
	double Ph1,Ph2;
	float IA1,P_total_new1;

	if(IsADC1&0x2)
	{
		LeakgeI_Cal();
		if(ArcCalStart1)
		{
			arcflag1++;
			if(arcflag1<=25)//25个周期 500ms的值
			{
				ArcCnt=LAN_Deal(I_Array1,1);
			//SCI_Printf("%d ",(int)ArcCnt);
			if(P_total1>200)
				ArcCnt1+=ArcCnt;
			}
			if(arcflag1 == 50) //50个周期再发送
			{
				arcflag1 = 0;
				ArcCalStart1 = 0;
				//ArcCnt1 = ArcCnt1%120;
				if(ArcCnt1)
				{
					LAN_SendArc(0x00);//LAN发送电弧识别包
					SCI_Printf("ARC1 = %d \n",(int)ArcCnt1);
				}
				//printf("ARC1 = %d\n",(int)ArcCnt1);
				if(ArcCnt1>8)
					Err_Code = Err_Code|gBIT12;  //set arcdanger bit
				ArcCnt1 = 0;
			}
		}
	    IA1 =RMS(I_Array1);
	    if(IA1 >=70)
	    {
	    	IA1 = 0; // 未安装霍尔环  电流为74A
	    	Err_Code = Err_Code|gBIT10;
	    }
	    else if(IA1 >= Set_Factory_TH)
	    {
	    	Err_Code = Err_Code|gBIT14;//功率过大设置标志位
	    }
	    else
	    {
	    	Err_Code = Err_Code&(~gBIT10);//清除霍尔环错误标志
	    	Err_Code = Err_Code&(~gBIT14);//清除功率过大标志
	    }
	    UA1  =RMS(U_Array1);
	    if(UA1>Set_Voltage_TH_Max&&UA1<Ignore_detection_voltage_Up)	{Err_Code = Err_Code&(~gBIT11);Err_Code = Err_Code|gBIT13;}//清除欠压，标记过压
	    else if(UA1<Set_Voltage_TH_Min&&UA1>Ignore_detection_voltage_Down)	{Err_Code = Err_Code&(~gBIT13);Err_Code = Err_Code|gBIT11;}
	    else {Err_Code = Err_Code&(~gBIT11);Err_Code = Err_Code&(~gBIT13);}
        P_total_new1 = IA1 *220; //总功率
        P_total_new1_CRR = P_total_new1;
#if Jian50
        if(P_total_new1 <100)
        	P_total_new1-= (float)Correction_value_1;
#endif
        if(P_total_new1<0)
        	P_total_new1=0;
        P_change1= P_total_new1 - P_total1; //变化功率
        if(RefreshPowerFlag1 && P_total_new1 < RefreshPowerTH && fabs(P_change1)<25) //功率小于30定时刷新
        {
        ////会出现功率因数不变化，疑似未限制功率变化上限导致在收到刷新指令时未能有效的检测波动，从而错过功率因数计算，可加一个功率变化上限<25
        	if(fabs(P_change1)>10)
        		P_total1 = P_total_new1;
        	RefreshPowerFlag1 = 0;
        }

        /*if(fabs(P_change1) < Pwr_Mini)
        {
			for(i=0;i<SampCnt;i++)   //输入实数信号转换为复数
			{
			  I_Array_Old1[i] = I_Array1[i]; //保存电流数组
			}
        }*/
        IsADC1 = 0x1;
        if(fabs(P_change1)>=Pwr_Mini) //功率发生变化
        	flag1++;
        if(flag1<WaveFlag) //功率发生变化后三个周波后 记录数据
        	return ;
		flag1 = 0;//标志位清除
		IsADC1 = 0x0;//数据处理完成，继续采集
        P_total1 = P_total_new1;//保存本次功率
        /*小包统计*/
        if( (Current_Diff_Start_1) && (fabs(P_change1)<=MinClassfiValueLowerTH) ) //前一次有小包且这次小于30，加小的包
        {
        	NumOfPacksLess1++;
        	if(NumOfPacksLess1==255)
        		NumOfPacksLess1 = 254;
        	return;
        }
        else if((Current_Diff_Start_1) && (fabs(P_change1)<MinClassfiValueUpperTH))//前一次有小包且这次大于30小于50
        {
        	NumOfPacksMore1++;
        	if(NumOfPacksMore1==255)
        		NumOfPacksMore1 = 254;
        	return;
        }
//        else if((Current_Diff_Start_1) && (fabs(P_change1)>MinClassfiValueUpperTH))
//        {
//        	LAN_SendTyp(0x0);
//        	//此时的功率buff？、挪到后面 大于50发送 清零 本段注释
//        	//若是大于50版函数直接发送，主函数发送停止，若小于50 本处不发送，主函数也不会发送，可以将主函数的发送取消掉
//        	SCI_Printf("catt\n");
//        	NumOfPacksLess1 = 0;
//        	NumOfPacksMore1 = 0;
//        	Current_Diff_Cnt_1 = 0;
//        	Current_Diff_Start_1 = 0;
//        	//发出本次	等待下一次小包的记录
//        }
        //end
//        P_change_buff_1 = P_change1;
//        P_total_buff_1 = P_total1;//保存第一次采集的功率


        for(i=0;i<SampCnt;i++)   //输入实数信号转换为复数
    	  {

    		if(P_change1>0)
    		    Diff_I_Buff[i] = I_Array1[i] - I_Array_Old1[i];//差分数组
    		else
    		  	Diff_I_Buff[i] = I_Array_Old1[i] - I_Array1[i];//差分数组

    		  Sample_I[i].real=Diff_I_Buff[i];
    		  Sample_I[i].imag=0;

    		  Sample_U[i].real=U_Array1[i];
    		  Sample_U[i].imag=0;

    		  I_Array_Old1[i] = I_Array1[i]; //保存电流数组
    	  }

   	   FFT(Sample_I,SampCnt);                //电流FFT
   	   FFT(Sample_U,SampCnt);                //电压FFT
       ModelComplex(Sample_I,SampCnt,output);  //求模

       Ph1 = atan2(Sample_I[1].imag,Sample_I[1].real); //相位 弧度
       Ph2 = atan2(Sample_U[1].imag,Sample_U[1].real); //相位 弧度
       Ph1 = Ph1*180/pi;
       Ph2 = Ph2*180/pi;
       //PwrFactor1 = fabs(cos((Ph2-Ph1)*pi/180));//功率因子
       PwrFactor1 = (cos((Ph2-Ph1)*pi/180));//功率因子
       PhDiff1 = (char)Ph1-Ph2; //相位差
       if(PhDiff1>180)
    	   PhDiff1-=180;
       if(PhDiff1<-180)
    	   PhDiff1+=180;

       for(i=0;i<TypParCnt;i++)
       	   gTypPara1[i] = output[i];

#if outputPar
           //打印第一路电器数据
       	   SCI_Printf("<meta>\n");
       	   SCI_Printf("<channel>1</channel>\n");
		   SCI_Printf("<total>%f</total>\n",(float)P_total1);
		   SCI_Printf("<change>%f</change>\n",(float)P_change1);
		   SCI_Printf("<freq>\n");
		   for(i=0;i<TypParCnt;i++)
		   {
			   SCI_Printf("%ld ",(long)(gTypPara1[i]*10000));
		   }
		   SCI_Printf("\n</freq>\n");
		   SCI_Printf("<factor>%f</factor>\n",(float)PwrFactor1);
		   SCI_Printf("</meta>\n");
#endif

	   //开始500ms电弧计算
	  if(fabs(P_change1)>MinClassfiValueUpperTH)
	  {
		  ArcCalStart1 = 1;
//		  LAN_SendTyp(0x0); //发送电器识别包
//		  SCI_Printf("bigger\n");
//		  for(i=0;i<256;i++)
//		  {
//			  SCI_Printf("%f ",Diff_I_Buff[i]);
//		  }
//		  SCI_Printf("\n%f ",(float)P_total1);
//		   SCI_Printf("%f ",(float)P_change1);
//		   for(i=0;i<TypParCnt;i++)
//		   {
//			   SCI_Printf("%f ",(float)gTypPara1[i]);
//		   }
//		   SCI_Printf("%f ",(float)PwrFactor1);

      	LAN_SendTyp(0x0);
		SCI_Printf("Send Type @1, in FFT.\r\n");

		//SCI_Printf("1大 小%d %d\n",NumOfPacksMore1,NumOfPacksLess1);
      	//此时的功率buff？、挪到后面 大于50发送 清零 本段注释
      	//若是大于50版函数直接发送，主函数发送停止，若小于50 本处不发送，主函数也不会发送，可以将主函数的发送取消掉
//      	SCI_Printf("catt\n");
      	NumOfPacksLess1 = 0;
      	NumOfPacksMore1 = 0;
      	Current_Diff_Cnt_1 = 0;
      	Current_Diff_Start_1 = 0;
	  }
	  else //小包
	  {
		Current_Diff_Start_1 = 1; //开始计数小包标志
	  }

	}//end
}
u8 ArcCalStart2 = 0; //电弧计数开始
unsigned char NumOfPacksLess2 = 0; //小包数量
unsigned char NumOfPacksMore2 = 0; //小包数量
void Freq_Analysis2()
{
	Uint16  i,ArcCnt;
	static char flag2 = 0;// 防止功率波动标志位
	static char arcflag2 = 0;// 计算电弧次数标志
	double Ph1,Ph2;
	float IA2,P_total_new2;

	if(IsADC2&0x2)
	{
		if(ArcCalStart2)
		{
			arcflag2++;
			if(arcflag2<=25)//25个周期 500ms的值
			{
				ArcCnt=LAN_Deal(I_Array2,2);
				//printf("%d ",(int)ArcCnt);
				if(P_total2>200)
				ArcCnt2+=ArcCnt;
			}
			if(arcflag2 == 50) //50个周期 500ms的值
			{
				arcflag2 = 0;
				ArcCalStart2 = 0;
				if(ArcCnt2)
				{
					LAN_SendArc(0x01);//LAN发送电弧识别包
					SCI_Printf("ARC2 = %d ",(int)ArcCnt2);
				}
				//printf("ARC2 = %d\n",(int)ArcCnt2);
				if(ArcCnt2>8)
					Err_Code = Err_Code|gBIT20;  //set arcdanger bit
				ArcCnt2 = 0;
			}
		}
	    IA2 =RMS(I_Array2);
	    if(IA2 >=70)
	    {
	    	IA2 = 0; // 未安装霍尔环  电流为74A
	    	Err_Code = Err_Code|gBIT18;
	    }
	    else if(IA2 >= Set_Factory_TH)
		{
			Err_Code = Err_Code|gBIT22; //功率过大设置标志位
		}
	    else
	    {
	    	Err_Code = Err_Code&(~gBIT18);//清除霍尔环错误标志
	    	Err_Code = Err_Code&(~gBIT22);//清除功率过大标志
	    }
	    UA2  =RMS(U_Array2);
	    if(UA2>Set_Voltage_TH_Max&&UA2<Ignore_detection_voltage_Up)	{Err_Code = Err_Code&(~gBIT19);Err_Code = Err_Code|gBIT21;}//清除欠压，标记过压
	    else if(UA2<Set_Voltage_TH_Min&&UA2>Ignore_detection_voltage_Down)	{Err_Code = Err_Code&(~gBIT21);Err_Code = Err_Code|gBIT19;}
	    else {Err_Code = Err_Code&(~gBIT19);Err_Code = Err_Code&(~gBIT21);}
        P_total_new2 = IA2 *220; //总功率
        P_total_new2_CRR = P_total_new2;
#if Jian50
        if(P_total_new2 <100)
        	P_total_new2-= (float)Correction_value_2;
#endif
        if(P_total_new2<0)
        	P_total_new2=0;
        P_change2= P_total_new2 - P_total2; //变化功率
        if(RefreshPowerFlag2 && P_total_new2 < RefreshPowerTH && fabs(P_change2)<25) //功率小于30定时刷新
		{
			if(fabs(P_change2)>10)
				P_total2 = P_total_new2;
			RefreshPowerFlag2 = 0;
		}
        IsADC2 = 0x1;
        if(fabs(P_change2)>=Pwr_Mini) //功率发生变化
        	flag2++;
        if(flag2<WaveFlag) //功率发生变化后三个周波后 记录数据
        	return ;
		flag2 = 0;//标志位清除
		IsADC2 = 0x0;//数据处理完成，继续采集
        P_total2 = P_total_new2;//保存本次功率
        /*小包统计*/
		if( (Current_Diff_Start_2) && (fabs(P_change2)<=MinClassfiValueLowerTH) ) //前一次有小包且这次小于30，加小的包
		{
			NumOfPacksLess2++;
        	if(NumOfPacksLess2==255)
        		NumOfPacksLess2 = 254;
			return;
		}
		else if((Current_Diff_Start_2) && (fabs(P_change2)<MinClassfiValueUpperTH))//前一次有小包且这次大于30小于50
		{
			NumOfPacksMore2++;
        	if(NumOfPacksMore2==255)
        		NumOfPacksMore2 = 254;
			return;
		}
//		else if((Current_Diff_Start_2) && (fabs(P_change2)>MinClassfiValueUpperTH))
//		{
//			LAN_SendTyp(0x1);//此时的功率buff？、
//			NumOfPacksLess2 = 0;
//			NumOfPacksMore2 = 0;
//			Current_Diff_Cnt_2 = 0;
//			Current_Diff_Start_2 = 0;
//			//发出本次	等待下一次小包的记录
//		}
        //P_change2_old = P_change2;
//        P_change_buff_2 = P_change2;
//        P_total_buff_2 = P_total2;//保存第一次采集的功率


        for(i=0;i<SampCnt;i++)   //输入实数信号转换为复数
    	  {

    		if(P_change2>0)
    		    Diff_I_Buff[i] = I_Array2[i] - I_Array_Old2[i];//差分数组
    		else
    		  	Diff_I_Buff[i] = I_Array_Old2[i] - I_Array2[i];//差分数组

    		  Sample_I[i].real=Diff_I_Buff[i];
    		  Sample_I[i].imag=0;

    		  Sample_U[i].real=U_Array2[i];
    		  Sample_U[i].imag=0;

    		  I_Array_Old2[i] = I_Array2[i]; //保存电流数组
    	  }

   	   FFT(Sample_I,SampCnt);                //电流FFT
   	   FFT(Sample_U,SampCnt);                //电压FFT
       ModelComplex(Sample_I,SampCnt,output);  //求模

       Ph1 = atan2(Sample_I[1].imag,Sample_I[1].real); //相位 弧度
       Ph2 = atan2(Sample_U[1].imag,Sample_U[1].real); //相位 弧度
       Ph1 = Ph1*180/pi;
       Ph2 = Ph2*180/pi;
       //PwrFactor2 = fabs(cos((Ph2-Ph1)*pi/180));//功率因子
       PwrFactor2 = (cos((Ph2-Ph1)*pi/180));//功率因子
       PhDiff2 = (char)Ph1-Ph2; //相位差
       if(PhDiff2>180)
    	   PhDiff2-=180;
       if(PhDiff2<-180)
    	   PhDiff2+=180;

       for(i=0;i<TypParCnt;i++)
       	   gTypPara2[i] = output[i];

#if outputPar
       	   SCI_Printf("<meta>\n");
       	   SCI_Printf("<channel>2</channel>\n");
		   SCI_Printf("<total>%f</total>\n",(float)P_total2);
		   SCI_Printf("<change>%f</change>\n",(float)P_change2);
		   SCI_Printf("<freq>\n");
		   for(i=0;i<TypParCnt;i++)
		   {

			   SCI_Printf("%ld ",(long)(gTypPara2[i]*10000));
		   }
		   SCI_Printf("\n</freq>\n");
		   SCI_Printf("<factor>%f</factor>\n",(float)PwrFactor2);
		   SCI_Printf("</meta>\n");
#endif

	   //开始500ms电弧计算
	  if(fabs(P_change2)>MinClassfiValueUpperTH)
	  {
		  ArcCalStart2 = 1;
//		  LAN_SendTyp(0x0); //发送电器识别包
//		  SCI_Printf("bigger\n");

      	LAN_SendTyp(0x1);
		SCI_Printf("Send Type @2, in FFT.\r\n");

		//SCI_Printf("2大 小%d %d\n",NumOfPacksMore2,NumOfPacksLess2);
      	//此时的功率buff？、挪到后面 大于50发送 清零 本段注释
      	//若是大于50版函数直接发送，主函数发送停止，若小于50 本处不发送，主函数也不会发送，可以将主函数的发送取消掉
//      	SCI_Printf("catt\n");
      	NumOfPacksLess2 = 0;
      	NumOfPacksMore2 = 0;
      	Current_Diff_Cnt_2 = 0;
      	Current_Diff_Start_2 = 0;
	  }
	  else //小包
	  {
		Current_Diff_Start_2 = 1; //开始计数小包标志
	  }

	}//end
}

u8 ArcCalStart3 = 0; //电弧计数开始
unsigned char NumOfPacksLess3 = 0; //小包数量
unsigned char NumOfPacksMore3 = 0; //小包数量
void Freq_Analysis3()
{
	Uint16  i,ArcCnt;
	static char flag3 = 0;// 防止功率波动标志位
	static char arcflag3 = 0;// 计算电弧次数标志
	double Ph1,Ph2;
	float IA3,P_total_new3;

	if(IsADC3&0x2)
	{
		//LeakgeI_Cal();
		if(ArcCalStart3)
		{
			arcflag3++;
			if(arcflag3<=25)//25个周期 500ms的值
			{
				ArcCnt=LAN_Deal(I_Array3,3);
				//printf("%d ",(int)ArcCnt);
				if(P_total3>200)
				ArcCnt3+=ArcCnt;
			}
			if(arcflag3 == 50) //25个周期 500ms的值
			{
				arcflag3 = 0;
				ArcCalStart3 = 0;
				if(ArcCnt3)
				{
					LAN_SendArc(0x02);//LAN发送电弧识别包
					SCI_Printf("ARC3 = %d ",(int)ArcCnt3);
				}
				//printf("ARC3 = %d\n",(int)ArcCnt3);
				if(ArcCnt3>8)
					Err_Code = Err_Code|gBIT28;  //set arcdanger bit
				ArcCnt3 = 0;
			}
		}
	    IA3 =RMS(I_Array3);
	    if(IA3 >=70)
	    {
	    	IA3 = 0; // 未安装霍尔环  电流为74A
	    	Err_Code = Err_Code|gBIT26;
	    }
	    else if(IA3 >= Set_Factory_TH)
		{
			Err_Code = Err_Code|gBIT30;  //功率过大设置标志位
		}
	    else
	    {
	    	Err_Code = Err_Code&(~gBIT26); //清除霍尔环错误标志
	    	Err_Code = Err_Code&(~gBIT30);		//清除功率过大标志
	    }
	    UA3  =RMS(U_Array3);
	    if(UA3>Set_Voltage_TH_Max&&UA3<Ignore_detection_voltage_Up)	{Err_Code = Err_Code&(~gBIT27);Err_Code = Err_Code|gBIT29;}//清除欠压，标记过压
		else if(UA3<Set_Voltage_TH_Min&&UA3>Ignore_detection_voltage_Down)	{Err_Code = Err_Code&(~gBIT29);Err_Code = Err_Code|gBIT27;}
		else {Err_Code = Err_Code&(~gBIT27);Err_Code = Err_Code&(~gBIT29);}
        P_total_new3 = IA3 *220; //总功率
        P_total_new3_CRR = P_total_new3;
#if Jian50
        if(P_total_new3 <100)
        	P_total_new3-= (float)Correction_value_3;
#endif
        if(P_total_new3<0)
        	P_total_new3=0;
        P_change3= P_total_new3 - P_total3; //变化功率
        if(RefreshPowerFlag3 && P_total_new3 < RefreshPowerTH && fabs(P_change3)<25) //功率小于30定时刷新
		{
			if(fabs(P_change3)>10)
				P_total3 = P_total_new3;
			RefreshPowerFlag3 = 0;
		}
        IsADC3 = 0x1;
        if(fabs(P_change3)>=Pwr_Mini) //功率发生变化
        	flag3++;
        if(flag3<WaveFlag) //功率发生变化后三个周波后 记录数据
        	return ;
		flag3 = 0;//标志位清除
		IsADC3 = 0x0;//数据处理完成，继续采集
        P_total3 = P_total_new3;//保存本次功率
        /*小包统计*/
		if( (Current_Diff_Start_3) && (fabs(P_change3)<=MinClassfiValueLowerTH) ) //前一次有小包且这次小于30，加小的包
		{
			NumOfPacksLess3++;
        	if(NumOfPacksLess3==255)
        		NumOfPacksLess3 = 254;
			return;
		}
		else if((Current_Diff_Start_3) && (fabs(P_change3)<MinClassfiValueUpperTH))//前一次有小包且这次大于30小于50
		{
			NumOfPacksMore3++;
        	if(NumOfPacksMore3==255)
        		NumOfPacksMore3 = 254;
			return;
		}
//		else if((Current_Diff_Start_3) && (fabs(P_change3)>MinClassfiValueUpperTH))//本次大于50发出
//		{
//			LAN_SendTyp(0x2);//此时的功率buff？、
//			NumOfPacksLess3 = 0;
//			NumOfPacksMore3 = 0;
//			Current_Diff_Cnt_3 = 0;
//			Current_Diff_Start_3 = 0;
//			//发出本次	等待下一次小包的记录
//		}
        //P_change3_old = P_change3;
//        P_change_buff_3 = P_change3;
//        P_total_buff_3 = P_total3;//保存第一次采集的功率


        for(i=0;i<SampCnt;i++)   //输入实数信号转换为复数
    	  {

    		if(P_change3>0)
    		    Diff_I_Buff[i] = I_Array3[i] - I_Array_Old3[i];//差分数组
    		else
    		  	Diff_I_Buff[i] = I_Array_Old3[i] - I_Array3[i];//差分数组

    		  Sample_I[i].real=Diff_I_Buff[i];
    		  Sample_I[i].imag=0;

    		  Sample_U[i].real=U_Array3[i];
    		  Sample_U[i].imag=0;

    		  I_Array_Old3[i] = I_Array3[i]; //保存电流数组
    	  }

   	   FFT(Sample_I,SampCnt);                //电流FFT
   	   FFT(Sample_U,SampCnt);                //电压FFT
       ModelComplex(Sample_I,SampCnt,output);  //求模

       Ph1 = atan2(Sample_I[1].imag,Sample_I[1].real); //相位 弧度
       Ph2 = atan2(Sample_U[1].imag,Sample_U[1].real); //相位 弧度
       Ph1 = Ph1*180/pi;
       Ph2 = Ph2*180/pi;
       //PwrFactor1 = fabs(cos((Ph2-Ph1)*pi/180));//功率因子
       PwrFactor3 = (cos((Ph2-Ph1)*pi/180));//功率因子
       PhDiff3 = (char)Ph1-Ph2; //相位差
       if(PhDiff3>180)
    	   PhDiff3-=180;
       if(PhDiff3<-180)
    	   PhDiff3+=180;

       for(i=0;i<TypParCnt;i++)
       	   gTypPara3[i] = output[i];

#if outputPar
   	   SCI_Printf("<meta>\n");
   	   SCI_Printf("<channel>3</channel>\n");
	   SCI_Printf("<total>%f</total>\n",(float)P_total3);
	   SCI_Printf("<change>%f</change>\n",(float)P_change3);
	   SCI_Printf("<freq>\n");
	   for(i=0;i<TypParCnt;i++)
	   {
		   SCI_Printf("%ld ",(long)(gTypPara3[i]*10000));
	   }
	   SCI_Printf("\n</freq>\n");
	   SCI_Printf("<factor>%f</factor>\n",(float)PwrFactor3);
	   SCI_Printf("</meta>\n");

#endif

	   //开始500ms电弧计算
	  if(fabs(P_change3)>MinClassfiValueUpperTH)
	  {
		  ArcCalStart3 = 1;
//		  LAN_SendTyp(0x0); //发送电器识别包
//		  SCI_Printf("bigger\n");

      	LAN_SendTyp(0x2);
		SCI_Printf("Send Type @3, in FFT.\r\n");

		//SCI_Printf("3大 小%d %d\n",NumOfPacksMore3,NumOfPacksLess3);
      	//此时的功率buff？、挪到后面 大于50发送 清零 本段注释
      	//若是大于50版函数直接发送，主函数发送停止，若小于50 本处不发送，主函数也不会发送，可以将主函数的发送取消掉
//      	SCI_Printf("catt\n");
      	NumOfPacksLess3 = 0;
      	NumOfPacksMore3 = 0;
      	Current_Diff_Cnt_3 = 0;
      	Current_Diff_Start_3 = 0;
	  }
	  else //小包
	  {
		Current_Diff_Start_3 = 1; //开始计数小包标志
	  }

	}//end
}
/**********************************
 * 描述：计算剩余电流的值
 * 输入：void
 * 返回：无
 * 备注：暂时是整流的做法，需要更改版本
 ***********************************/
#define Leakage_Current_TH 150 //剩余电流阈值，单位：mA
void LeakgeI_Cal()
{
	float temp;
	//计算第一路剩余电流
	temp = RMS(Sample_Leakge_Current_1);
	LeakgeI1 = temp;
	LeakgeI1 = ReturnLkgI(LeakgeI1);
	if(LeakgeI1 > Leakage_Current_TH)
		Err_Code = Err_Code|gBIT15;
	else
		Err_Code = Err_Code&(~gBIT15);
	//计算第2路剩余电流
	temp = RMS(Sample_Leakge_Current_2);
	LeakgeI2 = temp;
	LeakgeI2 = ReturnLkgI(LeakgeI2);
	if(LeakgeI2 > Leakage_Current_TH)
		Err_Code = Err_Code|gBIT23;
	else
		Err_Code = Err_Code&(~gBIT23);
	//计算第三路剩余电流
	temp = RMS(Sample_Leakge_Current_3);
	LeakgeI3 = temp;
	LeakgeI3 = ReturnLkgI(LeakgeI3);
	if(LeakgeI3 > Leakage_Current_TH)
		Err_Code = Err_Code|gBIT31;
	else
		Err_Code = Err_Code&(~gBIT31);
}
Uint16 ReturnLkgI(Uint16 temp)
{
	float real = 0;
	int i;
	for (i = 0; i < 42; i++)
	{
		if (temp < Lkg_out[3]+3)
		{
			real = 0;
			return (Uint16)real;
		}
		if (temp > Lkg_out[41])
			real = 999;
		if ((temp >= Lkg_out[i]) && (temp < Lkg_out[i + 1]))
		{
			real = ((temp - Lkg_out[i]) / (Lkg_out[i + 1] - Lkg_out[i]))*(Lkg_in[i + 1] - Lkg_in[i]) + Lkg_in[i];
			break;
		}
	}
	return (Uint16)real;
}
/**********************************
 * 描述：计算电流变化值
 * 输入：电流buff，buff大小
 * 返回：相邻变化最大的电流值
 * 举例：{2,1,3,7,8,5,6,27,15,6}; 则返回21，后期根据需要选择是否发送电流数组
 ***********************************/
int Cal_Current_dif(float *buf,char N)
{
    float max=-9999;
    float min=9999;
    float temp = 0;
    char i;
    for(i=N;i>1;i--)
    {
        temp = buf[i-1] - buf[i-2];
        if(temp > max)
            max = temp;
        if(temp < min)
            min = temp;
    }
    //printf("Max=%d\n",max);
    //printf("Min=%d\n",min);
    return (int)max;
}

//测试用  挑出最大的30个频点
void max30out(float * arry,float *MaxArry,char * ID)
{
    char i,j;
	float max = arry[0];

	for(j=0;j<30;j++)
	{
	  for(i=0;i<128;i++)
    	  {
    		if(arry[i]>max)
    		  {
    			max = arry[i];
    			ID[j]=i;
    		   }
    	  }
	  MaxArry[j] = max;
		arry[ID[j]]=0;
	  max = 0;
	}
}
