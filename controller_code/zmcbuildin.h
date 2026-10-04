#ifndef _ZMC_H
#define _ZMC_H

typedef unsigned long long uint64;
typedef long long int64; 

typedef unsigned char  uint8;                   /* defined for unsigned 8-bits integer variable     无符号8位整型变量  */
typedef signed   char  int8;                    /* defined for signed 8-bits integer variable        有符号8位整型变量  */
typedef unsigned short uint16;                  /* defined for unsigned 16-bits integer variable     无符号16位整型变量 */
typedef signed   short int16;                   /* defined for signed 16-bits integer variable         有符号16位整型变量 */
typedef unsigned int   uint32;                  /* defined for unsigned 32-bits integer variable     无符号32位整型变量 */
typedef signed   int   int32;                   /* defined for signed 32-bits integer variable         有符号32位整型变量 */
typedef float          fp32;                    /* single precision floating point variable (32bits) 单精度浮点数（32位长度） */
typedef double         fp64;                    /* double precision floating point variable (64bits) 双精度浮点数（64位长度） */
typedef unsigned int   uint;                    /* defined for unsigned 32-bits integer variable     无符号32位整型变量 */


typedef   double  TYPE_FRAME; 
typedef   double  TYPE_TABLE;
typedef   uint32  TYPE_AXIS;

#define PI  3.141592654

//int app_test(void);
int printf (const char *fmt, ...); // 不建议使用printf, gcc有bug
int rtprintf (const char *fmt, ...);
void *malloc(int size);
void free(void *block);
double acos(double x);
double asin(double x);
double cos(double x);
double sin(double x);
double tan(double x);
double cosh(double x);
double sinh(double x);
double tanh(double x);
double exp(double x);
double log(double x);
double modf(double value, double *iptr);
double pow(double x,double y);
double sqrt(double x);
double ceil(double x);
double fabs(double x);
double floor(double x);
double atan(double x);
double copysign(double x, double y);
float copysignf(float x, float y);
int rand(void);

#define abs(a)  ((a) < 0 ?  -(a) :(a))  

int isalnum(int ch);
int isalpha(int ch);
int isdigit(int ch);
int islower(int ch);
int isspace(int ch);
int isupper(int ch);
int isxdigit(int ch);
int tolower(int ch);
int toupper(int ch);


char *strcat(char *s1,char *s2);
char *strncat(char *dest, const char *src, int n);
char *strchr(char *s,int ch);
int strcmp(char *s1,char *s2);
int strncmp(const char *str1, const char *str2, int n);
char *strcpy(char *s1,char *s2);
char *strncpy(char *dest, const char *src, int n);
unsigned strlen(char *s);

char *itoa( int value, char *string,int radix);
char *strstr(char *str1, const char *str2);
char *strpbrk(const char *str1, const char *str2);
char *strtok(char *str, const char *delim);
int strspn(const char *str1, const char *str2);
int strcspn(const char *str1, const char *str2);
void *memset(void *str, int c, int n);
void *memmove(void *dest, const void *src, int n);
void *memcpy(void *dest, const void *src, int n);
int memcmp(const void *str1, const void *str2, int n);
void *memchr(const void *str, int c, int n);
char *strerror(int errnum);	// 预留, 无用


// motionrt 文件系统接口
// 不能在中断函数里面调用.
#define SEEK_SET	0	/* Seek from beginning of file.  */
#define SEEK_CUR	1	/* Seek from current position.  */
#define SEEK_END	2	/* Seek from end of file.  */

int motionrt_fopen(const char *path, const char *mode);
int motionrt_fclose(int ihandle);
int motionrt_fgetc(int ihandle);
int motionrt_fputc(int c, int ihandle);
char *motionrt_fgets(char *s, int size, int ihandle);
int motionrt_fputs(const char *s, int ihandle);
int motionrt_fread(void *ptr, int size, int nmemb, int ihandle);
int motionrt_fwrite(const void *ptr, int size, int nmemb, int ihandle);
int motionrt_fseek(int ihandle, long offset, int whence);
long motionrt_ftell(int ihandle);
void motionrt_rewind(int ihandle);
int motionrt_fflush(int ihandle);
int motionrt_fprintf(int ihandle, const char *format, ...);
int motionrt_fscanf(int ihandle, const char *format, ...);

//周期移动小距离
uint32 motionrt_tickrelpos(uint32 iaxis, int64 ipulspeed);

