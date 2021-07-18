#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#include "ev.h"
#include "cJSON.h"
#include "wt_json.h"

/*
    ******* 需要修改json文件的参数名(除密钥外) *******
    change_word     //修改密码
    set_mesh_para   //设置本机组网参数
    set_wifi_para   //设置本机wifi参数
    set_lan_para    //设置本机网口参数
    set_serial_para //设置本机串口参数
    #### 均使用set_single_para_req函数修改最里层参数 ####

    ******* 无需修改直接可发送的json参数(除密钥外) *******
    get_mesh_status     //获取组网参数        ?
    apply_mesh_para     //保存设置的组网参数    
    get_wifi_status     //获取wifi参数       ?
    apply_wifi_para     //保存设置的wifi参数
    get_lan_status      //获取网络参数        ?
    apply_lan_para      //保存设置的网络参数
    get_serial_status   //获取串口参数        ?
    apply_serial_para   //保存设置的串口参数
    get_gps             //获取gps参数
    get_connected_mesh  //获取已连接站点列表
    get_mesh_msg        //获取拓扑信息
    #### 使用get_single_para_resp函数修改最里层参数 ####
*/

#define JSON_REQ_FILE           "/mnt/wt_service/wt_request.json"
#define JSON_RESP_FILE          "/mnt/wt_service/wt_response.json"
#define JSON_FILE_LEN           65535
#define WT_SOCKET_TIMEOUT_COUNT 6   //重复连接次数

#define JRPC_HOST			"192.168.104.1"
#define JRPC_PORT			80
#define JRPC_POST			"POST /ubus %s HTTP/1.1\r\nHost: 192.168.104.1:%d\r\nConnection: keep-alive\r\nContent-Length: %d\r\nContent-Type: application/json\r\n\r\n%s"

#define D(exp,fmt,...) do {                     \
        if(!(exp)){                             \
            fprintf(stderr,fmt,##__VA_ARGS__);  \
            abort();                            \
        }                                       \
    }while(0)

static int new_tcp = 0; //建立tcp连接的返回值
char json_data[JSON_FILE_LEN] = {0};
struct wt_json wt = {{0},{0},{"f00b3e0b47a722f66b14c000wtdevice"}};

const char *wt_mesh[] = 
{
    "login",
    "change_word",
    "get_mesh_status",
    "set_mesh_para",
    "apply_mesh_para",
    "get_wifi_status",
    "set_wifi_para",
    "apply_wifi_para",
    "get_lan_status",
    "set_lan_para",
    "apply_lan_para",
    "get_serial_status",
    "set_serial_para",
    "apply_serial_para",
    "get_sys_status",
    "get_gps",
    "get_connected_mesh",
    "get_mesh_msg"
};

/*
*   brief:  自写幂函数a^b
*   retval: a^b
*   params: a    底数
            b    指数
*/
int32_t my_pow(int32_t a,int32_t b)
{
    int32_t result = 1;
    int i = 0;
    for(i=1;i<b+1;i++)
    {
        result = result * a; 
    }
    return result;
}

/*
*   brief:  由于strcat函数会修改原参数，自写字符串拼接
*   retval: None
*   params: str1    输入字符串1
            str2    输入字符串2
            out     输出字符串
*/
void my_string_cat(char *str1,char *str2,char *out)
{
    memcpy(out,str1,strlen(str1));
    memcpy((out + strlen(str1)),str2,strlen(str2));
}

// static void setnonblock(int fd)
// {
//     fcntl(fd,F_SETFL,fcntl(fd,F_GETFL) | O_NONBLOCK);
// }
// static void setreuseaddr(int fd)
// {
//     int ok = 1;
//     setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&ok,sizeof(ok));
// }

// static void setaddress(const char* ip,int port,struct sockaddr_in* addr)
// {
//     bzero(addr,sizeof(*addr));
//     addr->sin_family = AF_INET;
//     inet_pton(AF_INET,ip,&(addr->sin_addr));
//     addr->sin_port = htons(port);
// }

// static void address_to_string(struct sockaddr_in* addr,char* ip, char* port)
// {
//     inet_ntop(AF_INET,&(addr->sin_addr),ip,sizeof(ip));
//     snprintf(port,(size_t)sizeof(port),"%d",ntohs(addr->sin_port));
// }

// static int new_tcp_server(int port)
// {
//     int fd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
//     D(fd > 0,"socket failed(%m)\n");
//     setnonblock(fd);
//     setreuseaddr(fd);
//     struct sockaddr_in addr;
//     setaddress("0.0.0.0",port,&addr);
//     bind(fd,(struct sockaddr*)&addr,sizeof(addr));
//     listen(fd,64); // backlog = 64
//     return fd;
// }

static int new_tcp_client(const char* ip,int port)
{
    int fd = 0;
    struct sockaddr_in addr;
    fd_set fdr, fdw;
    struct timeval timeout = {5,0};
    int err = 0;
    int errlen = sizeof(err);

    fd = socket(AF_INET,SOCK_STREAM,0);   
    memset(&addr,0,sizeof(addr));
    if (fd < 0) 
    {
        fprintf(stderr, "create socket failed,error:%s.\n", strerror(errno));
        return -1;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);

    /*设置套接字为非阻塞*/
    int flags = fcntl(fd, F_GETFL, 0);  //获取socket文件描述符
    if (flags < 0) 
    {
        fprintf(stderr, "Get flags error:%s\n", strerror(errno));
        close(fd);
        return -1;
    }
    flags |= O_NONBLOCK;    //非阻塞操作
    if (fcntl(fd, F_SETFL, flags) < 0)  //设置文件描述符为非阻塞
    {
        fprintf(stderr, "Set flags error:%s\n", strerror(errno));
        close(fd);
        return -1;
    }

    /*阻塞情况下linux系统默认超时时间为75s*/
    int rc = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (rc != 0) 
    {
        if (errno == EINPROGRESS) 
        {
            // printf("Doing connection.\n");
            /*正在处理连接*/
            FD_ZERO(&fdr);
            FD_ZERO(&fdw);
            FD_SET(fd, &fdr);
            FD_SET(fd, &fdw);
            // timeout.tv_sec = 10;
            // timeout.tv_usec = 0;
            rc = select(fd + 1, &fdr, &fdw, NULL, &timeout);
            // printf("rc is: %d\n", rc);
            /*select调用失败*/
            if (rc < 0) 
            {
                fprintf(stderr,"WT Device connect error:%s\n", strerror(errno));
                close(fd);
                return -1;
            }
            /*连接超时*/
            if (rc == 0) 
            {
                fprintf(stderr,"WT Device Connect timeout\n");
                close(fd);
                return -1;
            }
            /*[1] 当连接成功建立时，描述符变成可写,rc=1*/
            if (rc == 1 && FD_ISSET(fd, &fdw)) 
            {
                // printf("Connect success\n");
                return fd;
            }
            /*[2] 当连接建立遇到错误时，描述符变为即可读，也可写，rc=2 遇到这种情况，可调用getsockopt函数*/
            if (rc == 2) 
            {
                if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1) 
                {
                    fprintf(stderr,"WT Device getsockopt(SO_ERROR): %s", strerror(errno));
                    close(fd);
                    return -1;
                }
                if (err)
                {
                    errno = err;
                    fprintf(stderr,"WT Device connect error:%s\n", strerror(errno));
                    close(fd);
                    return -1;
                }
            }

        }
        fprintf(stderr, "connect failed, error:%s.\n", strerror(errno));
        return -1;
    }
    return 0;
}

