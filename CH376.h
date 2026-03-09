/*
	Author: Diego Arnaudin 

	Problems and solutions:
		- Won't open file ([FileOpen]): Format with FAT32 or FAT16, the device does not recognize NTFS format.

*/

#ifndef CH376_H
#define CH376_H

/* Configuration defines */
#define RETRY_DLY 50 
#define RETRY_TIMES 3

/* responses  */
#define ANSW_USB_INT_SUCCESS 0x14
#define ANSW_RET_SUCCESS 0x51
#define ANSW_USB_INT_DISK_READ 0x1d

/* commands */
#define CMD_CHECK_EXIST 0x06
#define CMD_RESET_ALL 0x05
#define CMD_SET_USB_MODE 0x15
#define CMD_DISK_CONNECT 0x30
#define CMD_DISK_MOUNT 0x31
#define CMD_FILE_OPEN 0x32
#define CMD_SET_FILE_NAME 0x2F
#define CMD_BYTE_READ 0x3A
#define CMD_RD_USB_DATA0 0x27
#define CMD_BYTE_RD_GO 0x3b
#define CMD_SET_BAUDRATE 0x02

/* prototypes declaration */

const char _start[]={0x57,0xab,0x00};


#ifndef CH376_SPI_HW
	#define WriteCH376(a) fputc(a,PORTCH376)
	#define ReadCH376() fgetc(PORTCH376)
	#define StartCmd() 
#else
	#define CH376Xfer(x) spi_read(x)
	#define StartCmd() fprintf(PORTCH376,"%s",_start)
#endif	

char SetMode(char mode);
char ResetAll();
char CheckExists();
char DiskConnect();
char DiskMount();
char SetFileName(char*);
char FileOpen(char*);
char LoadFile(char*);
char ReadBlock(char*, char);
char ByteRead();

char InitDevice();

typedef char (*fun_ptr)();
char TryNTimes(fun_ptr func){
   char r=0x00, retrys = 0;      
   do{
      r = func();
	  delay_ms(RETRY_DLY);
   }while(!r && (++retrys)<RETRY_TIMES);
   
   return r;
}

typedef char (*fun_ptrP)(char);
char TryNTimesP(fun_ptrP func,char p1){
   char r=0x00, retrys = 0;      
   do{
      r = func(p1);
	  delay_ms(RETRY_DLY);
   }while(!r && (++retrys)<RETRY_TIMES);
   
   return r;
}


/* functions definitions */
char SetMode(char mode){
   dbg("[SetMode]\n\r");
   StartCmd();
   WriteCH376(CMD_SET_USB_MODE);
   WriteCH376(mode);
   delay_us(20);
   
   return ReadCH376() == ANSW_RET_SUCCESS;
}

char ResetAll(){
   dbg("[ResetAll]\n\r");
   StartCmd();
   WriteCH376(CMD_RESET_ALL);   
   delay_ms(50);
   
   return 1;
}

char CheckExists(){
   dbg("[CheckExists]\n\r");
   StartCmd();
   WriteCH376(CMD_CHECK_EXIST);
   WriteCH376(0x01);
   
   return ReadCH376() == 0xFE;
}

char DiskConnect(){
   dbg("[DiskConnect]\n\r");
   StartCmd();
   WriteCH376(CMD_DISK_CONNECT);
   
   return ReadCH376() == ANSW_USB_INT_SUCCESS;
}

long long GetFileSize(){
	dbg("[GetFileSize]\n\r");
	StartCmd();
	WriteCH376(0x0c);
	WriteCH376(0x68);
	char a=0,b=0,c=0,d=0;
	a=ReadCH376();
	b=ReadCH376();
	c=ReadCH376();
	d=ReadCH376();
	
	long long n = (long long) ((long long)d<<24) | ((long long)c<<16) |((long long)b<<8) | a;
	
	return n;
}

char DiskMount(){
	dbg("[DiskMount]\n\r");
	StartCmd();
	WriteCH376(CMD_DISK_MOUNT);

	return ReadCH376() == ANSW_USB_INT_SUCCESS;
}

char SetFileName(char *filename){
   dbg("[SetFileName]\n\r");
   StartCmd();
   WriteCH376(CMD_SET_FILE_NAME);
   
   int i=0;
   do{
      WriteCH376(filename[i]);
   }while(filename[i++]!='\0');
   
   return 1; 
}

char FileOpen(){
	dbg("[FileOpen]\n\r");
	StartCmd();
	WriteCH376(CMD_FILE_OPEN);

	return ReadCH376() == ANSW_USB_INT_SUCCESS;
}

char ByteRead(){
   dbg("[ByteRead]\n\r");
   char response = 0, retrys = 10;
   
   do{
	StartCmd();
	WriteCH376(CMD_BYTE_READ);   
	WriteCH376(0x40);
	WriteCH376(0x00);
	response = ReadCH376();
	retrys--;
   }while( response != ANSW_USB_INT_DISK_READ && retrys>0);
	
   return  response == ANSW_USB_INT_DISK_READ;
}

char ReadBlock(char* buff, char length){   
	int i=0;
	int size=0;

	dbg("[ReadBlock]\n\r");

	do{
		StartCmd();   
		WriteCH376(CMD_RD_USB_DATA0);
	
		i = size = ReadCH376();	
	}
	while(size == 0);	
		
	while(i--){
		*(buff++) = ReadCH376();
	}

	return size;
}


char ByteRdGo(){
   dbg("[ByteRdGo]\n\r");
   
   char response = 0, retrys = 10;
   do{
	StartCmd();
	WriteCH376(CMD_BYTE_RD_GO);
	response = ReadCH376();
	retrys--;
   }while( response != ANSW_USB_INT_SUCCESS && retrys>0);
   
   return response == ANSW_USB_INT_SUCCESS ;
   
}

char LoadFile(char *filename){
	int size = 0;
	dbg("(LoadFile)\n\r");
	
	SetFileName(filename);
	
	return FileOpen();
}

char ReadFile( char *buff ){
	int size = 0;
	
	dbg("\r[Read Block]\n\r");
	
	if (!ByteRead()) return 0;
	
	ReadBlock(buff,64);	

	return ByteRdGo();
}

char InitSPI(){
	setup_spi(SPI_MASTER | SPI_H_TO_L | SPI_XMIT_L_TO_H);
	#define sdcard_xfer(x)    spi_read(x)
	
	output_high(SDCARD_PIN_SELECT);
	output_drive(SDCARD_PIN_SELECT);
	delay_ms(200);
}

char InitDevice(){
	return (
		ResetAll() &&
		TryNTimes(&CheckExists) &&
		TryNTimesP(&SetMode, 0x06) &&
		TryNTimes(&DiskConnect) &&
		TryNTimes(&DiskMount) 
	);
}					

#endif /* CH376_H */