int64 motionrt_getpuldpos(uint32 iaxis);
int64 motionrt_getpulmpos(uint32 iaxis);
uint32 motionrt_getaxisstatus(uint32 iaxis);

/************下面是 version_build:231126 增加C内置函数**************/

/**************系统信息获取***************/
// table起始指针
TYPE_TABLE *motionrt_gettableptr();

// 4x缓冲, 修改无法更新到vr
uint16 *motionrt_getmodbus4xptr();

// 0x缓冲
uint8 *motionrt_getmodbus0xptr();

// 长度
uint32 	motionrt_getmodbus4xsize();
// 0x缓冲, BITES个数
uint32	motionrt_getmodbus0xsize();



/**************时间相关函数***************/

// 让出CPU, 中断里面不能调用.
void motionrt_delayms(int ims);

// 让出CPU, 中断里面不能调用.
void motionrt_delayus(int ius);

// 让出cpu, 避免RTSYS连不上, 中断里面不能调用.
void motionrt_schedule();

// 系统上电后的周期数, 可能溢出
uint32 motionrt_getticks();

// 系统上电后的ms时间, 可能溢出
uint32 motionrt_gettimems();

// 系统上电后的us时间, 可能溢出
uint32 motionrt_gettimeus();

// 当前tick开始后的时间, 与motionrt_getticks 可以结合确定时间.
uint32 motionrt_gettimeus_aftertick();






/**********************IO相关函数***********************/
/******************************************************
Description:    // io的最大个数
Input:          // 
Output:         //
Return:         //io最大个数
*******************************************************/
uint32 motionrt_getiomaxnum();

/******************************************************
Description:    // 读取io口输入状态, 没有翻转前的
Input:          // io口号
Output:         //
Return:         //io口输入状态
*******************************************************/
int motionrt_getin(uint32 ionum);

/******************************************************
Description:    // 本地输入快速读取
Input:          // io输入个数
Output:         //
Return:         //本地输入io状态
*******************************************************/
int motionrt_getlocalin(uint32 ionum);

/******************************************************
Description:    // 本地输入快速读取, 整体读取
Input:          // 
Output:         //
Return:         //本地输入全部io状态
*******************************************************/
uint64 motionrt_getlocalinall();

/******************************************************
Description:    //用于本地输入成片读取, 不能修改.
Input:          //
Output:         //
Return:         //本地输入成片读取的io状态
*******************************************************/
uint8* motionrt_getinbuff();

/******************************************************
Description:    // 用于远程IO刷新输入状态
Input:          //ionum：io口号 ； iostate：io口状态
Output:         //
Return:         //成功返回0
*******************************************************/
uint32 motionrt_inupdate(uint32 ionum, int iostate);

/******************************************************
Description:    // 设置io口输出状态
Input:          //ionum：io口号 ； iostate：io口状态
Output:         //
Return:         //成功返回0
*******************************************************/
uint32 motionrt_setop(uint32 ionum, int iostate);

/******************************************************
Description:    // 读取当前输出状态
Input:          //ionum：io口号 ； 
Output:         //
Return:         //输出io口状态
*******************************************************/
int motionrt_getop(uint32 ionum);

/******************************************************
Description:    // 读取模拟量输入
Input:          //ionum：模拟量输入口号 ；
Output:         //
Return:         //模拟量输入值
*******************************************************/
float motionrt_getad(uint32 ionum);

/******************************************************
Description:    // 用于远程IO刷新输入状态
Input:          //ionum：模拟量输入口号 ；
                  uivalue：远程io输入状态；
Output:         //
Return:         //成功返回0
*******************************************************/
uint32 motionrt_adupdate(uint32 ionum, float uivalue);

/******************************************************
Description:    //读取da
Input:          //ionum：输入口号 ；
Output:         //
Return:         //da值
*******************************************************/
float motionrt_getda(uint32 ionum);

/******************************************************
Description:    // 设置da aout
Input:          //ionum：输入口号 ；uidavalue：ad值
Output:         //
Return:         //成功返回0
*******************************************************/
uint32 motionrt_setda(uint32 ionum, float uidavalue);

/******************************************************
Description:    //精准指定时间输出op
Input:          //ionum：输出口号 ；
                  iostate:输出口状态；
                  ticksafter:当前ticks以后多少个；
				  usaftertick：tick起始后的us时间
Output:         //
Return:         //成功返回0
*******************************************************/
uint32 motionrt_sethwop(uint32 ionum, int iostate, uint32 ticksafter, uint32 usaftertick);

