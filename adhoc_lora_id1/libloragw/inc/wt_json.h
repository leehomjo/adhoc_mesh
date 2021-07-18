#ifndef __WT_JSON_H__
#define __WT_JSON_H__

#define SIGNLE_JSON_LEN 2048
#define WT_DEV_ID       1   //高速设备本机ID

struct wt_json
{
    char single_req[SIGNLE_JSON_LEN]; //单个json文件请求
    char single_resp[SIGNLE_JSON_LEN]; //单个json文件响应
    char secret_key[128];   //密钥
};

typedef enum
{
    LOGIN = 0,
    CHANGE_WORD,
    GET_MESH_STATUS,
    SET_MESH_PARA,
    APPLY_MESH_PARA,
    GET_WIFI_STATUS,
    SET_WIFI_PARA,
    APPLY_WIFI_PARA,
    GET_LAN_STATUS,
    SET_LAN_PARA,
    APPLY_LAN_PARA,
    GET_SERIAL_STATUS,
    SET_SERIAL_PARA,
    APPLY_SERIAL_PARA,
    GET_SYS_STATUS,
    GET_GPS,
    GET_CONNECTED_MESH,
    GET_MESH_MSG,    
}wt_mesh_para;

void my_string_cat(char *str1,char *str2,char *out);
int32_t my_pow(int32_t a,int32_t b);
void SetJsonRPC_Req(char *data);
void GetJsonRPC_Resp(char *data,int time);
void wt_read_file(char *filename,char *data);
void wt_write_file(char *filename,char *data);
void get_json_content(int line, char *source, char *out);
int set_single_para_req(char *obj_out,char *obj_in,char *value);
int get_single_para_resp(char *obj_out,char *obj_in,char *value);

int WT_GetMeshMsg(uint8_t *device_id);
int WT_GetLocalSetParams(uint8_t *buf);
int WT_Login(void);
int WT_SendGroupReq(const char *name);
int WT_SaveGroupResp(const char *name,int time);
int WT_GroupProcess(int name_num);
int Lora_SetWTParams(uint8_t *buf);
int Lora_GetWTParams(uint8_t *wt_params);

#endif