/*
*   brief:  向RPC服务端发送单个请求文件
*   retval: None
*   params: data    写入文件内容
*/
void SetJsonRPC_Req(char *data)
{
    char wrtbuf[SIGNLE_JSON_LEN] = {0};
    new_tcp = new_tcp_client(JRPC_HOST,JRPC_PORT);
	sprintf(wrtbuf,JRPC_POST,"",JRPC_PORT,strlen(data),data);
	send(new_tcp,wrtbuf,SIGNLE_JSON_LEN,0);
	// printf("Send--->:\r\n%s\r\n",wrtbuf);
	sleep(1);   //发送后延时等待接收
}

/*
*   brief:  从RPC服务端获取单个响应信息
*   retval: None
*   params: data    从设备获取json数据内容
            time    接收阻塞等待时间
*/
void GetJsonRPC_Resp(char *data,int time)
{
    int recvcnt = 0;
    char source_content[JSON_FILE_LEN] = {0};
    struct timeval timeout = {time,0};

    setsockopt(new_tcp,SOL_SOCKET,SO_SNDTIMEO,(char *)&timeout,sizeof(struct timeval));
    recvcnt = recv(new_tcp,source_content,JSON_FILE_LEN,MSG_WAITALL);
	// printf("Recv recvcnt %d byte<---:\r\n%s\r\n",recvcnt,source_content);
    get_json_content(7,source_content,data); //第7行数据为json数据内容
}

/*
*   brief:  文件上锁操作
*   retval: None
*   params: fd	  文件描述符
            type  上锁类型(读、写)
*/
void lock_set(int fd,int type)
{
	struct flock lock;

	lock.l_type = type;	//设置锁类型
	lock.l_start = 0;	//偏移值为０
	lock.l_whence = SEEK_SET;	//从文件起始位置开始
	lock.l_len = 0;	//整篇加锁
	lock.l_pid = -1;

	// if(lock.l_type != F_UNLCK)
	// {
	// 	if(lock.l_type == F_RDLCK)
	// 	{
    //         printf("rd lock\n");
	// 	}
	// 	else if(lock.l_type == F_WRLCK)
	// 	{
    //         printf("wr lock\n");
	// 	}
	// }
    // else
    // {
    //     printf("unlock\n");
    // }
    
    while(fcntl(fd,F_SETLK,&lock) < 0) //若被其他进程已加锁，则１s后继续
	{
		if((errno == EAGAIN) || (errno == EACCES))	//被其他进程加锁了
		{
            // printf("other process in running\n");
			sleep(1);
		}
	}
	// 此处的F_SETLKW为F_SETLK的阻塞版本，当无法获取锁时进入睡眠等待状态
	// if(fcntl(fd,F_SETLKW,&lock) < 0)
	// {
	// 	exit(1);
	// }
	// switch (lock.l_type)
	// {
	// 	case F_RDLCK:
	// 		printf("read lock set by %d\n",getpid());
	// 		break;
	// 	case F_WRLCK:
	// 		printf("write lock set by %d\n",getpid());
	// 		break;
	// 	case F_UNLCK:
	// 		printf("unlock set by %d\n",getpid());
	// 		break;
	// 	default:
	// 		break;
	// }
}

void wt_read_lock(char *filename,char *data)
{
	FILE *f = NULL;
	int len =0;
	int fd = -1;

	f = fopen(filename,"rb");
	if(f != NULL)
	{
		fd = fileno(f);
		lock_set(fd,F_RDLCK);
	}
	fseek(f,0,SEEK_END);
	len = ftell(f);
	fseek(f,0,SEEK_SET);
	fread(data,1,len,f);
	if(fd != -1)
	{
		lock_set(fd,F_UNLCK);
	}
	fclose(f);
}

void wt_write_lock(char *filename,char *data)
{
    FILE *f = NULL;
	int fd = -1;
	
	f = fopen(filename,"wb");
	if(f != NULL)
	{
		fd = fileno(f);
		lock_set(fd,F_WRLCK);
	}
	fseek(f,0,SEEK_END);
	fseek(f,0,SEEK_SET);

    fwrite(data,1,strlen(data),f);
	if(fd != -1)
	{
		lock_set(fd,F_UNLCK);
	}
	fclose(f);
}

/*
*   brief:  读JSON文件，获取文件内容
*   retval: None
*   params: filename  文件名
            data      文件内容
*/
void wt_read_file(char *filename,char *data)
{
    FILE *f = NULL;
	int len = 0;
	
	f = fopen(filename,"rb");
    if(f == NULL)
	{
		fprintf(stderr,"open read file failed!\n");
		exit(-1);
	}
	fseek(f,0,SEEK_END);
	len = ftell(f);
	fseek(f,0,SEEK_SET);
//    printf("len=%d\n",len);
    fread(data,1,len,f);

    if(f != NULL) 
        fclose(f);
}

