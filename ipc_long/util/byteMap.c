#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>    // for socket
#include <netinet/in.h>    // for sockaddr_in
#include <netdb.h>
#include <iconv.h>         //for code convert
#include <time.h>          //for time
#include "cJSON.h"
#include "byteMap.h"
#include "base64.h"

#define BYTEMAP_BUFFER_SIZE 30000
#define LONG_TEXT_BUFFER_SIZE 3000
#define SMALL_TEXT_BUFFER_SIZE 256

const char ADD_BEGIN_END_CHAR1 = 0;
const char READ_BEGIN1 = 0xff;
const char READ_END1 = 0xfe;
const char END_CHAR1 = '\0'; 

const char * KEY_MESSAGE_TYPE = "messageType";
const char * KEY_PACKAGE_NAME ="packageName";
const char * KEY_CLASS_NAME = "activityName";
const char * KEY_CLASS_TITLE = "title";
const char * KEY_BYTEMAP = "byteIcon";
const char * KEY_ALL_APPS = "allApps";

const short jsonContainPackage[] = {MESSAGE_LINUX_GETONEAPP,MESSAGE_ANDROID_SENDONEAPP,MESSAGE_ANDROID_ADDONE,MESSAGE_ANDROID_UPDATEONE,MESSAGE_ANDROID_REMOVEONE,MESSAGE_ANDROID_APP_START_SUCCESS,MESSAGE_ANDROID_APP_RESUME,MESSAGE_ANDROID_APP_START_FAIL,MESSAGE_ANDROID_APP_BACK,MESSAGE_ANDROID_APP_EXIT};
const short jsonContainClass[] = {MESSAGE_LINUX_GETONEAPP,MESSAGE_ANDROID_SENDONEAPP,MESSAGE_ANDROID_ADDONE,MESSAGE_ANDROID_UPDATEONE,MESSAGE_ANDROID_APP_START_SUCCESS,MESSAGE_ANDROID_APP_RESUME,MESSAGE_ANDROID_APP_START_FAIL,MESSAGE_ANDROID_APP_BACK,MESSAGE_ANDROID_APP_EXIT};
const short jsonContainTitle[] ={MESSAGE_ANDROID_SENDONEAPP,MESSAGE_ANDROID_ADDONE,MESSAGE_ANDROID_UPDATEONE};
const short jsonContainBytemap[] ={MESSAGE_ANDROID_SENDONEAPP,MESSAGE_ANDROID_ADDONE,MESSAGE_ANDROID_UPDATEONE};
const short jsonContainAllAppInfo[]={MESSAGE_ANDROID_APPBASICINFO};

int sd = -1; //for single instance

void print1(){
  printf(">>byteMap.print1()1111 \n");
}
long getNowMills(){
    struct timeval time;
    gettimeofday(&time,NULL);
    long mills =  (1000000*time.tv_sec+time.tv_usec)/1000;   
    //printf("now mills: %ld \n",mills);
    return mills;
}

//for decode begin--------------------
//dec ascii char to char
unsigned char decToChar(char num){
   char out[2];
   sprintf(out,"%c",num);
   return out[0];
}
void ascArrToStr(char* dest,char buffer[],int len){
  int i;
  for(i=0;i<len;i++){
    dest[i]=decToChar(buffer[i]);
  }
  printf(">>ascArrToStr>>%s >>sizeof(dest):%ld >>sizeof(char):%ld\n",dest,sizeof(dest),sizeof(char));
}
int code_convert(char *inbuf,size_t inlen,char *outbuf,size_t outlen)  
{  
        iconv_t cd;  
        int rc;  
        char **pin = &inbuf;  
        char **pout = &outbuf;  
  
        cd = iconv_open("GBK","UTF-8");  
        if (cd==0)  
           return -1;  
        memset(outbuf,0,outlen);  
        if (iconv(cd,pin,&inlen,pout,&outlen) == -1)  
           return -1;  
        iconv_close(cd);  
        return 0;  
}  

//for decode end
//file operate begin ---------
//file operate begin ---------
void checkOrCreateDir(char * dir){
 if(-1 == access(dir,0)){
    int status = -1; 
    status = mkdir(dir,0777);
    printf(">>checkOrCreateDir()>>dir %s not exisit!, create dir status is:%d!!\n",dir,status);
  }   
}