/******************************************************
Description:    // 读取当前的RTBASIC任务号
Input:          //
Output:         //
Return:         //当前的RTBASIC任务号
*******************************************************/
int motionrt_getcurtaskid();

/***************ZV视觉相关*******************/
// zv视觉特殊函数, ZVOBJ转换为内部CFUNC使用编号.
int motionrt_zvobj_getnum(const char* sname);

// zvboj 转换为CFUNC使用的内部编号
int motionrt_zvobj_zindexgetnum(int izindexnum);


/*************************************************PDOBUFF驱动相关*****************************************************************/
/*********************************************************************************************************************************
Description:    // 发送 pdobuff 修改
Input:          // islot：槽位； node：设备编号； iindex：数据字典编；,  isubindex：子编号；ibytes：数据类型； valueptr：数据指针
Output:         //
Return:         //错误码
*********************************************************************************************************************************/
uint32 motionrt_pdobuff_write(int islot, int inode, int  iindex, int isubindex, int ibytes, char *valueptr);

/********************************************************************************************************************************
Description:    // 接收 pdobuff 读取
Input:          // islot：槽位； node：设备编号； iindex：数据字典编；,  isubindex：子编号；ibytes：数据类型； valueptr：数据指针
Output:         //
Return:         //错误码
*********************************************************************************************************************************/
uint32 motionrt_pdobuff_read(int islot, int inode, int  iindex, int isubindex, int ibytes, char *valueptr);

/*******************************************************************************************************************************
Description:    // 发送 pdobuff 修改
Input:          // islot:槽位； inode:设备编号；  ioffset：字节偏移；   valueptr：数据指针
Output:         //
Return:         //错误码
********************************************************************************************************************************/
uint32 motionrt_pdobuff_write2(int islot, int inode, int ioffset, int ibytes, char *valueptr);

/*******************************************************************************************************************************
Description:    // 接收 pdobuff 读取
Input:          // islot:槽位； inode:设备编号；  ioffset：字节偏移；   valueptr：数据指针
Output:         //
Return:         //错误码
 ******************************************************************************************************************************/
uint32 motionrt_pdobuff_read2(int islot, int inode, int ioffset, int ibytes, char *valueptr);






















#define SOFRAME_FRAME_NUM  1      //用户自定义个数
#define SOFRAME_TABLE_NUM  100    //用户自定义个数