/*
*   brief:  写JSON文件，保存修改的文件参数
*   retval: None
*   params: filename  文件名
            data      修改的文件内容
*/
void wt_write_file(char *filename,char *data)
{
    FILE *f = NULL;
	int len = 0;
	
	f = fopen(filename,"wb");
    if(f == NULL)
	{
		fprintf(stderr,"open write file failed!\n");
		exit(-1);
	}
	fseek(f,0,SEEK_END);
	len = ftell(f);
	fseek(f,0,SEEK_SET);

    // printf("read file %s complete, len=%d.\n",filename,len);

    fwrite(data,1,strlen(data),f);
	if(f != NULL) 
        fclose(f);
}

/*
*   brief:  过滤掉RPC服务器发回的字符串前缀后缀，仅保留当前行
*   retval: None
*   params: line    从固定行开始
            source  原始数据
            out     仅包含json文件的数据内容
*/
void get_json_content(int line, char *source, char *out)
{
	char temp_string[SIGNLE_JSON_LEN];
    int temp_line = 0;
	int i = 0,j = 0;

	for(i=0;i<(int)strlen(source);i++)
	{
        temp_string[j++] = source[i];
		if(i > 0)
		{
			if((source[i] == '\n') && (source[i-1] == '\r'))
			{
				temp_line++;
				if(temp_line == line)
				{
					memcpy(out,temp_string,j);
					break;
				}
				else
				{
					memset(temp_string,0,SIGNLE_JSON_LEN);
					j = 0;
				}
			}
		}
	}
}

/*
*   brief:  设置单个请求json参数 request
            set_mesh_para
            set_wifi_para
            set_lan_para
            set_serial_para
*   retval: 0 success
            -1 failed
*   params: obj_out 单个json命令名称
            obj_in  单个网络参数名
            value   修改参数，字符型
*/
int set_single_para_req(char *obj_out,char *obj_in,char *value)
{
    cJSON *para = NULL;
    cJSON *para_out = NULL,*para_mid = NULL,*para_inner = NULL,*para_array = NULL;
    char *print_out = NULL;

    memset(json_data,0,JSON_FILE_LEN);
    wt_read_lock(JSON_REQ_FILE,json_data);
    para = cJSON_Parse(json_data);

    // print_out = cJSON_Print(para);
    // printf("before modify:%s\n",print_out);
    // free(print_out);

    if (!para)
	{
		printf("Error Single Req: [%s]\n",cJSON_GetErrorPtr());
		return -1;
	}
	else
	{
        para_out = cJSON_GetObjectItem(para,obj_out);
        para_mid = cJSON_GetObjectItem(para_out,"params");
        para_array = cJSON_GetArrayItem(para_mid,3);
        para_inner = cJSON_GetObjectItem(cJSON_GetObjectItem(para_array,"configs"),obj_in);
        cJSON_SetValuestring(para_inner,value);

        print_out = cJSON_Print(para);
        // printf("after modify:%s\n",print_out);
        if(print_out != NULL)
        {
            wt_write_lock(JSON_REQ_FILE,print_out);
            free(print_out);          
        }
    }
    cJSON_Delete(para);
    memset(json_data,0,JSON_FILE_LEN);
    return 0;
}

/*
*   brief:  获取单个响应json参数 response
            该函数可用于：
            get_mesh_status_resp    ：获取mesh节点状态
            get_wifi_status_resp    ：获取wifi节点状态
            get_lan_status_resp     ：获取lan网络状态
            get_sys_status_resp     ：获取系统状态信息
            get_gps_resp            ：获取gps信息
*   retval: 0 success
            -1 failed
*   params: obj_out 单个json命令名称
            obj_in  单个网络参数名
            value   读取数值
*/
int get_single_para_resp(char *obj_out,char *obj_in,char *value)
{
    cJSON *para = NULL;
    cJSON *para_out = NULL,*para_mid = NULL,*para_inner = NULL,*para_array = NULL;
    char *print_out = NULL;

    memset(json_data,0,JSON_FILE_LEN);
    wt_read_lock(JSON_RESP_FILE,json_data);
    para = cJSON_Parse(json_data);

    // print_out = cJSON_Print(para);
    // printf("before modify:%s\n",print_out);
    // free(print_out);

    if (!para)
	{
		printf("Error Single Resp: [%s]\n",cJSON_GetErrorPtr());
		return -1;
	}
	else
	{
        para_out = cJSON_GetObjectItem(para,obj_out);
        para_mid = cJSON_GetObjectItem(para_out,"result");
        para_array = cJSON_GetArrayItem(para_mid,1);
        para_inner = cJSON_GetObjectItem(para_array,obj_in);
        // cJSON_SetValuestring(para_inner,value);

        print_out = cJSON_Print(para_inner);
        memcpy(value,print_out+1,strlen(print_out)-2);
        // printf("after modify:%s\n",print_out);
        if(print_out != NULL)
        {
            // wt_write_lock(JSON_RESP_FILE,print_out);
            free(print_out);          
        }
    }
    cJSON_Delete(para);
    memset(json_data,0,JSON_FILE_LEN);
    return 0;
}