void writeTxtFile(char buffer[],int n){
  FILE * file;
  char path[256] = "images/";
  checkOrCreateDir(path);
  file = fopen("images/1.txt","ab+"); //b is byte
  fwrite(buffer,n,1,file);
  fclose(file);
}
void writeImageFile(char * buffer,int n,char * path){ 
  FILE * file;
  printf(">>call writeImageFile() write size:%d>>write path is:%s \n",n,path);
  file = fopen(path,"w"); //w clear then write
  fwrite(buffer,n,1,file);
  fclose(file);  
}

void writePngFileBase64(char * buf,int lenBuf,char * title,int lenTitle){
  int lenDec = -1;
  char * convert = base64_decode(buf,lenBuf,&lenDec);
  int lenBase64 = strlen(convert);
  char path[256] = "images/";
  checkOrCreateDir(path);
  char dot[] =".png";
  strcat(path,title);
  strcat(path,dot);
  printf(">>writePngFileBase64>>base64 decode buf size is:%d >>origin buff size is:%d\n",lenBase64,lenBuf);
  printf(">>writePngFileBase64>>new path is:%s>>base64 decode buf new size is:%d \n",path,lenDec);
  writeImageFile(convert,lenDec,path); 
}

//file operate end 
//utils begin -------------
short isInArray(short num,const short * arr,int len){
  //printf(">>isInArray num:%d  len:%d \n",num,len);
  if(arr != NULL){
    int i;
    for(i=0;i<len;i++){
       if(arr[i]==num)
	 return 1;
    }
  }
  return 0;
}
int createSocket(){
    struct sockaddr_in pin;
    struct hostent *nlp_host;
    int socketId; 
    char host_name[256];
    int port;
    //test
    //strcpy(host_name,"localhost");
    strcpy(host_name,"192.168.31.81");
    port= 4700;
    if((nlp_host=gethostbyname(host_name))==0){
       printf("Resolve Error!\n");
    }   
    //set ip,port
    bzero(&pin,sizeof(pin));
    pin.sin_family=AF_INET;                 //AF_INET¿¿¿¿IPv4
    pin.sin_addr.s_addr=htonl(INADDR_ANY);  
    pin.sin_addr.s_addr=((struct in_addr *)(nlp_host->h_addr))->s_addr;
    pin.sin_port=htons(port);

    //create client socket
    socketId = socket(AF_INET,SOCK_STREAM,0);
    printf("try createSocket  connecting:%s port:%d socketId:%d \n",host_name,port,socketId);
    //connect reomot server
    if(connect(socketId,(struct sockaddr*)&pin,sizeof(pin))==-1){
      printf("Connect Error!\n");
    }   
    return socketId;
}

void sendMsgToAndroid(int * send_fd,char * jsonString){
   int len = strlen(jsonString);
   sd  = *send_fd;
   printf(">>sendMsgToAndroid()>>000 socket id is:%d>>ADD_BEGIN_END_CHAR1 is:%d \n",sd,ADD_BEGIN_END_CHAR1);
   printf(">>sendMsgToAndroid()>>111 jsonString length is:%ld >>jsonString is:%s \n",strlen(jsonString),jsonString);
   int i,j=0;
   const char ASCII32 = 32;
   char newStr[len+3];
 if(!ADD_BEGIN_END_CHAR1){//check whether add begin end char
    for(i=0;i<len;i++){ 
     if(jsonString[i]>ASCII32){  //filter out control chars
       newStr[j]=jsonString[i];
       j++;
     }   
   }
   newStr[j] = '\0'; 
 }else{
   newStr[j] = READ_BEGIN1; //add head 0xff -1
   j++;
   for(i=0;i<len;i++){ 
     if(jsonString[i]>ASCII32){
       newStr[j]=jsonString[i];
       j++;
     }   
   }
   newStr[j++] = READ_END1;  //add tail 0xfe -2
   newStr[j] = '\0';
/*   for(i=0;i<j;i++){
    printf(">>sendMsgToAndroid() newStr[%d]=%d char is:%c \n",i,newStr[i],newStr[i]);
  }
  if(sd == -1){
     printf(">>sendMsgToAndroid() sd == -1 should createSocket! #### \n");
     sd = createSocket();
   }  */   

 }
 printf(">>sendMsgToAndroid()>>222 call send() sd is:%d >>newStr length is:%d >>newStr is:%s  \n",sd,j,newStr);
 send(sd,newStr,strlen(newStr)*sizeof(char),0);
   //close(sd); //if in two way remove this

}
//for heartbeat message begin
typedef struct Msg{
  int * fd; 
  char * jsonStr;
}JsonMsg;