// zmc控制器, so调用的时候传递过去, 只读
// 函数指针NULL时,表示预留
typedef struct strsozmcDisp{
    // socall 接口版本, 1
    uint32  m_iSoCallVersion;
    
    // 硬件id
    uint32  m_iHardID;
    // 唯一编号
    uint32  m_iSerialID;

    //固件版本
    uint32  m_iVersionDate;
    float   m_fVersion;
    
    // 最大规格, 不是实际的输入输出
    uint32   m_iMaxInes;
    uint32   m_iMaxOutes;
    uint32   m_iMaxAdes;
    uint32   m_iMaxDaes;

    //基本规格
    uint32  m_iTotalAxies;
    uint32  m_iTotalTaskes;  // 不包含pc
    uint32  m_iTotalTimeres;  // 
    uint32  m_iTotalVRs;    
    uint32  m_iTotalModbus4x;    
    uint32  m_iTotalModbus0x;    
    uint32  m_iTotalHmies;  //hmi

    // IO读写, 不能在非主实时任务外使用
    uint8 (*m_pfuncReadIn)(uint32 iindex);   // 
    uint8 (*m_pfuncReadOut)(uint32 iindex);   // 
    TYPE_TABLE (*m_pfuncReadAd)(uint32 iindex);   // 
    TYPE_TABLE (*m_pfuncReadDa)(uint32 iindex);   //     
    uint32  (*m_pfuncWriteOut)(uint32 iindex, uint8 istate);   // 
    uint32  (*m_pfuncWriteDa)(uint32 iindex, TYPE_TABLE davalue);   // 
    
    
    // 32 还是64double
    uint32 m_iTableBites;
	uint32 (*m_pfuncGetTableSize)(void);   //读取table大小
	TYPE_TABLE* (*m_pfuncGetTablePtr)(uint32 itablenum);   //读取table指针
	
	uint32 (*m_pfuncWriteTable)(uint32 itablenum, TYPE_TABLE dValue);   //	
	TYPE_TABLE (*m_pfuncReadTable)(uint32 itablenum);   //

    // vr本身是float类型的
	uint32 (*m_pfuncWriteVr)(uint32 iindex, TYPE_TABLE dValue);   //	
	TYPE_TABLE (*m_pfuncReadVr)(uint32 iindex);   //
    // 整数方式访问
	uint32 (*m_pfuncWriteVrInt)(uint32 iindex, int32 iValue);   //	
	int32 (*m_pfuncReadVrInt)(uint32 iindex);   //


    // set参数读写
    uint32 (*m_pfuncSetNametoId)(const char *psSetName, uint32* m_pSetId);   // 字符串转换为参数编号
    uint32 (*m_pfuncSetWrite)(const char *psSetName, uint32 iSetIndex, TYPE_TABLE fSetValue);   //    
    uint32 (*m_pfuncSetWrite2)(uint32 iSetId, uint32 iSetIndex, TYPE_TABLE fSetValue);   // 通过参数编号来快速访问
    TYPE_TABLE (*m_pfuncSetRead)(const char *psSetName, uint32 iSetIndex);   //    
    TYPE_TABLE (*m_pfuncSetRead2)(uint32 iSetId, uint32 iSetIndex);   //    // 通过参数编号来快速访问

    // 快速参数访问
    TYPE_TABLE (*m_pfuncDposRead)(uint32 iAxisIndex);   // 
    TYPE_TABLE (*m_pfuncMposRead)(uint32 iAxisIndex);   // 
    TYPE_TABLE (*m_pfuncVpspeedRead)(uint32 iAxisIndex);   //  
    TYPE_TABLE (*m_pfuncMspeedRead)(uint32 iAxisIndex);   // 
    TYPE_TABLE (*m_pfuncUnitsRead)(uint32 iAxisIndex);   // 脉冲当量
    
    // 周期数读取
    uint32 (*m_pfuncCyclesRead)();   // 
    
    // 调试输出
    uint32 (*m_pfuncDebugPrint)(const char *pstring, uint32 ilen);   // 
    // excute执行时输出
    uint32 (*m_pfuncExcutePrint)(const char *pstring, uint32 ilen);   // 


    // basic任务信息
	uint32 (*m_pfuncGetTaskLine)(uint32 itasknum);   //
	uint32 (*m_pfuncGetTaskFileName)(uint32 itasknum, char *pname, uint32 ibuffsize);   //

    //
    // 预留: basic自己读取参数, 只能行内读取
    // 只能cmd扩展函数里面使用
	uint32 (*m_pfuncTaskReadChar)(uint32 itasknum, char *pch);   //
	uint32 (*m_pfuncTaskReadQuote)(int32 itask, char *pstring, int32 imax);   // 读取字符串
	uint32 (*m_pfuncTaskReadVariable)(int32 itask, double *pdouble);   // 读取变量

    // plc寄存器, 预留
	uint32 (*m_pfuncPlcWriteRegister)(const char *pRegName, uint8 iRegbites, uint32 start, uint32 inum, uint8* pdata);   //
	uint32 (*m_pfuncPlcReadRegister)(const char *pRegName, uint8 iRegbites, uint32 start, uint32 inum, uint8* pdata);   //

    // hmi 的相关操作函数
	uint32 (*m_pfuncHmiGetInfo)(int32 ihminum, int32 *pwidth, int32 *pheight, int32 *pBites);   //
	uint16* (*m_pfuncHmiGetBuffAddr)(int32 ihminum);   //取缓冲地址, 一般都是16位颜色
	uint32 (*m_pfuncHmiInvalidRange)(int32 ihminum, int32 ix, int32 iy, int32 iwidth, int32 iheight); // 设置指定区域无效, 需要绘图
	uint32 (*m_pfuncHmiGetInvalidRange)(int32 ihminum, int32 iblock, int32 *pix, int32 *piy, int32 *piwidth, int32 *piheight); // 
	uint32 (*m_pfuncHmiSetMouseState)(int ihminum, int32 iMouseState, int32 iPosx, int32 iPosy); // 
	uint32 (*m_pfuncHmiSetKeyState)(int ihminum, int32 iphykey, int32 istate); // 
	uint32 (*m_pfuncHmiSetVKeyState)(int ihminum, int32 ivkey, int32 istate); // 
	uint32 (*m_pfuncHmiGetMouseState)(int ihminum, int32 *piMouseState, int32 *piPosx, int32* piPosy); // 	
	uint32 (*m_pfuncHmiGetKeyState)(int32 ihminum, int32 ikey, int32*pkeystate, int32 *pkeyEvent);   //
	uint32 (*m_pfuncHmiGetVKeyState)(int32 ihminum, int32 ikey, int32*pkeystate, int32 *pkeyEvent);   //
	
    // ZV锁存图像的操作
	uint32 (*m_pfuncHmiSetZVLatchInfo)(int32 izvlat, int32 iwidth, int32 iheight);   //
	uint32 (*m_pfuncHmiSetZVLatchWrProtect)(int32 izvlat, int32 bifprotect);   //
	uint32 (*m_pfuncHmiGetZVLatchInfo)(int32 izvlat, int32 *pwidth, int32 *pheight, int32 *pBites);   //
	uint16* (*m_pfuncHmiGetZVLatchAddr)(int32 izvlat);   //一般都是16位颜色


    // 预留
	uint32 (*m_pfuncWriteDeviceReg)(uint32 idevnum, uint32 iregtype, uint32 iregindex,  TYPE_TABLE dValue);   //
	TYPE_TABLE (*m_pfuncReadDeviceReg)(uint32 idevnum, uint32 iregtype, uint32 iregindex);   //	


    // ecat 快速访问
    // 必须启动后才能读写
    uint32 (*m_pfuncEcatReadNodeCount)(uint32 islot);   //
    uint32 (*m_pfuncEcatReadAxisCount)(uint32 islot);   //  轴数
    uint32 (*m_pfuncEcatReadNodeInfo)(uint32 islot, uint32 inode, int32 isel);   //
    uint32 (*m_pfuncEcatReadNodeAxisCount)(uint32 islot, uint32 inode);   //  轴数
    // 部分node pdo无法读取
    uint32 (*m_pfuncEcatReadNodePdoCount)(uint32 islot, uint32 inode);   //  pdo的个数
    uint32 (*m_pfuncEcatReadNodePdoBytes)(uint32 islot, uint32 inode);   //  pdo的字节数
    // pdo信息读取
    uint32 (*m_pfuncEcatReadNodePdo)(uint32 islot, uint32 inode, uint32 inum);   //  

    // pdo读写    
	uint32 (*m_pfuncEcatPdoWrite)(uint32 islot, uint32 inode, uint32  iindex, uint32  isubindex, uint32 itype, uint32 ivalue);   //
	uint32 (*m_pfuncEcatPdoRead)(uint32 islot, uint32 inode, uint32  iindex, uint32  isubindex, uint32 itype);   //
    // 按片写入 读取
	uint32 (*m_pfuncEcatPdoWrite2)(uint32 islot, uint32 inode, const uint8 *pbuff, uint32 ilen);   
	uint32 (*m_pfuncEcatPdoRead2)(uint32 islot, uint32 inode, uint8 *pbuff, uint32 ilenmax);   //


    // 其他预留
    void (*m_pfuncReserve1)(void);   //
    void (*m_pfuncReserve2)(void);   //
    void (*m_pfuncReserve3)(void);   //
    void (*m_pfuncReserve4)(void);   //
    void (*m_pfuncReserve5)(void);   //

    // 更多预留
    void *ptrreserve[32];
    
}struct_soZmcDisp;