/*
*   brief:  登录设备，返回密钥信息，并将密钥信息写入response文件
*   retval: 0 success
            -1 failed
*   params: None
*/
int WT_Login(void)
{
    cJSON *para_req = NULL,*para_resp = NULL,*para_login_resp = NULL;
    cJSON *para_login_req = NULL;
    cJSON *para_out = NULL,*para_inner = NULL,*para_array = NULL;
    char *print_out = NULL;
    char resbuf[SIGNLE_JSON_LEN] = {0};

    memset(json_data,0,JSON_FILE_LEN);
    /************* 请求 **************/
    wt_read_lock(JSON_REQ_FILE,json_data);
    para_req = cJSON_Parse(json_data);
    if (!para_req)
	{
		printf("Error Login Req: [%s]\n",cJSON_GetErrorPtr());
		return -1;
	}
	else
	{
        para_login_req = cJSON_GetObjectItem(para_req,"login");//从json文件中提取出login命令
        print_out = cJSON_Print(para_login_req);    //将提取命令转化为字符型
        SetJsonRPC_Req(print_out);     //向RPC服务端发送login 
        if(print_out != NULL)
        {
            // wt_write_lock(JSON_RESP_FILE,print_out); 
            free(print_out);          
        }
        GetJsonRPC_Resp(resbuf,0);   //获取响应参数
    }
    cJSON_Delete(para_req);
    memset(json_data,0,JSON_FILE_LEN);
    if(strlen(resbuf) != 123)   //login返回的json参数固定为123个字节
    {
        return -1;
    }
    /************* 响应 **************/
    para_login_resp = cJSON_Parse(resbuf);
    if(!para_login_resp)
    {
		printf("Error Login Resp: [%s]\n",cJSON_GetErrorPtr());
		return -1;
    }
    else
    {
        para_out = cJSON_GetObjectItem(para_login_resp,"result");
        para_array = cJSON_GetArrayItem(para_out,1);
        para_inner = cJSON_GetObjectItem(para_array,"ubus_rpc_session");//获取密钥
        print_out = cJSON_Print(para_inner);  
        memcpy(wt.secret_key,print_out+1,strlen(print_out)-2); //更新密钥，去除双引号
        free(print_out);
    }
    cJSON_Delete(para_login_resp);
    /************* 更新response文件 **************/
    wt_read_lock(JSON_RESP_FILE,json_data);
    para_resp = cJSON_Parse(json_data);
    if (!para_resp)
	{
		printf("Error Resp: [%s]\n",cJSON_GetErrorPtr());
		return -1;
	}
	else
	{
        para_out = cJSON_GetObjectItem(cJSON_GetObjectItem(para_resp,"login_resp"),"result");
        para_array = cJSON_GetArrayItem(para_out,1);
        para_inner = cJSON_GetObjectItem(para_array,"ubus_rpc_session");//获取密钥
        cJSON_SetValuestring(para_inner,wt.secret_key);
        print_out = cJSON_Print(para_resp);
        printf("secret_key:%s\n",wt.secret_key);
        if(print_out != NULL)
        {
            wt_write_lock(JSON_RESP_FILE,print_out); 
            free(print_out);          
        }
    }
    cJSON_Delete(para_resp);
    memset(json_data,0,JSON_FILE_LEN);
    return 0;
}

/*
*   brief:  获取本地wt_request.json中的三个设置参数
*   retval: 0 success
            -1 failed
*   params: buf[0] channel
            buf[1] bw
            buf[2] power
            buf[3] channel add
*/
int WT_GetLocalSetParams(uint8_t *buf)
{
    cJSON *para = NULL;
    cJSON *para_out = NULL,*para_inner = NULL,*para_array = NULL;
    cJSON *set_para[3] = {NULL};
    bool word_contain = false;
    char str_value[128] = {0},str_word[128] = {0};
    char *print_out = NULL;
    int i = 0;

    memset(json_data,0,JSON_FILE_LEN);
    wt_read_lock(JSON_REQ_FILE,json_data);
    para = cJSON_Parse(json_data);

    if (!para)
	{
		printf("Error Single Req: [%s]\n",cJSON_GetErrorPtr());
		return -1;
	}
	else
	{
        para_out = cJSON_GetObjectItem(para,"set_mesh_para");
        para_inner = cJSON_GetObjectItem(para_out,"params");
        para_array = cJSON_GetObjectItem(cJSON_GetArrayItem(para_inner,3),"configs");
        set_para[0] = cJSON_GetObjectItem(para_array,"channel");
        set_para[1] = cJSON_GetObjectItem(para_array,"chanbw");
        set_para[2] = cJSON_GetObjectItem(para_array,"txpower");

        print_out = cJSON_Print(set_para[0]);
        memcpy(str_value,print_out+1,strlen(print_out)-2);
        free(print_out);
        for(i=0;i<(int)strlen(str_value);i++)   //channel/channel_add
        {
            if(((str_value[i] <= 0x44) && (str_value[i] >= 0x41)) ||\
            ((str_value[i] <= 0x64) && (str_value[i] >= 0x61))) //在大写ABCD和小写abcd的区间内
            {
                word_contain = true;
            }
            if((str_value[i] <= 0x64) && (str_value[i] >= 0x61))
            {
                str_value[i] = str_value[i] - 0x20; //小写转换为大写
            }
        }
        if(word_contain)
        {
            memcpy(str_word,str_value,strlen(str_value)-1);
            buf[3] = str_value[(int)strlen(str_value)-1] - 0x37;
            if(strlen(str_word) == 1)
            {
                buf[0] = str_word[0] - 0x30;
            }
            else if(strlen(str_word) == 2)
            {
                buf[0] = (str_word[1] - 0x30) + (str_word[0] - 0x30) * 10;
            }
            else if(strlen(str_word) == 3)
            {
                buf[0] = (str_word[2] - 0x30) + (str_word[1] - 0x30) * 10 + (str_word[0] - 0x30) * 100;
            }
            else
            {
                buf[0] = 0;
            }
            memset(str_word,0,128);
            memset(str_value,0,128);
        }
        else
        {
            if(strlen(str_value) == 1)
            {
                buf[0] = str_value[0] - 0x30;
            }
            else if(strlen(str_value) == 2)
            {
                buf[0] = (str_value[1] - 0x30) + (str_value[0] - 0x30) * 10;
            }
            else if(strlen(str_value) == 3)
            {
                buf[0] = (str_value[2] - 0x30) + (str_value[1] - 0x30) * 10 + (str_value[0] - 0x30) * 100;
            }
            else
            {
                buf[0] = 0;
            }
            memset(str_value,0,128);
            buf[3] = 0;
        }
        
        for(i=1;i<3;i++)    //bw / power
        {
            print_out = cJSON_Print(set_para[i]);
            memcpy(str_value,print_out+1,strlen(print_out)-2);
            free(print_out);
            if(strlen(str_value) == 1)
            {
                buf[i] = str_value[0] - 0x30;
            }
            else if(strlen(str_value) == 2)
            {
                buf[i] = (str_value[1] - 0x30) + (str_value[0] - 0x30) * 10;
            }
            else if(strlen(str_value) == 3)
            {
                buf[i] = (str_value[2] - 0x30) + (str_value[1] - 0x30) * 10 + (str_value[0] - 0x30) * 100;
            }
            else
            {
                buf[i] = 0;
            }
            memset(str_value,0,128);
        }
        print_out = cJSON_Print(para);
        if(print_out != NULL)
        {
            free(print_out);          
        }
    }
    cJSON_Delete(para);
    memset(json_data,0,JSON_FILE_LEN);
    return 0;
}