void * sendHeartBeatMsg(void * jsonMsg){
  JsonMsg jMsg = *(JsonMsg *)jsonMsg;
  printf("sendHeartBeatMsg() 111 \n");
  sleep(30);
  printf("sendHeartBeatMsg() 222 >>fd is:%d >>jsonStr is %s \n",*jMsg.fd,jMsg.jsonStr);
  while(1){
    printf("sendHeartBeatMsg() ~~\n");
    //send(jMsg.fd,jMsg.jsonStr,strlen(jMsg.jsonStr)*sizeof(char),0);
    sendMsgToAndroid(jMsg.fd,jMsg.jsonStr); 
    sleep(300);
  }
}
sendHeartBeatInstak(JsonMsg * jMsg){
  printf("sendHeartBeatInstak \n");
  pthread_t thread;
  pthread_create(&thread,NULL,sendHeartBeatMsg,jMsg);
  //pthread_join(thread,NULL);
}



//test begin----------------------------
void testJson(char* buf,int n,int * send_fd){ //add send_fd
   int i,j;
   printf(">>testJson() ~000>>buf size:%d >>buf is:%s \n",n,buf);
   printf(">>testJson() ~000>>buf size:%d  \n",n);
   if(n<=0)
     return;
   char bufNew[n];
   for(i=0;i<n;i++){
     bufNew[i] = buf[i];
   } 
/*    //test begin lilei
   char ** str;
   char a[2] ="ab";
   int len;
   len = base64DecodeStringRemoveEndZero(a,str);
   printf(">>testJson() 111.2.1 a is:%s\n",a);
   printf(">>testJson() 111.2>>decode str is:%s >>len:%d \n",*str,len);
   return;
   //test end  */
   char packageName[SMALL_TEXT_BUFFER_SIZE];
   char className[SMALL_TEXT_BUFFER_SIZE];
   char title[SMALL_TEXT_BUFFER_SIZE];
   char byteMap[BYTEMAP_BUFFER_SIZE];
   char * jsonStr;
    
   cJSON *fmt = NULL,*JSONroot = NULL,*mJSONroot;
   JSONroot = cJSON_Parse(bufNew);
   int num = cJSON_GetArraySize(JSONroot);//check arrays 
   printf(">>testJson() 222>>json root size is:%d \n",num);
   
   fmt = cJSON_GetObjectItem(JSONroot,KEY_MESSAGE_TYPE);
   int msgType = fmt->valueint;
   printf(">>testJson() 33>>msgType is:%d \n",msgType);
    
   
   if(isInArray(msgType,jsonContainPackage,sizeof(jsonContainPackage)/sizeof(short))){
     fmt = cJSON_GetObjectItem(JSONroot,KEY_PACKAGE_NAME);
     bzero(packageName,SMALL_TEXT_BUFFER_SIZE);     //clear packageName
     snprintf(packageName,SMALL_TEXT_BUFFER_SIZE,"%s",fmt->valuestring);
     printf(">>testJson() 33>>packageName is:%s \n",packageName);
   }
   
   if(isInArray(msgType,jsonContainClass,sizeof(jsonContainClass)/sizeof(short))){
     fmt = cJSON_GetObjectItem(JSONroot,KEY_CLASS_NAME);
     bzero(className,SMALL_TEXT_BUFFER_SIZE);         //clear className
     snprintf(className,SMALL_TEXT_BUFFER_SIZE,"%s",fmt->valuestring);
     printf(">>testJson() 33>className is:%s>>className len is:%ld \n",className,strlen(className));
   }
   if(isInArray(msgType,jsonContainTitle,sizeof(jsonContainTitle)/sizeof(short))){
     fmt = cJSON_GetObjectItem(JSONroot,KEY_CLASS_TITLE);
     bzero(title,SMALL_TEXT_BUFFER_SIZE);             //clear title
     snprintf(title,SMALL_TEXT_BUFFER_SIZE,"%s",fmt->valuestring);
     int length  = strlen(title);
     int lenDec = -1;
     char * convert = base64_decode(title,length,&lenDec);
     int lenBase64 = strlen(convert);
     printf(">>testJson() 33>>base64 decode title is:%s >>title is:%s \n",convert,title); 
     int i;
     printf(">>testJson() 33>>base64 decode title length is:%d >>origin title length is:%d \n",lenBase64,length);
     for(i = 0;i<lenBase64;i++){
       printf(">>testJson() 33>titile convert[%d]:%d \n",i,convert[i]);
     }
   }
   
   if(isInArray(msgType,jsonContainBytemap,sizeof(jsonContainBytemap)/sizeof(short))){
     fmt = cJSON_GetObjectItem(JSONroot,KEY_BYTEMAP);
     bzero(byteMap,BYTEMAP_BUFFER_SIZE);               //clear byteMap
     snprintf(byteMap,BYTEMAP_BUFFER_SIZE,"%s",fmt->valuestring);
     //printf(">>testJson() 33>byteMap is:%s \n",byteMap);
     printf(">>testJson() 33>byteMap length is:%ld \n",strlen(byteMap));
     writePngFileBase64(byteMap,strlen(byteMap),className,strlen(className));//save image
     printf(">>testJson() 33>byteMap of %s save end>>time is:%ld \n",className,getNowMills());
   }
   if(isInArray(msgType,jsonContainAllAppInfo,sizeof(jsonContainAllAppInfo)/sizeof(short))){
     fmt = cJSON_GetObjectItem(JSONroot,KEY_ALL_APPS); //this fmt is array [...]
     //char allAppInfo[LONG_TEXT_BUFFER_SIZE];
     int arraySize = cJSON_GetArraySize(fmt);
     //snprintf(allAppInfo,LONG_TEXT_BUFFER_SIZE,"%s",fmt->valuestring);
     printf(">>testJson() 33>all app size is:%d \n",arraySize);
     cJSON * jsonApp = fmt->child;
     int appNum = 0;
     while(jsonApp != NULL){
       int count = cJSON_GetArraySize(jsonApp);

       fmt = cJSON_GetObjectItem(jsonApp,KEY_PACKAGE_NAME); //this fmt is object
       bzero(packageName,SMALL_TEXT_BUFFER_SIZE);           //clear packageName
       snprintf(packageName,SMALL_TEXT_BUFFER_SIZE,"%s",fmt->valuestring);
       
       fmt = cJSON_GetObjectItem(jsonApp,KEY_CLASS_NAME);       
       bzero(className,SMALL_TEXT_BUFFER_SIZE);             //clear className
       snprintf(className,SMALL_TEXT_BUFFER_SIZE,"%s",fmt->valuestring);
       jsonApp = jsonApp->next;
       //printf(">>testJson() 33>>packageName:%s >>className:%s \n",packageName,className);
       //get one app
       appNum++;
       if(appNum <= 100){
         printf(">>testJson() 33>>packageName:%s >>className:%s \n",packageName,className);
         mJSONroot = cJSON_CreateObject();  //for send to android
         cJSON_AddItemToObject(mJSONroot,KEY_MESSAGE_TYPE,cJSON_CreateNumber(MESSAGE_LINUX_GETONEAPP));
         cJSON_AddItemToObject(mJSONroot,KEY_PACKAGE_NAME,cJSON_CreateString(packageName));
         cJSON_AddItemToObject(mJSONroot,KEY_CLASS_NAME,cJSON_CreateString(className)); 
         jsonStr = cJSON_Print(mJSONroot);
         cJSON_Delete(mJSONroot);
         printf(">>testJson() 33>>appNum == %d~~~~~~~~~~~~~~should send\n",appNum);
         sendMsgToAndroid(send_fd,jsonStr); //if get all app should not do below
       }
     }
    // cJSON_Delete(JSONroot);
    // return;   //if get all app should not do below
   }
   //perse action
   mJSONroot = cJSON_CreateObject();  //for send to android
   switch(msgType){
     case MESSAGE_ANDROID_READY:
        cJSON_AddItemToObject(mJSONroot,KEY_MESSAGE_TYPE,cJSON_CreateNumber(MESSAGE_LINUX_GETALL));
        jsonStr = cJSON_Print(mJSONroot);  
        //printf(">>testJson() 444>>should send message is:%s >>length is:%ld \n",jsonStr,strlen(jsonStr));      
        sendMsgToAndroid(send_fd,jsonStr);
        //for heartbeat begin do not send heartbeat message any more
      /*  mJSONroot = cJSON_CreateObject();
        cJSON_AddItemToObject(mJSONroot,KEY_MESSAGE_TYPE,cJSON_CreateNumber(MESSAGE_ANDROID_HEART_BEAT));
        jsonStr = cJSON_Print(mJSONroot);
        JsonMsg * msg = (JsonMsg *)malloc(sizeof(JsonMsg));
        msg->fd = send_fd;
        msg->jsonStr = jsonStr;
	sendHeartBeatInstak(msg);
        //for heartbeat end  */  
	break;
     
     default:
     ;   
   }
   //cJSON_Delete(fmt);
   cJSON_Delete(JSONroot);
   cJSON_Delete(mJSONroot);
}
void test1(){
  char buffer[] = {97,98,99,100};
  printf(">>call test1() \n");
  writeTxtFile(buffer,sizeof(buffer)); 
}
//test end -----------------