//鏃嬭浆绫诲瀷
enum ZMOTION_FRAME_ROTATETYPE
{
    FRAME_ROTATE_TYPENO = 0, // 涓嶆敮鎸?
    
    FRAME_ROTATE_TYPEXY = 2, // XY 鏃嬭浆

    //
    FRAME_ROTATE_TYPEXYZ = 3, // xyz鏃嬭浆
    
    //
    FRAME_ROTATE_TYPEXYZRXRYRZ = 6, // xyz鏃嬭浆, RXRYRZ 涔熻窡鐫€鍙樺寲
    
    //
    FRAME_ROTATE_TYPESO = 100,   //鑷畾涔?
    
};


//frame的一些基本信息, 只读
typedef struct strsoFrameStatus{

    //
    uint32  m_iFrameId;
    //
    int32   m_iHand;    //当前姿态, init 的时候可以修改, 其他函数不能修改
    uint32  m_iTotalHandes;  // 所有姿态数
    
    //
    uint32  m_iTotalAxisesNoAux;
    uint32  m_iTotalAxisesAll;  // 带辅助轴

    // 记录是否使用了旋转功能
    uint32  m_iRotateType;  // basic传过来   
    uint32  m_bifRotate;   
    TYPE_FRAME  m_aRotatePara[6];   //最多6个旋转, so不能修改
    
    //轴号列表
    TYPE_AXIS  *m_pJointAxisList;
    TYPE_AXIS  *m_pVirtuAxisList;

    // 存储参数指针，对应table不能修改, 否则被变动.
    TYPE_TABLE * m_pInitPara; 

    // 唯一so可以修改的指针
    void    *m_pPrivate;
    
}struct_soFrameStatus;