/*
*   brief:  向RPC服务端发送获取组网信息命令并在返回的json文件参数中提取出节点ID，作为组网成功的判断依据
*   retval: 0 success
            -1 failed
*   params: device_id    ID号
*/
int WT_GetMeshMsg(uint8_t *device_id)
{
    cJSON *para = NULL;
    cJSON *para_out = NULL,*para_mid = NULL,*para_inner = NULL,*para_array = NULL;
    cJSON *para_id[sizeof(device_id)] = {NULL};
    char *print_out = NULL;
    char str_id[128] = {0};
    int i = 0,array_size = 0;

    i = WT_GroupProcess(GET_MESH_MSG);
    if(i == -1)
    {
        return -1;
    }
    else
    {
        memset(json_data,0,JSON_FILE_LEN);
        wt_read_lock(JSON_RESP_FILE,json_data);
        para = cJSON_Parse(json_data);
        if (!para)
        {
            printf("Error WT Msg: [%s]\n",cJSON_GetErrorPtr());
            return -1;
        }
        else
        {
            para_out = cJSON_GetObjectItem(para,"get_mesh_msg_resp");
            para_mid = cJSON_GetObjectItem(para_out,"result");
            para_inner = cJSON_GetArrayItem(para_mid,1);
            para_array = cJSON_GetObjectItem(para_inner,"vis");
            array_size = cJSON_GetArraySize(para_array);
            // printf("array_size=%d\n",array_size);
            if(array_size == 0)
            {
                memset(device_id,0,(size_t)sizeof(device_id));
            }
            else
            {
                for(i=0;i<array_size;i++)
                {
                    para_id[i] = cJSON_GetObjectItem(cJSON_GetArrayItem(para_array,i),"nodeid");
                    print_out = cJSON_Print(para_id[i]);
                    memcpy(str_id,print_out+1,strlen(print_out)-2);
                    // printf("array_content_%d:%s\n",i,str_id);
                    free(print_out);
                    if(strlen(str_id) == 1)
                    {
                        device_id[i] = str_id[0] - 0x30;
                        // printf("device_id_%d=%d\n",i,device_id[i]);
                        memset(str_id,0,128);
                    }
                    else if(strlen(str_id) == 2)
                    {
                        device_id[i] = (str_id[1] - 0x30) + (str_id[0] - 0x30) * 10;
                        // printf("device_id_%d=%d\n",i,device_id[i]);
                        memset(str_id,0,128);
                    }
                    else if(strlen(str_id) == 3)
                    {
                        device_id[i] = (str_id[2] - 0x30) + (str_id[1] - 0x30) * 10 + (str_id[0] - 0x30) * 100;
                        // printf("device_id_%d=%d\n",i,device_id[i]);
                        memset(str_id,0,128);
                    }
                    else
                    {
                        device_id[i] = 0;
                        memset(str_id,0,128);
                    }
                }
            }
            print_out = cJSON_Print(para);
            if(print_out != NULL)
            {
                // wt_write_lock(JSON_RESP_FILE,print_out);
                free(print_out);          
            }
        }
        cJSON_Delete(para);
        memset(json_data,0,JSON_FILE_LEN);
        return 0;
    }
}

/*
*   brief:  向RPC服务端发送除登录请求命令外的单组json参数(此动作一定发生在login获取密钥之后)
*   retval: 0 success
            -1 failed
*   params: name    单组json参数名称
*/
int WT_SendGroupReq(const char *name)
{
    cJSON *para_req = NULL,*para_name = NULL;
    cJSON *para_out = NULL,*para_inner = NULL;
    char *print_out = NULL;
    // static int i = 0;

    memset(json_data,0,JSON_FILE_LEN);
    wt_read_lock(JSON_REQ_FILE,json_data);
    para_req = cJSON_Parse(json_data);
    if (!para_req)
	{
		printf("Error Group Req: [%s]\n",cJSON_GetErrorPtr());
		return -1;
	}
	else
	{
        para_name = cJSON_GetObjectItem(para_req,name); //获取参数名
        //更改更新密钥
        para_out = cJSON_GetObjectItem(para_name,"params");
        para_inner = cJSON_GetArrayItem(para_out,0);       
        cJSON_SetValuestring(para_inner,wt.secret_key);
        print_out = cJSON_Print(para_name);
        // printf("port:%d\n%s:%s\n",++i,name,print_out);
        SetJsonRPC_Req(print_out);     //向RPC服务端发送请求命令
        free(print_out);
    }
    cJSON_Delete(para_req);
    memset(json_data,0,JSON_FILE_LEN);
    return 0;
}

