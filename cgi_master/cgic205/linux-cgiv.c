#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "cJSON.h"

#define JSON_REQ_FILE           "/mnt/wt_service/wt_request.json"
#define JSON_FILE_LEN           65535

char json_data[JSON_FILE_LEN] = {0};
static uint8_t wt_para[3] = {0};	//高速组网的三个参数[0] 信道channel   10~14:1/1A/1B/1C/1D
									//								110~114:11/11A/11B/11C/11D
									//				 [1] 带宽bw	  2/5/10/20
									//				 [2] 功率power	3-33

/*
*   brief:  web网页高速组网参数获取
*   retval: 0 success
*   params: None
*/
int web_cgi_input(void)
{
	char *data = NULL;
	long wan = 0;
	long nodNum = 0,hopNum = 0,retNum = 0;

	printf("Content-Type:text/html;charset=gb2312 \n\n");
	printf("<html>\n"); 

	printf("<script type=\"text/javascript\">");
	printf("var wzdWanTypeInf=new Array(0,0,0,0 );");
	printf("</script>");

	printf("<META http-equiv=\"Pragma\" content=\"no-cache\">");
	printf("<META http-equiv=\"Expires\" content=\"wed, 26 Feb 1997 08:21:57 GMT\">");
	printf("<link href=\"../dynaform/css_main.css\" rel=\"stylesheet\" />");
	printf("<script src=\"../dynaform/common.js\" type=\"text/javascript\"></script>");

	printf("<SCRIPT language=\"javascript\" src=\"../dynaform/custom.js\" type=\"text/javascript\"></SCRIPT>");

	printf("<body><center><br>\n");
	printf("<form action=\"../userRpm/StatusRpm.htm\" enctype=\"multipart/form-data\" method=\"get\" >");

	printf("<table width=\"502\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">");
	printf("<tr>");
	printf("<td width=\"7\" class=\"title\"><img src=\"../userRpm/images/arc.gif\" width=\"7\" height=\"24\"></td>");
	printf("<td width=\"495\" align=\"left\" valign=\"middle\" class=\"title\">设置结果-AdHoc组网优化</td>");
	printf("</tr>");

	data = getenv("QUERY_STRING");
	if(data == NULL)
	{
		printf("<tr>错误: 数据未输入或传输有问题</tr>");
	}
	else
	{
		if(sscanf(data,"wan=%ld&nodNum=%ld&hopNum=%ld&retNum=%ld",&wan,&nodNum,&hopNum,&retNum) != 4)
		{
			printf("<table width=\"502\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">");
			printf("<tr><td valign=\"middle\"><span style=\"vertical-align: middle\"><font size=\"2\">错误: 非法数据，必须是数字</td></tr>");
			printf("<tr>");
			printf("<td height=\"30\" class=\"tail\" align=\"right\">&nbsp;");  
			printf("<input type=\"submit\" class=\"button\"  value=\"退出\">"); 
			printf("</td>");
			printf("</tr>");
			printf("</table>"); 
		}
		else
		{
			printf("<tr>");
			printf("<td colspan=\"2\">");
			printf("<table width=\"502\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">");

			printf("<tr>");
			printf("<td width=\"1\" rowspan=\"15\" class=\"vline\"><br> </td>");
			printf("<td colspan=\"1\">");
			printf("<table width=\"430\" border=\"0\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" class=\"space\">");
			
			printf("<tr>");
			if(wan == 0)
			{
				printf("<td valign=\"middle\"><span style=\"vertical-align: middle\"><font size=\"2\">单参优化: 频点信道</font></span></td>");
				wt_para[0] = (uint8_t)nodNum;
			}
			else if(wan == 1)
			{
				printf("<td valign=\"middle\"><span style=\"vertical-align: middle\"><font size=\"2\">单参优化: 通信带宽</font></span></td>");
				wt_para[1] = (uint8_t)hopNum;
			}
			else if(wan == 2)
			{
				printf("<td valign=\"middle\"><span style=\"vertical-align: middle\"><font size=\"2\">单参优化: 发射功率</font></span></td>");
				wt_para[2] = (uint8_t)retNum;
			}
			else if(wan == 3)
			{
				printf("<td valign=\"middle\"><span style=\"vertical-align: middle\"><font size=\"2\">全参优化: 一键优化</font></span></td>");
				wt_para[0] = (uint8_t)nodNum;
				wt_para[1] = (uint8_t)hopNum;
				wt_para[2] = (uint8_t)retNum;
			}

			printf("</tr>");
			printf("</table>");
			printf("</td>");
			printf("<td width=\"1\" rowspan=\"15\" class=\"vline\"><br> </td>");
			printf("</tr>");
            printf("<tr>");
			printf("<td colspan=\"1\" class=\"hline\"><img src=\"../userRpm/images/empty.gif\" width=\"1\" height=\"1\"></td>");
			printf("</tr>");

			printf("<tr>");
			printf("<td colspan=\"1\">"); 
			printf("<table width=\"430\" border=\"0\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" class=\"space\">");
			printf("<tr>");  
            printf("<td valign=\"middle\"><span style=\"vertical-align: middle\"><font size=\"2\">频点信道:  %ld</font></span></td>",nodNum);  
			printf("</tr>"); 
			printf("</table>");
			printf("</td>"); 
			printf("</tr>");
			printf("<tr>");
			printf("<td colspan=\"1\" class=\"hline\"><img src=\"../userRpm/images/empty.gif\" width=\"1\" height=\"1\"></td>");
			printf("</tr>");

			printf("<tr>");
			printf("<td colspan=\"1\">");
			printf("<table width=\"430\" border=\"0\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" class=\"space\">");
			printf("<tr>");  
            printf("<td valign=\"middle\"><span style=\"vertical-align: middle\"><font size=\"2\">通信带宽:  %ld</font></span></td>",hopNum);  
			printf("</tr>"); 
			printf("</table>");
			printf("</td>"); 
			printf("</tr>");
            printf("<tr>");
			printf("<td colspan=\"1\" class=\"hline\"><img src=\"../userRpm/images/empty.gif\" width=\"1\" height=\"1\"></td>");
			printf("</tr>");

			printf("<tr>");
			printf("<td colspan=\"1\">");
			printf("<table width=\"430\" border=\"0\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" class=\"space\">");
			printf("<tr>");  
            printf("<td valign=\"middle\"><span style=\"vertical-align: middle\"><font size=\"2\">发射功率:  %ld</font></span></td>",retNum);  
			printf("</tr>"); 
			printf("</table>");
			printf("</td>"); 
			printf("</tr>");
            printf("<tr>");
			printf("<td colspan=\"1\" class=\"hline\"><img src=\"../userRpm/images/empty.gif\" width=\"1\" height=\"1\"></td>");
			printf("</tr>"); 

			printf("<tr>");
			printf("<td height=\"30\" class=\"tail\" align=\"right\">&nbsp;");  
			printf("<input type=\"submit\" class=\"button\"  value=\"完成\">"); 
			printf("</td>");
			printf("</tr>");
			printf("<tr>");
			printf("<td colspan=\"1\" class=\"hline\"><img src=\"../userRpm/images/empty.gif\" width=\"1\" height=\"1\"></td>");
			printf("</tr>");

			printf("</table>");
			printf("</td>");
			printf("</tr>");
		}
	}
	printf("</table>");

	printf("</form>"); 
	printf("</center></body>\n");
	printf("</html>\n");

	return 0;
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
	// int len = 0;
	
	f = fopen(filename,"wb");
    // if(f == NULL)
	// {
	// 	fprintf(stderr,"open write file failed!\n");
	// 	exit(-1);
	// }
	fseek(f,0,SEEK_END);
	// len = ftell(f);
	fseek(f,0,SEEK_SET);

    // printf("read file %s complete, len=%d.\n",filename,len);

    fwrite(data,1,strlen(data),f);
	if(f != NULL) 
        fclose(f);
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

    if (!para)
	{
		// printf("Error Single Req: [%s]\n",cJSON_GetErrorPtr());
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
*   brief:  将web端获取组网参数保存到wt_request.json
			(注意:需要将信道参数的10/11/12/13/14转换为json文件字符型1/1A/1B/1C/1D)
*   retval: 0 success
            -1 failed
*   params: buf 组网参数
            0   channel     信道号 取值范围1-11，需转换为字符型(0x1/0x1A/0x1B/0x1C/0x1D)
            1   chanbw      频宽Mhz  "2"/"5"/"10"/"20"
            2   txpower     功率dBm  3-33，需转换为字符型
*/
int SetWTParams_Json(uint8_t *buf)
{
    uint8_t char_to_num[3] = {0};
    char channel_add_word[128] = {0};
    uint8_t last = 0,add = 0;

    memset(char_to_num,0,3);
    if(buf[0] != 0) 
    {
        if(((buf[0] >= 10) && (buf[0] <= 14)) || \
        ((buf[0] >= 20) && (buf[0] <= 24)) || \
        ((buf[0] >= 30) && (buf[0] <= 34)) || \
        ((buf[0] >= 40) && (buf[0] <= 44)) || \
        ((buf[0] >= 50) && (buf[0] <= 54)) || \
        ((buf[0] >= 60) && (buf[0] <= 64)) || \
        ((buf[0] >= 70) && (buf[0] <= 74)) || \
        ((buf[0] >= 80) && (buf[0] <= 84)) || \
        ((buf[0] >= 90) && (buf[0] <= 94)) || \
        ((buf[0] >= 100) && (buf[0] <= 104)) || \
        ((buf[0] >= 110) && (buf[0] <= 114)))
        {
            add = buf[0] / 10;
            last = buf[0] % 10;     
            if(last == 0)
            {
                if((add < 10) && (add > 0))
                {
                    char_to_num[1] = add + 0x30;
                    set_single_para_req("set_mesh_para","channel",(char *)&char_to_num[1]);
                }
                else if((add >= 10) && (add <= 11))
                {
                    char_to_num[1] = add % 10 + 0x30;
                    char_to_num[0] = add / 10 + 0x30;
                    set_single_para_req("set_mesh_para","channel",(char *)&char_to_num[0]);
                }
            }
            else if((last >= 1) && (last <= 4))
            {
                last += 0x40;   //转化为字符型ABCD
                if((add < 10) && (add > 0))
                {
                    char_to_num[1] = add + 0x30;
                    memcpy(channel_add_word,(char *)char_to_num+1,1);
                    memcpy(channel_add_word+1,(char *)&last,1);
                    set_single_para_req("set_mesh_para","channel",channel_add_word);
                }
                else if((add >= 10) && (add <= 11))
                {
                    char_to_num[1] = add % 10 + 0x30;
                    char_to_num[0] = add / 10 + 0x30;
                    memcpy(channel_add_word,(char *)char_to_num,2);
                    memcpy(channel_add_word+2,(char *)&last,1);
                    set_single_para_req("set_mesh_para","channel",channel_add_word);
                }
            }
        }
    }
    if(buf[1] != 0)
    {
        switch (buf[1])
        {
            case 2:
                set_single_para_req("set_mesh_para","chanbw","2");
                break;
            case 5:
                set_single_para_req("set_mesh_para","chanbw","5");
                break;
            case 10:
                set_single_para_req("set_mesh_para","chanbw","10");
                break;
            case 20:
                set_single_para_req("set_mesh_para","chanbw","20");
                break;
            default:
                break;
        }
    }
    memset(char_to_num,0,3);
    if(buf[2] != 0) 
    {
        if((buf[2] < 10) && (buf[2] >= 3))
        {
            char_to_num[1] = buf[2] + 0x30;
            set_single_para_req("set_mesh_para","txpower",(char *)&char_to_num[1]);
        }
        else if((buf[2] >= 10) && (buf[2] <= 33))
        {
            char_to_num[1] = buf[2] % 10 + 0x30;
            char_to_num[0] = buf[2] / 10 + 0x30;
            set_single_para_req("set_mesh_para","txpower",(char *)&char_to_num[0]);
        }   
    }
    return 0;
}

int main(void)
{
	memset(wt_para,0,3);
	web_cgi_input();
    SetWTParams_Json(wt_para);
	return 0;
}