//机械手初始化 (函数名SOFRAME_INIT1000)
	// pzmc	  控制器描述结构指针
	// pframe	  机械手基本状态指针
// pParaList Table参数列表
// 返回值, 成功与否, 0-OK
typedef uint32 (*PSoFrameInitFunc)(struct_soZmcDisp *pzmc,  struct_soFrameStatus* pframe, TYPE_TABLE* pParaList);

// 正解函数 (函数名SOFRAME_TRANS1000)
	// pzmc	  控制器描述结构指针
	// pframe	  机械手基本状态指针
	// pfJointPulsein	输入关节脉冲坐标
	// pHand	  输出当前姿态
	// pfWorldout输出units单位世界坐标WPOS
// 返回值, 成功与否, 0-OK
typedef uint32 (*PSoFrameTransFunc)(struct_soZmcDisp *pzmc,  struct_soFrameStatus* pframe, TYPE_FRAME*pfJointPulsein, int32 *pHand, TYPE_FRAME* pfWorldout);

//逆解函数 (函数名SOFRAME_RETRANS1000)
	// pzmc	  控制器描述结构指针
	// pframe	  机械手基本状态指针
	// pfWorldin	输入世界坐标units单位
	// ihand	  输入坐标对应姿态, -1表示使用当前姿态.
	// pfJointPulseout	输出关节脉冲坐标
// 返回值, 成功与否, 0-OK
typedef uint32 (*PSoFrameReTransFunc)(struct_soZmcDisp *pzmc,  struct_soFrameStatus* pframe, TYPE_FRAME *pfWorldin, int32 ihand, TYPE_FRAME* pfJointPulseout);

//自定义坐标旋转函数DPOS转换为WPOS (函数名SOFRAME_ROTATETOWPOS1000)
	// pzmc	  控制器描述结构指针
	// pframe	  机械手基本状态指针
// pfRotate  坐标平移旋转的参数, 依次为XYZ,RX,RY,RZ
// pfin 	  输入虚拟轴坐标列表, units单位
// pfout 	  输出世界坐标列表, units单位
// 返回值, 成功与否, 0-OK
typedef uint32 (*PSoFrameRotateDpostoWpos)(struct_soZmcDisp *pzmc,  struct_soFrameStatus* pframe, TYPE_FRAME *pfRotate, TYPE_FRAME *pfin, TYPE_FRAME* pfout);

//自定义坐标旋转函数: WPOS转换为DPOS (函数名SOFRAME_ROTATETODPOS1000)
	// pzmc	  控制器描述结构指针
	// pframe	  机械手基本状态指针
// pfRotate  坐标旋转的参数
// pfin 	  输入世界坐标列表, units单位
// pfout 	  输出虚拟轴坐标列表, units单位
// 返回值, 成功与否, 0-OK
typedef uint32 (*PSoFrameRotateWpostoDpos)(struct_soZmcDisp *pzmc,  struct_soFrameStatus* pframe, TYPE_FRAME *pfRotate, TYPE_FRAME *pfin, TYPE_FRAME* pfout);

//用户自定义拓展结构体，用于记录机械手信息
typedef struct str_userframeinfo
{
    //用户正逆解全局变量
    //结构参数
    fp32 b1[3];
    fp32 b2[3];
    fp32 b3[3];
    fp32 b4[3];
    fp32 b5[3];
    fp32 m1[3];
    fp32 m2[3];
    fp32 m3[3];
    fp32 m4[3];
    fp32 m5[3];

    fp32 b[5][3];
    fp32 m[5][3];
    fp32 limb0[5];

    //关节一单位(当前：mm)的脉冲数
    fp64 u_j1;
    fp64 u_j2;
    fp64 u_j3;
    fp64 u_j4;
    fp64 u_j5;
    // 尺寸系数
    fp32 LENGTH_UNIT;
    // 零位位姿
    fp64 zero_pose[5];
    // R副偏差
    fp32 d_bais;
    //虚拟轴的unit
    fp64 m_pulsev; 

    //table数据存储
    fp64 m_table[SOFRAME_TABLE_NUM];
    
    //数据更新标志位
    uint8 m_getflag;
    
    //....
    
}struct_userframeinfo;


#endif