/*
*   brief:  保存RPC服务端返回单组json数据到response文件
*   retval: 0 success
            -1 failed
*   params: name    单组json参数名称，与请求参数名一一对应
            time    阻塞等待时间
*/
int WT_SaveGroupResp(const char *name,int time)
{
    cJSON *para_resp = NULL;
    // cJSON *para_name = NULL;
    cJSON *para_create = NULL;
    char *print_out = NULL;
    char modified_name[SIGNLE_JSON_LEN] = {0};
    char resbuf[SIGNLE_JSON_LEN] = {0};

    //响应对象的名称使用固定后缀名_resp，字符串拼接实现
    my_string_cat(name,"_resp",modified_name);  
    GetJsonRPC_Resp(resbuf,time);//获取端口返回数据
    // printf("resbuf:%s\n",resbuf);
    if(strlen(resbuf) == 0)
    {
        return -1;
    }
    memset(json_data,0,JSON_FILE_LEN);
    wt_read_lock(JSON_RESP_FILE,json_data);
    para_resp = cJSON_Parse(json_data);
    if (!para_resp)
	{
		printf("Error Group Save: [%s]\n",cJSON_GetErrorPtr());
		return -1;
	}
	else
	{
        // para_name = cJSON_GetObjectItem(para_resp,modified_name); //获取参数名    
        cJSON_DeleteItemFromObject(para_resp,modified_name); //删掉已有参数
        para_create = cJSON_Parse(resbuf);  //将返回单组json参数转换为cJSON型
        cJSON_AddItemToObject(para_resp,modified_name,para_create);//向modified_name对象中添加item，与被删除对象同名
        
        print_out = cJSON_Print(para_resp);
        // printf("after:%s\n",print_out);
        if(print_out != NULL)
        {
            wt_write_lock(JSON_RESP_FILE,print_out); 
            free(print_out);          
        }
    }
    cJSON_Delete(para_resp);
    memset(json_data,0,JSON_FILE_LEN);
    return 0;
}

/*
*   brief:  单组json参数发送返回处理，最多10次重复请求
*   retval: 0 success
            -1 failed
*   params: name_num  参数名称代号
*/
int WT_GroupProcess(int name_num)
{
    int i = -1,j = -1;
    int count = 0;
    switch (name_num)
    {
        case LOGIN: //登录
            while(i == -1)
            {
                i = WT_Login();
                count++;
                if(count > WT_SOCKET_TIMEOUT_COUNT)
                {
                    printf("login_failed!\n");
                    return -1;
                }
            }
            printf("login_success\n");
            break;
        case CHANGE_WORD:   //修改密码
            while((i == -1) || (j == -1))
            {
                i = WT_SendGroupReq(wt_mesh[CHANGE_WORD]);
                j = WT_SaveGroupResp(wt_mesh[CHANGE_WORD],0); 
                count++;
                if(count > WT_SOCKET_TIMEOUT_COUNT)
                {
                    printf("change_word_failed\n");
                    return -1;
                }
            }
            printf("change_word_success\n");
            break;
        case GET_MESH_STATUS:
            while((i == -1) || (j == -1))
            {
                i = WT_SendGroupReq(wt_mesh[GET_MESH_STATUS]);
                j = WT_SaveGroupResp(wt_mesh[GET_MESH_STATUS],0);
                count++;
                if(count > WT_SOCKET_TIMEOUT_COUNT)
                {
                    printf("get_mesh_status_failed!\n");
                    return -1;
                }
            }
            printf("get_mesh_status_success\n");
            break;
        case SET_MESH_PARA:
            while((i == -1) || (j == -1))
            {
                i = WT_SendGroupReq(wt_mesh[SET_MESH_PARA]);
                j = WT_SaveGroupResp(wt_mesh[SET_MESH_PARA],0);
                count++;
                if(count > 9)
                {
                    printf("set_mesh_para_failed!\n");
                    return -1;
                }
            }
            printf("set_mesh_para_success\n");
            break;
        case APPLY_MESH_PARA:
            while((i == -1) || (j == -1))
            {
                i = WT_SendGroupReq(wt_mesh[APPLY_MESH_PARA]);
                j = WT_SaveGroupResp(wt_mesh[APPLY_MESH_PARA],0);
                count++;
                if(count > WT_SOCKET_TIMEOUT_COUNT)
                {
                    printf("apply_mesh_para_failed!\n");
                    return -1;
                }
            }
            printf("apply_mesh_para_success\n");
            break;
        case GET_WIFI_STATUS:
            while((i == -1) || (j == -1))
            {
                i = WT_SendGroupReq(wt_mesh[GET_WIFI_STATUS]);
                j = WT_SaveGroupResp(wt_mesh[GET_WIFI_STATUS],0);
                count++;
                if(count > WT_SOCKET_TIMEOUT_COUNT)
                {
                    printf("get_wifi_status_failed!\n");
                    return -1;
                }
            }
            printf("get_wifi_status_success\n");
            break;
        case SET_WIFI_PARA:
            while((i == -1) || (j == -1))
            {
                i = WT_SendGroupReq(wt_mesh[SET_WIFI_PARA]);
                j = WT_SaveGroupResp(wt_mesh[SET_WIFI_PARA],0);
                count++;
                if(count > WT_SOCKET_TIMEOUT_COUNT)
                {
                    printf("set_wifi_para_failed!\n");
                    return -1;
                }
            }
            printf("set_wifi_para_success\n");
            break;
        case APPLY_WIFI_PARA:
            while((i == -1) || (j == -1))
            {
                i = WT_SendGroupReq(wt_mesh[APPLY_WIFI_PARA]);
                j = WT_SaveGroupResp(wt_mesh[APPLY_WIFI_PARA],0);
                count++;
                if(count > WT_SOCKET_TIMEOUT_COUNT)
                {
                    printf("apply_wifi_para_failed!\n");
                    return -1;
                }
            }
            printf("apply_wifi_para_success\n");
            break;
        case GET_LAN_STATUS:
            while((i == -1) || (j == -1))
            {
                i = WT_SendGroupReq(wt_mesh[GET_LAN_STATUS]);
                j = WT_SaveGroupResp(wt_mesh[GET_LAN_STATUS],0);
                count++;
                if(count > WT_SOCKET_TIMEOUT_COUNT)
                {
                    printf("get_lan_status_failed!\n");
                    return -1;
                }
            }
            printf("get_lan_status_success\n");
            break;
        case SET_LAN_PARA:
            while((i == -1) || (j == -1))
            {
                i = WT_SendGroupReq(wt_mesh[SET_LAN_PARA]);
                j = WT_SaveGroupResp(wt_mesh[SET_LAN_PARA],0);
                count++;
                if(count > WT_SOCKET_TIMEOUT_COUNT)
                {
                    printf("set_lan_para_failed!\n");
                    return -1;
                }
            }
            printf("set_lan_para_success\n");
            break;
        case APPLY_LAN_PARA:
            while((i == -1) || (j == -1))
            {
                i = WT_SendGroupReq(wt_mesh[APPLY_LAN_PARA]);
                j = WT_SaveGroupResp(wt_mesh[APPLY_LAN_PARA],0);
                count++;
                if(count > WT_SOCKET_TIMEOUT_COUNT)
                {
                    printf("apply_lan_para_failed!\n");
                    return -1;
                }
            }
            printf("apply_lan_para_success\n");
            break;
        case GET_SERIAL_STATUS:
            while((i == -1) || (j == -1))
            {
                i = WT_SendGroupReq(wt_mesh[GET_SERIAL_STATUS]);
                j = WT_SaveGroupResp(wt_mesh[GET_SERIAL_STATUS],0);
                count++;
                if(count > WT_SOCKET_TIMEOUT_COUNT)
                {
                    printf("get_serial_status_failed!\n");
                    return -1;
                }
            }
            printf("get_serial_status_success\n");
            break;
        case SET_SERIAL_PARA:
            while((i == -1) || (j == -1))
            {
                i = WT_SendGroupReq(wt_mesh[SET_SERIAL_PARA]);
                j = WT_SaveGroupResp(wt_mesh[SET_SERIAL_PARA],0);
                count++;
                if(count > WT_SOCKET_TIMEOUT_COUNT)
                {
                    printf("set_serial_para_failed!\n");
                    return -1;
                }
            }
            printf("set_serial_para_success\n");
            break;
        case APPLY_SERIAL_PARA:
            while((i == -1) || (j == -1))
            {
                i = WT_SendGroupReq(wt_mesh[APPLY_SERIAL_PARA]);
                j = WT_SaveGroupResp(wt_mesh[APPLY_SERIAL_PARA],0);
                count++;
                if(count > WT_SOCKET_TIMEOUT_COUNT)
                {
                    printf("apply_serial_para_failed!\n");
                    return -1;
                }
            }
            printf("apply_serial_para_success\n");
            break;
        case GET_SYS_STATUS:
            while((i == -1) || (j == -1))
            {
                i = WT_SendGroupReq(wt_mesh[GET_SYS_STATUS]);
                j = WT_SaveGroupResp(wt_mesh[GET_SYS_STATUS],0);
                count++;
                if(count > WT_SOCKET_TIMEOUT_COUNT)
                {
                    printf("get_sys_status_failed!\n");
                    return -1;
                }
            }
            printf("get_sys_status_success\n");
            break;
        case GET_GPS:
            while((i == -1) || (j == -1))
            {
                i = WT_SendGroupReq(wt_mesh[GET_GPS]);
                j = WT_SaveGroupResp(wt_mesh[GET_GPS],0);
                count++;
                if(count > WT_SOCKET_TIMEOUT_COUNT)
                {
                    printf("get_gps_failed!\n");
                    return -1;
                }
            }
            printf("get_gps_success\n");
            break;
        case GET_CONNECTED_MESH:
            while((i == -1) || (j == -1))
            {
                i = WT_SendGroupReq(wt_mesh[GET_CONNECTED_MESH]);
                j = WT_SaveGroupResp(wt_mesh[GET_CONNECTED_MESH],0);
                count++;
                if(count > WT_SOCKET_TIMEOUT_COUNT)
                {
                    printf("get_connected_mesh_failed!\n");
                    return -1;
                }
            }
            printf("get_connected_mesh_success\n");
            break;
        case GET_MESH_MSG:
            while((i == -1) || (j == -1))
            {
                i = WT_SendGroupReq(wt_mesh[GET_MESH_MSG]);
                j = WT_SaveGroupResp(wt_mesh[GET_MESH_MSG],0);
                count++;
                if(count > WT_SOCKET_TIMEOUT_COUNT)
                {
                    printf("get_mesh_message_failed!\n");
                    return -1;
                }
            }
            printf("get_mesh_message_success\n");
            break;
        default:
            printf("wrong_input_params\n");
            break;
    }
    return 0;
}

/*
*   brief:  获取response.json中的三个主要的mesh参数，信道 带宽 功率，并将其转换为int型
*   retval: 0 success
            -1 failed
*   params: params[0]  channel  信道号 取值范围1-11(0x1/0x1A/0x1B/0x1C/0x1D)
            params[1]  chanbw   频宽Mhz  2/5/10/20
            params[2]  txpower  功率dBm  3-33
            params[3]  channel_add  信道号后缀
*/
int Lora_GetWTParams(uint8_t *params)
{
    int wt_temp_para[4] = {0};
    char ch[20] = {0},bw[20] = {0},pw[20] = {0};
    uint8_t ch_dat[20] = {0},bw_dat[20] = {0},pw_dat[20] = {0};
    bool word_contain = false;
    int i = 0;

    if(get_single_para_resp("get_mesh_status_resp","channel",ch) == -1)
    {
        return -1;
    }
    if(get_single_para_resp("get_mesh_status_resp","chanbw",bw) == -1)
    {
        return -1;
    }
    if(get_single_para_resp("get_mesh_status_resp","txpower",pw) == -1)
    {
        return -1;
    }
    // printf("\nget_wt_channel:%s\nget_wt_bw:%s\r\nget_wt_power:%s\n\n",ch,bw,pw);

    for(i=0;i<(int)strlen(ch);i++)
    {
        if((ch[i] < 0x45) && (ch[i] > 0x40))    //是否包含字符型字母A/B/C/D
        {
            word_contain = true;
        }
    }
    if(word_contain)    //包含字符型字母
    {
        for(i=0;i<(int)strlen(ch);i++)
        {
            if((ch[i] < 0x3a) && (ch[i] > 0x2f))    //字符型数字
            {
                ch_dat[i] = ch[i] - 0x30;
                wt_temp_para[0] += ch_dat[i] * my_pow(10,(int)strlen(ch) - i - 2);
            }
            else if((ch[i] < 0x45) && (ch[i] > 0x40))    //字符型字母A/B/C/D
            {
                ch_dat[i] = ch[i] - 0x37;
                wt_temp_para[3] = ch_dat[i];
            }
            else
            {
                printf("get ch params error\n");
            }
        }
    }
    else    //不含字母后缀
    {
        for(i=0;i<(int)strlen(ch);i++)
        {
            if((ch[i] < 0x3a) && (ch[i] > 0x2f))    //字符型数字
            {
                ch_dat[i] = ch[i] - 0x30;
                wt_temp_para[0] += ch_dat[i] * my_pow(10,(int)strlen(ch) - i - 1);
            }
        }
        wt_temp_para[3] = 0;
    }
    
    for(i=0;i<(int)strlen(bw);i++)
    {
        bw_dat[i] = bw[i] - 0x30;
        wt_temp_para[1] += bw_dat[i] * my_pow(10,(int)strlen(bw) - i - 1);
    }
    for(i=0;i<(int)strlen(pw);i++)
    {
        pw_dat[i] = pw[i] - 0x30;
        wt_temp_para[2] += pw_dat[i] * my_pow(10,(int)strlen(bw) - i - 1);
    }

    if((wt_temp_para[0] > 255) || (wt_temp_para[3] > 16))
    {
        printf("get channel num out of range\n");
        return -1;
    }
    else
    {
        params[0] = wt_temp_para[0];
        params[3] = wt_temp_para[3];
    }
    if(wt_temp_para[1] > 255)
    {
        printf("get chanbw value out of range\n");
        return -1;
    }
    else
    {
        params[1] = wt_temp_para[1];
    }
    if(wt_temp_para[2] > 255)
    {
        printf("get txpower value out of range\n");
        return -1;
    }
    else
    {
        params[2] = wt_temp_para[2];
    }

    return 0;
}

/*
*   brief:  接收其他节点发送的业务数据，其中包含更改本机宽带网络参数的命令
*   retval: 0 success
            -1 failed
*   params: buf lora业务帧中包含的三个修改参数
            0   channel     信道号 取值范围1-11，需转换为字符型(0x1/0x1A/0x1B/0x1C/0x1D)
            1   chanbw      频宽Mhz  "2"/"5"/"10"/"20"
            2   txpower     功率dBm  3-33，需转换为字符型
            3   channel_add 信道号后缀
*/
int Lora_SetWTParams(uint8_t *buf)
{
    int i = 0,j = 0;
    uint8_t char_to_num[2] = {0};
    char channel_add_word[128] = {0};
    uint8_t word = 0;

    memset(char_to_num,0,2);
    if(buf[0] != 0)   //设置信道号并保存到request
    {
        if(buf[3] == 0) //不含字母后缀
        {
            if((buf[0] < 10) && (buf[0] >= 1))
            {
                char_to_num[1] = buf[0] + 0x30;
                printf("set channel number:%s\n",(char *)&char_to_num[1]);
                set_single_para_req("set_mesh_para","channel",(char *)&char_to_num[1]);
            }
            else if((buf[0] >= 10) && (buf[0] <= 11))
            {
                char_to_num[1] = buf[0] % 10 + 0x30;
                char_to_num[0] = buf[0] / 10 + 0x30;
                printf("set channel number:%s\n",(char *)&char_to_num[0]);
                set_single_para_req("set_mesh_para","channel",(char *)&char_to_num[0]);
            }
            else
            {
                printf("input channel number out of range!\n");
            }     
        }
        else    //含字母后缀
        {
            if((buf[3] <= 0x0d) && (buf[3] >= 0x0a))
            {
                if((buf[0] < 10) && (buf[0] >= 1))
                {
                    char_to_num[1] = buf[0] + 0x30;
                    word = buf[3] + 0x37;
                    memcpy(channel_add_word,(char *)char_to_num+1,1);
                    memcpy(channel_add_word+1,(char *)&word,1);
                    printf("set channel number:%s\n",channel_add_word);
                    set_single_para_req("set_mesh_para","channel",channel_add_word);
                }
                else if((buf[0] >= 10) && (buf[0] <= 11))
                {
                    char_to_num[1] = buf[0] % 10 + 0x30;
                    char_to_num[0] = buf[0] / 10 + 0x30;
                    word = buf[3] + 0x37;
                    memcpy(channel_add_word,(char *)char_to_num,2);
                    memcpy(channel_add_word+2,(char *)&word,1);
                    printf("set channel number:%s\n",channel_add_word);
                    set_single_para_req("set_mesh_para","channel",channel_add_word);
                }
                else
                {
                    printf("input channel number out of range!\n");
                } 
            }
        }
    }
    if(buf[1] != 0)  //设置带宽并保存到request
    {
        switch (buf[1])
        {
            case 2:
                printf("set channel bw:2MHz\n");
                set_single_para_req("set_mesh_para","chanbw","2");
                break;
            case 5:
                printf("set channel bw:5MHz\n");
                set_single_para_req("set_mesh_para","chanbw","5");
                break;
            case 10:
                printf("set channel bw:10MHz\n");
                set_single_para_req("set_mesh_para","chanbw","10");
                break;
            case 20:
                printf("set channel bw:20MHz\n");
                set_single_para_req("set_mesh_para","chanbw","20");
                break;
            default:
                printf("channel bw set error\n");
                break;
        }
    }
    memset(char_to_num,0,2);
    if(buf[2] != 0)  //设置信道号并保存到request
    {
        if((buf[2] < 10) && (buf[2] >= 3))
        {
            char_to_num[1] = buf[2] + 0x30;
            printf("set txpower:%sdBm\n",(char *)&char_to_num[1]);
            set_single_para_req("set_mesh_para","txpower",(char *)&char_to_num[1]);
        }
        else if((buf[2] >= 10) && (buf[2] <= 33))
        {
            char_to_num[1] = buf[2] % 10 + 0x30;
            char_to_num[0] = buf[2] / 10 + 0x30;
            printf("set txpower:%sdBm\n",(char *)&char_to_num[0]);
            set_single_para_req("set_mesh_para","txpower",(char *)&char_to_num[0]);
        }
        else
        {
            printf("input txpower out of range!\n");
        }     
    }
    i = WT_GroupProcess(SET_MESH_PARA);
    j = WT_GroupProcess(APPLY_MESH_PARA);
    if((i == -1) || (j == -1))
    {
        printf("set mesh parameters failed\n");
        return -1;
    }
    return 0;
}


