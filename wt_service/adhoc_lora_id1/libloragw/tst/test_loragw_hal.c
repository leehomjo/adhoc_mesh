/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2013 Semtech-Cycleo

Description:
    Minimum test program for the loragw_hal 'library'

License: Revised BSD License, see LICENSE.TXT file include in the project
Maintainer: Sylvain Miermont
*/


/* -------------------------------------------------------------------------- */
/* --- DEPENDANCIES --------------------------------------------------------- */

/* fix an issue between POSIX and C99 */
#if __STDC_VERSION__ >= 199901L
    #define _XOPEN_SOURCE 600
#else
    #define _XOPEN_SOURCE 500
#endif

#include <stdint.h>        /* C99 types */
#include <stdbool.h>       /* bool type */
#include <stdio.h>         /* printf */
#include <math.h>
#include <stddef.h>
#include <string.h>        /* memset */
#include <strings.h>
#include <signal.h>        /* sigaction */
#include <unistd.h>        /* getopt access */
#include <fcntl.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h> 
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <semaphore.h>

#include "loragw_hal.h"
#include "loragw_reg.h"
#include "loragw_aux.h"
#include "crc8.h"
#include "sx1278.h"
#include "tx1278-LoRa.h"

#include "ev.h"
#include "cJSON.h"
#include "wt_json.h"

static uint32_t context_id_table[6][2] = {0}; //[i][0]:接入终端id [i][1]:接入终端信道频点
static uint8_t connected_terminor_id[6] = {0}; 

/* 定时器*/
#define MAX_TIMEOUT_LEN (10)
#define TIME_OUT_TIME   (10) // 超时时间
static uint32_t cur_time = 0;   //计数自加变量
static uint32_t time_table[MAX_TIMEOUT_LEN][3] = {0}; // [0]:ID [1]:超时时间 [2]:分配信道�?
static uint32_t cur_cnt = 0; // 当前计数
static void time_out_func(union sigval v);

/* 发送接收线�? */
void *ex_recv_server(void *args);
void *ex_send_broadcast_server(void *args);
void *ex_send_ack_server(void *args);
void *ex_send_business_server(void *args);
void *ex_wt_service(void *args);

/* 读写�? */
pthread_rwlock_t rwlock;

/* 信号�? */
sem_t semaphore_tx;
sem_t semaphore_rx;
sem_t semaphore_all;

// union semun
// {
// 	int32_t val;
// 	struct semid_ds *buf;
// 	unsigned short *arry;
// };
// static int32_t sem_id = 0;

/* 消息队列 */
#define MSG_ID  (0x8888)
struct msg
{
    uint8_t msg_type;
    uint8_t rx_id;
    uint8_t cur_chanel_num;
};

struct tx_config_msgbuf
{
    long mtype;
    struct msg message;
};
/* 发送接收配�? */
static void set_txpkt_config(uint32_t send_ft, int32_t send_xp, int32_t send_xq, uint8_t cr_lora);
static void set_rx_chanel_config(uint32_t rx_fa, uint32_t rx_fb , enum lgw_radio_type_e rx_radio_type);
static uint32_t gateway_get_node_frq(uint8_t id);
static uint8_t mac_id = 0x11; //默认终端ID

/* 下行数据帧类�? */
#define TERMINAL_NET_ID                     (0x01)
#define TERMINAL_MAC_ID                     mac_id
#define BUSINESS_ACK_LEN                    (21)
#define BROADCAST_LEN                       (21 + 12) // 扩展了12字节表示接入ID和接入ID状态
#define HEAD_LEN                            (6) //帧头到帧序号长度

#define NODE_FRAME_HEAD_1                   (0xDA)
#define NODE_FRAME_HEAD_2                   (0xC8)
#define NODE_NET_REQ                        (0x01) // 请求�?
#define NODE_NET_ACK_REQ                    (0x02) // 应答�?
#define NODE_SEND_BUSINESS                  (0x06) // 业务�?
#define NODE_SEND_WT_BUSINESS               (0x07) // 高速组网业务
#define NODE_SEND_HEARTBEAT                 (0x5A) // 心跳�?

#define GATEWAY_FRAME_HEAD_1                (0xEB)
#define GATEWAY_FRAME_HEAD_2                (0x60)
#define GATEWAY_FRAME_TYPE_BRODCAST         (0xFF)
#define GATEWAY_FRAME_TYPE_CONNECT          (0xB1)
#define GATEWAY_FRAME_TYPE_CONNECT_ACK      (0xB2)
#define GATEWAY_FRAME_TYPE_CONNECT_TIMEOUT  (0xBB)
#define GATEWAY_TO_GATEWAY_TYPE             (0xCF)
#define GATEWAY_SEND_BUSINESS               (0x60) // 业务�?
#define GATEWAY_SEND_WT_BUSINESS            (0x70) // 高速组网业务
#define GATEWAY_LEN                         (0x3) // 网关个数
#define VALID_DATA_LEN                      (128)

#define BUSINESS_WTPARA_SETREQ              (0xB1)
#define BUSINESS_WTPARA_SETRESP             (0x01)
#define BUSINESS_WTPARA_GETREQ              (0xB2)
#define BUSINESS_WTPARA_GETRESP             (0x02)
#define BUSINESS_WTPARA_GETSNR              (0xB3)
#define BUSINESS_WTPARA_GETSNR_RESP         (0x03)
#define BUSINESS_WTPARA_RSSI                (0xA4)
#define BUSINESS_WTPARA_NOISE               (0xA5)
#define BUSINESS_WTPARA_SNR                 (0xA6)

static uint32_t node_timeout[6] = {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF};
static uint32_t gateway_timeout[256] = {0};
static uint32_t connected_status = true; //连接状态标志位 节点使用，节点未连接时true／节点连接后为false
static uint32_t net_id = 1;
static uint8_t node_or_gateway_flag = true; //节点网关身份标志位 true:节点 false:网关 默认为节点
static uint8_t node_connecting = false; //节点连接标志，在接收到ID号大的广播后置为true
static uint32_t node_connecting_time = 0xFFFFFFFF;
static uint32_t gateway_table[GATEWAY_LEN] = {471000000,491000000,501000000};
static uint32_t gateway_id = 0;

struct business_wt_para //高速模块参数
{
    uint8_t channel;    //通道号
    uint8_t channel_num;//通道号数值
    uint8_t channel_num_add;    //通道号数值添加(A/B/C/D)
    uint8_t channel_dev;   //从高速设备中读回的通道号
    uint8_t channel_dev_add;    //从高速设备中读回的通道号数值后缀(A/B/C/D)
    uint8_t bw; //带宽
    uint8_t bw_value;   //带宽数值
    uint8_t bw_dev;    //从高速设备中读回的带宽
    uint8_t txpower;    //发射功率
    uint8_t txpower_value;  //发射功率数值
    uint8_t txpower_dev;   //从高速设备中读回的功率
    uint8_t source_node;    //源节点
    uint8_t busi_num;   //高速参数修改次数(任意节点发起的全局更新参数，注意进位溢出)
    uint8_t busi_type;      //高速业务帧类型　0xb1 0xb2 0xb3
    uint8_t device_id[128];  //ID号
};

struct lora_frame
{
    uint8_t frame_head_1; // 帧头
    uint8_t frame_head_2; // 帧头
    uint8_t frame_type; //类型
    uint8_t des_id_1; // 目的ID
    uint8_t des_id_2; // 设备ID
    uint8_t len; // 长度
    uint8_t src_id_1; // 源id
    uint8_t src_id_2; // 源id
    uint8_t frame_sn; // 帧序号
    uint8_t valid_data[VALID_DATA_LEN]; //帧有效数据包含源id和帧序号
};

struct gateway_frame
{
    uint32_t gateway_frq; // 网关频点
    uint8_t id[6]; // 网关连接的ID
};
static struct gateway_frame gateway_id_table[GATEWAY_LEN]; // context_id_table[i][0]

static uint8_t gateway_cnt = 1; // 发送计数，递增
static uint8_t rx_freq_table[6] = {0}; // 接收通道
static uint8_t connected_id_business_status[6] = {0}; //入网ID判断 0:连接失败 1：连接正常
static uint32_t busniess_frq = 470000000;
static uint32_t restart_rx = false; //重启广播标志位
//static uint32_t business_data_len = 50;

static uint8_t chanel_status = 0xC0; // 信道状态
static void set_chanel_status(uint8_t chanel, uint8_t flag); // 设置信道状态
static uint8_t get_chanel_status(void); //获取空闲信道
static void set_switch_frq_rx(uint32_t fa_frq, uint32_t fb_frq);
static bool wt_input_refresh(void); 
static void wt_idtype_set(uint8_t *buf,uint8_t dev_id);
/* 速率�? */
#define SPEED_500BPS        (1)
#define SPEED_1700BPS       (2)
#define SPEED_2400BPS       (3)
#define SEND_SPEED_0        (0)
#define SEND_SPEED_1        (1)
#define SEND_SPEED_2        (2)
#define SEND_SPEED_3        (3)
#define SEND_SPEED_4        (4)
#define SEND_SPEED_5        (5)
#define SEND_SPEED_6        (6)
#define SEND_SPEED_7        (7)
#define SEND_SPEED_8        (8)
#define SEND_SPEED_9        (9)
#define SEND_SPEED_10       (10)
#define SEND_SPEED_11       (11)
#define SEND_SPEED_12       (12)
#define SEND_SPEED_13       (13)
#define SEND_SPEED_14       (14)
#define SEND_SPEED_15       (15)

#define WT_CHANBW_5MHZ      (5)
#define WT_CHANBW_10MHZ     (10)
#define WT_CHANBW_20MHZ     (20)

static const uint8_t LoRa_DataRate_Table[][3] =
{
  { 7, 12, 1 },   //0.3kbps,       BW: 125KHz, Spreading Factor: 12, Error Coding value: 4/5,
  { 7, 11, 1 },   //0.5kbps,        BW: 125KHz, Spreading Factor: 11, Error Coding value: 4/5, ch0
  { 7, 10, 1 },   //1.0kbps,        BW: 125KHz, Spreading Factor: 10, Error Coding value: 4/5,
  { 7, 9 , 1 },   //1.7kbps,         BW: 125KHz, Spreading Factor: 9,  Error Coding value: 4/5,
  { 7, 8 , 1 },   //3.1kbps,         BW: 125KHz, Spreading Factor: 8,  Error Coding value: 4/5,
  { 7, 7 , 1 },   //5.5kbps        BW: 125KHz, Spreading Factor: 7,   Error Coding value: 4/5,
  { 7, 12, 2 },   //0.3kbps,       BW: 125KHz, Spreading Factor: 12, Error Coding value: 4/6,
  { 7, 11, 2 },   //0.5kbps,        BW: 125KHz, Spreading Factor: 11, Error Coding value: 4/6,
  { 7, 10, 2 },   //0.8kbps,        BW: 125KHz, Spreading Factor: 10, Error Coding value: 4/6,
  { 7, 9 , 2 },   //1.5kbps,         BW: 125KHz, Spreading Factor: 9,  Error Coding value: 4/6, ch7
  { 7, 8 , 2 },   //2.6kbps,         BW: 125KHz, Spreading Factor: 8,  Error Coding value: 4/6,  ch8
  { 7, 7 , 2 },   //4.5kbps        BW: 125KHz, Spreading Factor: 7,   Error Coding value: 4/6,
  { 7, 10, 3 },   //0.7kbps,        BW: 125KHz, Spreading Factor: 10, Error Coding value: 4/7,
  { 7, 9 , 3 },   //1.2kbps,         BW: 125KHz, Spreading Factor: 9,  Error Coding value: 4/7, ch11
  { 7, 8 , 3 },   //2.2kbps,         BW: 125KHz, Spreading Factor: 8,  Error Coding value: 4/7,  
  { 7, 7 , 3 }    //3.9kbps        BW: 125KHz, Spreading Factor: 7,   Error Coding value: 4/7,
};

/* -------------------------------------------------------------------------- */
/* --- PRIVATE MACROS ------------------------------------------------------- */

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/* -------------------------------------------------------------------------- */
/* --- PRIVATE CONSTANTS ---------------------------------------------------- */

#define DEFAULT_RSSI_OFFSET 0.0
#define DEFAULT_NOTCH_FREQ  129000U

/* -------------------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */

static int32_t exit_sig = 0; /* 1 -> application terminates cleanly (shut down hardware, close open files, etc) */
static int32_t quit_sig = 0; /* 1 -> application terminates without shutting down the hardware */

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DECLARATION ---------------------------------------- */

static void sig_handler(int32_t sigio);

/* -------------------------------------------------------------------------- */
/* --- PRIVATE FUNCTIONS DEFINITION ----------------------------------------- */

static void sig_handler(int32_t sigio) {
    if (sigio == SIGQUIT) {
        quit_sig = 1;;
    } else if ((sigio == SIGINT) || (sigio == SIGTERM)) {
        exit_sig = 1;
    }
}

/* describe command line options */
void usage(void) {
    printf("Library version information: %s\n", lgw_version_info());
    printf( "Available options:\n");
    printf( " -h print this help\n");
    printf( " -a <float> Radio A RX frequency in MHz\n");
    printf( " -b <float> Radio B RX frequency in MHz\n");
    printf( " -t <float> Radio TX frequency in MHz\n");
    printf( " -p <uint8_t> Radio Power (20dBm-30dBm)\n");
    printf( " -i <uint8_t> ID(1-255))\n");
    printf( " -l <uint8_t> package size len(byte)\n");
}

/* -------------------------------------------------------------------------- */
/* --- MAIN FUNCTION -------------------------------------------------------- */

static struct lgw_pkt_tx_s broadcast_txpkt; /* configuration and metadata for an outbound packet */
static struct sigaction sigact; /* SIGQUIT&SIGINT&SIGTERM signal handling */

static struct lgw_conf_rxrf_s rfconf;
static struct lgw_conf_rxif_s ifconf;

static struct lgw_pkt_tx_s txpkt; /* configuration and metadata for an outbound packet */
static struct lgw_pkt_rx_s *p; /* pointer on a RX packet */
static int32_t speed = 1;
static uint32_t ft = 0;
static int32_t xp = 0;
static uint32_t frq_cyc = 0;

static struct business_wt_para wt_params; //高速设备初始参数
static bool wt_init_flag = false;   //初始化读取到密钥，获取组网参数后将标志位置位，执行后续高速设备控制操作
static bool wt_para_send_flag = false;  //读取到json文件发现参数变化，给其他高速设备通过lora
                                        //发送更改组网参数标志位，当自身参数设置成功后将该标志位置位
static bool wt_lora_timeout_flag = false;    //高速设备低速业务应答超时标志位
static bool wt_lora_timecnt_flag = false;   //高速设备超时计数启动标志位
static bool wt_meshpara_change_flag = false; //查询参数变化，避免查询本机参数与外部获取参数间产生冲突
static uint32_t wt_business_gateway_cnt = 0;
static uint32_t wt_business_node_cnt = 0; 
static uint32_t wt_business_timeout = 0;
static uint32_t wt_busi_broadcast = 0;   //网关发送高速参数到子网内所有节点，需重复发送置所有节点收到该次指令
static uint8_t wt_para_table[6][5] = {}; //高速参数表，保存每个子网内的所有节点高速参数　ID([0]数字/[4]后缀字母)／信道／带宽／功率

extern uint8_t TXLORAState; // 1278

static uint32_t fa_index = 0, fa = 0, fb = 0;
struct lgw_conf_board_s boardconf;
static enum lgw_radio_type_e radio_type = LGW_RADIO_TYPE_SX1255;

int32_t main(int32_t argc, char **argv)
{
    int32_t i;
    double xd = 0.0;
    // int32_t xi = 0;
    // int32_t xq = 0;
    uint32_t num = 0;
    // int32_t wt_chan = 0,wt_bw = 0,wt_pw = 0;
    struct lgw_conf_board_s boardconf;    
    // uint32_t arbitrary_value = 0x01;
    uint8_t clocksource = 1; /* Radio B is source by default */
    /* 创建消息队列 */
    int32_t msg_id;

    msg_id = msgget((key_t)MSG_ID, IPC_CREAT|0664);
    if(-1 == msg_id)
    {
        printf("create msg error!!!\n");
    }

    // sem_id = semget((key_t)1234,1,0666|IPC_CREAT);
    // if(!set_semvalue())
    // {
    //     printf("sem failed\n");
    // }

    pthread_rwlock_init(&rwlock, NULL);

    sem_init(&semaphore_tx, 0, 1);
    sem_init(&semaphore_rx, 0, 1);
    sem_init(&semaphore_all,0, 1);

    /* parse command line options */
    // while ((i = getopt (argc, argv, "ha:b:t:p:s:i:c:w:f:")) != -1) {
    while ((i = getopt (argc, argv, "ha:b:t:p:s:i:")) != -1) {
        switch (i) {
            case 'h':
                usage();
                return -1;
                break;
            case 'a': /* <float> Radio A RX frequency in MHz */
                sscanf(optarg, "%lf", &xd);
                fa = (uint32_t)((xd*1e6) + 0.5); /* .5 Hz offset to get rounding instead of truncating */
                break;
            case 'b': /* <float> Radio B RX frequency in MHz */
                sscanf(optarg, "%lf", &xd);
                fb = (uint32_t)((xd*1e6) + 0.5); /* .5 Hz offset to get rounding instead of truncating */
                break;
            case 't': /* <float> Radio TX frequency in MHz */
                sscanf(optarg, "%lf", &xd);
                ft = (uint32_t)((xd*1e6) + 0.5); /* .5 Hz offset to get rounding instead of truncating */
                break;
            // case 'r': /* <int32_t> Radio type (1255, 1257) */
            //     sscanf(optarg, "%i", &xi);
            //     switch (xi) {
            //         case 1255:
            //             radio_type = LGW_RADIO_TYPE_SX1255;
            //             break;
            //         case 1257:
            //             radio_type = LGW_RADIO_TYPE_SX1257;
            //             break;
            //         default:
            //             printf("ERROR: invalid radio type\n");
            //             usage();
            //             return -1;
            //     }
            //     break;
            case 'p':
                sscanf(optarg, "%i", &xp);
                printf("rf_power: %d\n", xp);
                break;
            // case 'd':
            //     sscanf(optarg, "%i", &xq);
            //     switch (xq) {
            //         case 7:
            //             xq = DR_LORA_SF7;
            //             break;
            //         case 8:
            //             xq = DR_LORA_SF8;
            //             break;
            //         case 9:
            //             xq = DR_LORA_SF9;
            //             break;
            //         case 10:
            //             xq = DR_LORA_SF10;
            //             break;
            //         case 11:
            //             xq = DR_LORA_SF11;
            //             break;
            //         case 12:
            //             xq = DR_LORA_SF12;
            //             break;
            //         default:
            //             printf("ERROR: invalid radio type\n");
            //             usage();
            //             return -1;
            //     }
            //     printf("Datarate: %d\n", xq);
            //     break;
            // case 'j':
            //     sscanf(optarg, "%i", &num);
            //     arbitrary_value = num;
            //     break;
            // case 'k': /* <int32_t> Concentrator clock source (Radio A or Radio B) */
            //     sscanf(optarg, "%i", &xi);
            //     clocksource = (uint8_t)xi;
            //     break;
            case 'i': 
                sscanf(optarg, "%i", &num);
                if((num > 0) && (num < 256))
                    mac_id = num;
                else
                    mac_id = 44;
                printf("ID:%d\n",mac_id);
                break;
            // case 'l': 
            //     sscanf(optarg, "%i", &num);
            //     if((num > 0) && (num <= 128))
            //         business_data_len = num;
            //     else
            //         business_data_len = 128;
            //     printf("Business data len:%d\n",business_data_len);
            //     break; 
            case 's': 
                sscanf(optarg, "%i", &num);
                switch (num)
                {
                case SPEED_500BPS/* constant-expression */:
                    speed = 1;
                    break;
                case SPEED_1700BPS/* constant-expression */:
                    speed = 3;
                    break;
                case SPEED_2400BPS/* constant-expression */:
                    speed = 10;
                    break;              
                default:
                    speed = 3;
                    break;
                }
                break;
            // case 'c':  //高速通道号
            //     sscanf(optarg, "%i", &wt_chan);
            //     if((wt_chan <= 11) && (wt_chan >= 1))
            //     {
            //         printf("wt_channel:%d\n",wt_chan);
            //         wt_params.channel_num = wt_chan;
            //         wt_params.channel = 0xc1;
            //     }
            //     else
            //     {
            //         wt_params.channel = 0;
            //         wt_params.channel_num = 0;
            //         printf("wt_channel_out_of_range\n");
            //     }
            //     break;
            // case 'w':  //高速带宽
            //     sscanf(optarg, "%i", &wt_bw);
            //     if((wt_bw == WT_CHANBW_5MHZ) || (wt_bw == WT_CHANBW_10MHZ) || (wt_bw == WT_CHANBW_20MHZ))
            //     {
            //         wt_params.bw = 0xc2;
            //         wt_params.bw_value = wt_bw;
            //         printf("wt_channel_bw:%dMHz\n",wt_bw);
            //     }
            //     else
            //     {
            //         wt_params.bw = 0;
            //         wt_params.bw_value = 0;
            //         printf("wt_channel_bw_error\n");
            //     }
            //     break;
            // case 'f':  //高速功率
            //     sscanf(optarg, "%i", &wt_pw);
            //     if((wt_pw <= 33) && (wt_chan >= 3))
            //     {
            //         printf("wt_power:%ddBm\n",wt_pw);
            //         wt_params.txpower = 0xc3;
            //         wt_params.txpower_value = wt_pw;
            //     }
            //     else
            //     {
            //         wt_params.txpower = 0;
            //         wt_params.txpower_value = 0;
            //         printf("wt_power_out_of_range\n");
            //     }
            //     break;
            default:
                printf("ERROR: argument parsing\n");
                usage();
                return -1;
        }
    }

    /* check input parameters */
    if ((fa == 0) || (fb == 0) || (ft == 0)) {
        printf("ERROR: missing frequency input parameter:\n");
        printf("  Radio A RX: %u\n", fa);
        printf("  Radio B RX: %u\n", fb);
        printf("  Radio TX: %u\n", ft);
        usage();
        return -1;
    }

    if (radio_type == LGW_RADIO_TYPE_NONE) {
        printf("ERROR: missing radio type parameter:\n");
        usage();
        return -1;
    }

    /* configure signal handling */
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigact.sa_handler = sig_handler;
    sigaction(SIGQUIT, &sigact, NULL);
    sigaction(SIGINT, &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);

    /* beginning of LoRa concentrator-specific code */
    printf("Beginning of test for loragw_hal.c\n");

    printf("*** Library version information ***\n%s\n\n", lgw_version_info());

    /* set configuration for board */
    memset(&boardconf, 0, sizeof(boardconf));

    boardconf.lorawan_public = false;
    boardconf.clksrc = clocksource;
    lgw_board_setconf(boardconf);

    set_rx_chanel_config(fa, fb, radio_type);

    SX1278_Init();   
    TXLORAState = TXLORA_STATE_TX_IDLE;
    set_power(xp);
    
    /* connect, configure and start the LoRa concentrator */
    i = lgw_start();
    if (i == LGW_HAL_SUCCESS) {
        printf("*** Concentrator started ***\n");
    } else {
        printf("*** Impossible to start concentrator ***\n");
        return -1;
    }

    /* once configured, dump content of registers to a file, for reference */
    // FILE * reg_dump = NULL;
    // reg_dump = fopen("reg_dump.log", "w");
    // if (reg_dump != NULL) {
    //     lgw_reg_check(reg_dump);
    //     fclose(reg_dump);
    // }

    sem_wait(&semaphore_all);//默认不发广播

    for ( i = 0; i < 6; i++)
    {
        context_id_table[i][1] = rx_freq_table[i];
    }

    pthread_t send_broadcast_server_thread;
    pthread_t send_business_server_thread;
    pthread_t recv_server_thread;
    pthread_t mt_business_server_thread;
    pthread_t wt_server_thread;

    if(pthread_create(&recv_server_thread,NULL,ex_recv_server, NULL))//创建接收子线�?
    { 
        perror("receive server thread create failed!");
    }
    else  
    {
        printf("receive server thread create ok!\n");
    }

    if(pthread_create(&send_broadcast_server_thread,NULL,ex_send_broadcast_server, NULL))//创建发送子线程
    { 
        perror("send broadcast server thread create failed!");
    } 
    else
    {
        printf("send broadcast server thread create ok!\n");
    }

    if(pthread_create(&send_business_server_thread,NULL,ex_send_ack_server, NULL))//创建发送子线程
    { 
        perror("send ack server thread create failed!");
    } 
    else
    {
        printf("send ack server thread create ok!\n");
    }

    if(pthread_create(&mt_business_server_thread,NULL,ex_send_business_server, NULL))//创建发送子线程
    { 
        perror("send business server thread create failed!");
    } 
    else
    {
        printf("send business server thread create ok!\n");
    }

    if(pthread_create(&wt_server_thread,NULL,ex_wt_service, NULL))//创建发送子线程
    { 
        perror("wt server thread create failed!");
    } 
    else
    {
        printf("wt server thread create ok!\n");
    }

    /*设置并启动定时器*/
    struct sigevent evp;
    struct itimerspec ts;
    timer_t timer;
    int32_t ret;
    memset(&evp, 0, sizeof(evp));
    evp.sigev_value.sival_ptr = &timer;
    evp.sigev_notify = SIGEV_THREAD;
    evp.sigev_notify_function = time_out_func;
    evp.sigev_value.sival_int = 3;   //作为handle()的参�?
    ret = timer_create(CLOCK_REALTIME, &evp, &timer);
    if(ret)
    {
        perror("timer_create");
    }

    ts.it_interval.tv_sec = 1;
    ts.it_interval.tv_nsec = 0;
    ts.it_value.tv_sec = 3;
    ts.it_value.tv_nsec = 0;
    ret = timer_settime(timer, TIMER_ABSTIME, &ts, NULL);
    if( ret )
    {
        perror("timer_settime");
    }

    while (getchar() != 'c')
    {
        
    }
    lgw_stop();

    // pthread_join(recv_server_thread,NULL);
    // pthread_join(send_broadcast_server_thread,NULL);
    // pthread_join(send_business_server_thread,NULL);
    // pthread_join(mt_business_server_thread,NULL);
    // pthread_join(wt_server_thread,NULL);

    return 0;
}

/* --- EOF ------------------------------------------------------------------ */

void *ex_wt_service(void *args)
{
    int32_t i = 0,cnt = 0;
    int32_t wt_device_cnt = 0,device_cnt = 0,send_once = 0,node_cnt = 0;
    uint8_t dev_id[128] = {0};
    uint8_t wt_json_para[4] = {0},wt_set_para[4] = {0};
    uint8_t wt_net[4] = {0};
    uint8_t wt_type[3] = {0};
    bool node_connected_gw_flag = false;

    i = WT_GroupProcess(LOGIN);
    if(i == -1)
    {
        perror("wt device login failed");
    }
    else //高速上电后自动组网不用初始化设置，获取组网参数判断参数变化即可
    {
        if(WT_GroupProcess(GET_MESH_STATUS) == -1)
        {
            perror("wt device get parameters failed");
        }
        else  //第一次获取组网参数后给全局变量赋值，并使能初始化成功标志位
        {
            wt_init_flag = true;
            Lora_GetWTParams(wt_set_para);
            wt_params.channel_num = wt_set_para[0];
            wt_params.bw_value = wt_set_para[1];
            wt_params.txpower_value = wt_set_para[2];
            wt_params.channel_num_add = wt_set_para[3];

            wt_params.channel_dev = wt_set_para[0];
            wt_params.bw_dev = wt_set_para[1];
            wt_params.txpower_dev = wt_set_para[2];
            wt_params.channel_dev_add = wt_set_para[3];
            memset(wt_set_para,0,4);

            wt_idtype_set(wt_type,WT_DEV_ID);
            wt_params.channel = wt_type[0];
            wt_params.bw = wt_type[1];
            wt_params.txpower = wt_type[2];
            WT_GetMeshMsg(wt_params.device_id);
            wt_input_refresh();
        }
    }
    while (1)
    {
        if(wt_init_flag)    //初始化成功
        {
            if(false == wt_meshpara_change_flag)
            {
                memset(wt_json_para,0,4);
                WT_GetLocalSetParams(wt_json_para); //每次接收节点发送来的修改参数时不应获取本地参数，避免修改参数冲突
                wt_params.channel_num = wt_json_para[0];
                wt_params.bw_value = wt_json_para[1];
                wt_params.txpower_value = wt_json_para[2];
                wt_params.channel_num_add = wt_json_para[3];
                // printf("run here\n");
            }
            
            if(true == wt_input_refresh())
            {
                wt_net[0] = wt_params.channel_num;
                wt_net[1] = wt_params.bw_value;
                wt_net[2] = wt_params.txpower_value;
                wt_net[3] = wt_params.channel_num_add;
                i = Lora_SetWTParams(wt_net);
                if(i == 0)
                {
                    wt_para_send_flag = true;   //本机高速组网参数设置成功
                    wt_params.busi_num++;
                    wt_busi_broadcast = 0;  //每次修改本机参数后，清零广播(节点网关都适用)
                    wt_meshpara_change_flag = false;    //无论上一次是哪种方式触发的更改，都将复位查询方式
                    if(0 == WT_GroupProcess(GET_MESH_STATUS))    //每次更改参数后获取一次参数，确保更改成功，用获取参数响应高速获取业务命令
                    {
                        Lora_GetWTParams(wt_set_para);
                        wt_params.channel_dev = wt_set_para[0];
                        wt_params.bw_dev = wt_set_para[1];
                        wt_params.txpower_dev = wt_set_para[2];
                        wt_params.channel_dev_add = wt_set_para[3];
                        if(wt_params.channel_dev_add == 0)
                        {        
                            printf("dev_channel:%d dev_bw:%d dev_power:%d\n",\
                            wt_params.channel_dev,wt_params.bw_dev,wt_params.txpower_dev);
                        }
                        else
                        {
                            printf("dev_channel:%d%x dev_bw:%d dev_power:%d\n",\
                            wt_params.channel_dev,wt_params.channel_dev_add,wt_params.bw_dev,wt_params.txpower_dev);
                        }
                        memset(wt_set_para,0,3);
                    }
                }
                memset(wt_net,0,4);
            }     
            else    //参数未发生变化时，显示本机高速参数
            {    
                if(0 == cnt % 5)
                {
                    if(wt_params.channel_dev_add == 0)
                    {        
                        printf("dev_channel:%d dev_bw:%d dev_power:%d\n",\
                        wt_params.channel_dev,wt_params.bw_dev,wt_params.txpower_dev);
                    }
                    else
                    {
                        printf("dev_channel:%d%x dev_bw:%d dev_power:%d\n",\
                        wt_params.channel_dev,wt_params.channel_dev_add,wt_params.bw_dev,wt_params.txpower_dev);
                    }
                }
            }
            //当作为网关或者节点已经入网，且已经修改参数后开始刷新高速节点连接拓扑
            if(node_or_gateway_flag == false)   //网关
            {
                for(i=0;i<6;i++)
                {
                    if(context_id_table[i][0] != 0) //判断是否有节点已连接，若无节点连接则不发网关高速业务
                    {
                        device_cnt = 1;
                        node_connected_gw_flag = true;  //确保网关有节点已经连接上
                        sem_post(&semaphore_all); //该段判断是否应该启动业务线程
                    }
                    else
                    {
                        node_cnt++; 
                    }
                }
                if((node_cnt == 6) && (node_connected_gw_flag))   //当没有节点连接到网关时，网关断开了节点连接
                {
                    sem_wait(&semaphore_all);   //等待信号量，阻塞业务线程
                    node_connected_gw_flag = false;
                }
            }
            else
            {
                if(connected_status == false)
                {
                    device_cnt = 1;
                }
            }
            if((wt_para_send_flag) && (send_once != wt_params.busi_num))    //当每次修改参数且已修改参数
            {
                device_cnt++;
            }
            send_once = wt_params.busi_num;
            if(device_cnt > 1)
            {
                device_cnt = 0;
                WT_GetMeshMsg(wt_params.device_id); 
                for(i=0;i<128;i++)
                {
                    if((wt_params.device_id[i] != WT_DEV_ID) && (wt_params.device_id[i] != 0)) //去除自身ID，保证ID为有效数据
                    {
                        dev_id[wt_device_cnt] = wt_params.device_id[i];
                        wt_device_cnt++;
                    }
                }
                if(wt_device_cnt > 0)
                {
                    printf("\ncurrent wt device number:%d \n self device id:%d ** connected device id:",wt_device_cnt,WT_DEV_ID);
                    for(i=0;i<wt_device_cnt;i++)
                    {
                        printf("%d ",dev_id[i]);
                    }
                    printf("\n");
                    wt_device_cnt = 0;
                }
            }
        }
        sleep(2);
        cnt++;
    }
}

/**************************更改代码**************************/
void *ex_send_business_server(void *args)
{
    uint32_t i = 0;
    struct tx_config_msgbuf tx_msg;
    int32_t msg_id;
    int32_t heart_cycle = 1;
    bool send_flag = false;

    msg_id = msgget(MSG_ID, 0);
    while (1)
    {
        sem_wait(&semaphore_all);
        if(node_or_gateway_flag == false)   //网关业务
        {
            if(wt_para_send_flag)   //本机高速组网参数设置成功后发送业务应答及其他子网的参数修改业务(立即发)
            {
                wt_business_gateway_cnt++;
                if(wt_business_gateway_cnt > 100)  //避免溢出错误
                {
                    wt_business_gateway_cnt = 2;
                }
                if(wt_business_gateway_cnt == 1) //第一次直接发送
                {
                    send_flag = true;
                }
                // 网关没有超时重发机制，设置和获取参数的主动过程都不需要
                // if(wt_lora_timeout_flag)    //未收到应答超时后执行后续发送，每次发送后关闭超时并刷新超时时间
                // {
                //     send_flag = true;
                //     wt_lora_timeout_flag = false;
                //     wt_business_timeout = cur_time;
                // }
                if(send_flag)
                {
                    send_flag = false;
                    //本机参数设置完后发送全局高速业务
                    wt_params.busi_type = BUSINESS_WTPARA_SETREQ;
                    tx_msg.message.rx_id = wt_params.source_node;
                    tx_msg.message.msg_type = GATEWAY_SEND_WT_BUSINESS;
                    tx_msg.mtype = GATEWAY_SEND_WT_BUSINESS;
                    if(msgsnd(msg_id, (void *)&tx_msg, sizeof(struct msg), 0) < 0)
                    {
                        printf("msg ack send failed\n");
                    }
                }
            }
            else    //应对网关的定时查询业务作区分，使用wt_params.busi_type
            {
                if(0 == heart_cycle % 5)
                {
                    send_flag = false;
                    wt_params.busi_type = BUSINESS_WTPARA_GETREQ;
                    tx_msg.message.rx_id = TERMINAL_MAC_ID; //全局发送
                    tx_msg.message.msg_type = GATEWAY_SEND_WT_BUSINESS;
                    tx_msg.mtype = GATEWAY_SEND_WT_BUSINESS;
                    if(msgsnd(msg_id, (void *)&tx_msg, sizeof(struct msg), 0) < 0)
                    {
                        printf("msg ack send failed\n");
                    }
                }
                sleep(9);
                heart_cycle++;
            }
        }
        else  //节点业务
        {
            if(false == connected_status)//入网后该标志位复位，开始作为节点发送业务和心跳
            {
                if(wt_para_send_flag)    //当发送业务帧时不发送心跳，仅发送一次
                {
                    wt_business_node_cnt++;
                    if(wt_business_node_cnt > 100)  //避免溢出错误
                    {
                        wt_business_node_cnt = 2;
                    }
                    else if(wt_business_node_cnt == 1) //第一次直接发送
                    {
                        send_flag = true;
                    }
                    // else
                    // {
                    //     if(wt_busi_broadcast > 0)
                    //     {
                    //         send_flag = true;
                    //     }
                    // }
                    // 网关没有超时重发机制，设置和获取参数的主动过程都不需要
                    if(wt_lora_timeout_flag)    //未收到应答超时后执行后续发送，每次发送后关闭超时并刷新超时时间
                    {
                        send_flag = true;
                        wt_lora_timeout_flag = false;
                        wt_business_timeout = cur_time;
                    }
                    if(send_flag)
                    {
                        send_flag = false;
                        wt_params.busi_type = BUSINESS_WTPARA_SETREQ;
                        tx_msg.message.rx_id = TERMINAL_NET_ID;
                        tx_msg.message.msg_type = NODE_SEND_WT_BUSINESS;
                        tx_msg.mtype = NODE_SEND_WT_BUSINESS;
                        if(msgsnd(msg_id, (void *)&tx_msg, sizeof(struct msg), 0) < 0)
                        {
                            printf("msg net req send failed\n");
                        }
                    }
                }
                else
                {
                    if(0 == heart_cycle % 2)
                    {
                        tx_msg.message.rx_id = TERMINAL_NET_ID;
                        tx_msg.message.msg_type = NODE_SEND_HEARTBEAT;
                        tx_msg.mtype = NODE_SEND_HEARTBEAT;
                        if(msgsnd(msg_id, (void *)&tx_msg, sizeof(struct msg), 0) < 0)
                        {
                            printf("msg net req send failed\n");
                        }
                    }
                    else
                    {
                        if(0 == heart_cycle % 5)
                        {
                            wt_params.busi_type = BUSINESS_WTPARA_GETREQ;
                            tx_msg.message.rx_id = TERMINAL_NET_ID;
                            tx_msg.message.msg_type = NODE_SEND_WT_BUSINESS;
                            tx_msg.mtype = NODE_SEND_WT_BUSINESS;
                            if(msgsnd(msg_id, (void *)&tx_msg, sizeof(struct msg), 0) < 0)
                            {
                                printf("msg net req send failed\n");
                            }
                        }
                    }
                    sleep(9);
                    heart_cycle++; 
                }
            }
        }
        sem_post(&semaphore_all);
        usleep(1000);
    }
}

void *ex_send_ack_server(void *args)
{
    int32_t i = 0,j = 0,cnt = 0;
    uint8_t chanel_num = 0;
    uint8_t crc;
    int32_t msg_id;
    struct lora_frame business_frame;
    struct tx_config_msgbuf rx_msg;
    uint32_t num_temp;
    uint32_t send_frq = ft; // 发送频�?
    bool send_busi_flag = false; //区分网关和节点的业务 true节点业务　false网关业务
    bool send_mgt_flag = false;  //区分业务和管理　true管理 false业务　

    msg_id = msgget(MSG_ID, 0);
    if(1==speed)
        num_temp = SEND_SPEED_1; // 发送需要延�?1000ms
    else if(2==speed)
        num_temp = SEND_SPEED_3; // 发送需要延�?300ms
    else
        num_temp = SEND_SPEED_10; // 发送需要延�?200ms

    while(1)
    {
        if(msgrcv(msg_id, &rx_msg, sizeof(struct msg) ,0 ,0) < 0)
        {
            printf("rcv msg failed!!\n");
        }   
        else
        {
            if(GATEWAY_FRAME_TYPE_CONNECT == rx_msg.message.msg_type)
            {
                business_frame.frame_head_1 = GATEWAY_FRAME_HEAD_1;
                business_frame.frame_head_2 = GATEWAY_FRAME_HEAD_2;
                send_frq = ft;
                send_busi_flag = false;
                send_mgt_flag = true;
                business_frame.frame_type = GATEWAY_FRAME_TYPE_CONNECT;

                for(i = 0; i < MAX_TIMEOUT_LEN; i++)
                {
                    if((time_table[i][0] == rx_msg.message.rx_id))
                    {
                        continue;
                    }
                } 
                /* ID校验 */
                for(i = 0; i < 6; i++)
                {
                    if(rx_msg.message.rx_id==context_id_table[i][0])
                        break;
                }
                /* 信道获取和设置 */
                chanel_num = get_chanel_status();
                if(i!=6)
                {
                    printf("ID已存在 通道号：%d\n",i);
                    // context_id_table[i][0] = 0;
                    // set_chanel_status(i+1, 0);
                    continue;
                }
                printf("通道号：%d %d\n",chanel_num,470000000 + context_id_table[chanel_num - 1][1] * 200000);
                busniess_frq = 470000000 + context_id_table[chanel_num - 1][1] * 200000;
                if(chanel_num > 6)
                {
                    printf("*****************************chanel out\n");
                    continue; // TODO:通道满了后续处理，信道恢复未处理
                }
                else
                {
                    business_frame.valid_data[0] = chanel_num;
                    /* 保存接入终端信息 */
                    context_id_table[chanel_num - 1][0] = rx_msg.message.rx_id;
                    printf("\n");
                    for(i = 0; i < 6; i++)
                    {
                        printf("ID:%d,Freq:%d\n", context_id_table[i][0], 470000000 + context_id_table[i][1] * 200000);
                    }
                }
                set_chanel_status(chanel_num, 1);
                
                /* 频点表、速率设置 */
                for(i = 0; i < 6; i++)
                {
                    business_frame.valid_data[i + 2] = context_id_table[i][1];
                    business_frame.valid_data[i + 2 + 6] = num_temp;
                }             
                /* 加入超时数组 */
                while (0!=time_table[cur_cnt][1])
                {
                    cur_cnt++;
                    if(cur_cnt >= MAX_TIMEOUT_LEN)
                        cur_cnt = 0;
                    else
                        cur_cnt++;
                }
                time_table[cur_cnt][0] = rx_msg.message.rx_id;
                time_table[cur_cnt][1] = cur_time + TIME_OUT_TIME;
                time_table[cur_cnt][2] = chanel_num;
            }
            else if(GATEWAY_FRAME_TYPE_CONNECT_ACK == rx_msg.message.msg_type)
            {
                business_frame.frame_head_1 = GATEWAY_FRAME_HEAD_1;
                business_frame.frame_head_2 = GATEWAY_FRAME_HEAD_2;
                send_frq = ft;
                send_busi_flag = false;
                send_mgt_flag = true;
                for(i = 0; i < MAX_TIMEOUT_LEN; i++)
                {
                    if((time_table[i][0] == rx_msg.message.rx_id))
                    {
                        time_table[i][1] = 0;
                    }
                }            
                business_frame.frame_type = GATEWAY_FRAME_TYPE_CONNECT_ACK;
                /* 频点表、速率设置 */
                for(i = 0; i < 6; i++)
                {
                    business_frame.valid_data[i + 2] = rx_freq_table[i];
                    business_frame.valid_data[i + 2 + 8] = num_temp;
                }
            }
            else if(GATEWAY_SEND_BUSINESS == rx_msg.message.msg_type)
            {
                send_frq = ft;
                send_busi_flag = false;
                send_mgt_flag = false;
                business_frame.frame_head_1 = GATEWAY_FRAME_HEAD_1;
                business_frame.frame_head_2 = GATEWAY_FRAME_HEAD_2;
                business_frame.frame_type = GATEWAY_SEND_BUSINESS;
            }
            else if(GATEWAY_SEND_WT_BUSINESS == rx_msg.message.msg_type)
            {
                send_frq = ft;
                send_busi_flag = false;
                send_mgt_flag = false;
                business_frame.frame_head_1 = GATEWAY_FRAME_HEAD_1;
                business_frame.frame_head_2 = GATEWAY_FRAME_HEAD_2;
                business_frame.frame_type = GATEWAY_SEND_WT_BUSINESS;
            }
            else if(NODE_NET_REQ == rx_msg.message.msg_type) // 入网请求�?
            {
                send_frq = ft - 800000;
                send_busi_flag = true;
                send_mgt_flag = true;
                business_frame.frame_head_1 = NODE_FRAME_HEAD_1;
                business_frame.frame_head_2 = NODE_FRAME_HEAD_2;
                business_frame.frame_type = NODE_NET_REQ;
                
            }
            else if(NODE_NET_ACK_REQ == rx_msg.message.msg_type) // 入网请求�?
            {
                //set_switch_frq_rx(busniess_frq, fb);
                send_frq = busniess_frq;
                send_busi_flag = true;
                send_mgt_flag = true;
                business_frame.frame_head_1 = NODE_FRAME_HEAD_1;
                business_frame.frame_head_2 = NODE_FRAME_HEAD_2;
                business_frame.frame_type = NODE_NET_ACK_REQ;
            }
            else if(NODE_SEND_WT_BUSINESS == rx_msg.message.msg_type)
            {
                send_frq = busniess_frq;
                send_busi_flag = true;
                send_mgt_flag = false;
                business_frame.frame_type = NODE_SEND_WT_BUSINESS;
                business_frame.frame_head_1 = NODE_FRAME_HEAD_1;
                business_frame.frame_head_2 = NODE_FRAME_HEAD_2;
            }
            else if(NODE_SEND_BUSINESS == rx_msg.message.msg_type)
            {
                send_frq = busniess_frq;
                send_busi_flag = true;
                send_mgt_flag = false;
                business_frame.frame_type = NODE_SEND_BUSINESS;
                business_frame.frame_head_1 = NODE_FRAME_HEAD_1;
                business_frame.frame_head_2 = NODE_FRAME_HEAD_2;
            }
            else if(NODE_SEND_HEARTBEAT == rx_msg.message.msg_type)
            {
                send_frq = busniess_frq;
                send_busi_flag = true;
                send_mgt_flag = true;
                business_frame.frame_type = NODE_SEND_HEARTBEAT;
                business_frame.frame_head_1 = NODE_FRAME_HEAD_1;
                business_frame.frame_head_2 = NODE_FRAME_HEAD_2;
                
            }
            else // 接收帧类型错误
            {
                continue;
            }

            business_frame.des_id_1 = TERMINAL_NET_ID;
            business_frame.des_id_2 = rx_msg.message.rx_id;
            business_frame.src_id_1 = TERMINAL_NET_ID;
            business_frame.src_id_2 = TERMINAL_MAC_ID;
            business_frame.valid_data[0] = chanel_num;
            business_frame.valid_data[1] = chanel_status;

            if(send_mgt_flag)  //管理帧
            {
                business_frame.len = BUSINESS_ACK_LEN;
            }
            else  //若不是管理帧
            {
                if(false == send_busi_flag)    //网关业务
                {
                    if(rx_msg.message.msg_type == GATEWAY_SEND_WT_BUSINESS) //高速节点业务
                    {
                        /** 网关响应当前节点数据　**/
                        if(wt_params.busi_type == BUSINESS_WTPARA_SETRESP) //网关响应高速参数设置业务
                        {
                            business_frame.valid_data[2] = wt_params.busi_num;
                            business_frame.valid_data[3] = BUSINESS_WTPARA_SETRESP;
                            business_frame.valid_data[4] = WT_DEV_ID;
                            business_frame.valid_data[5] = wt_params.channel;
                            business_frame.valid_data[6] = wt_params.channel;
                            business_frame.valid_data[7] = wt_params.bw;
                            business_frame.valid_data[8] = wt_params.bw;
                            business_frame.valid_data[9] = wt_params.txpower;
                            business_frame.valid_data[10] = wt_params.txpower;
                            business_frame.valid_data[11] = 0;
                            business_frame.len = 15;
                            wt_params.busi_type = 0;
                        }
                        //参数按节点序号排序，包含自身参数和子网内收到节点的参数，收到多少组发多少组，实时刷新
                        else if (wt_params.busi_type == BUSINESS_WTPARA_GETRESP) 
                        {
                            wt_para_table[WT_DEV_ID - 1][0] = WT_DEV_ID;
                            wt_para_table[WT_DEV_ID - 1][1] = wt_params.channel_dev;
                            wt_para_table[WT_DEV_ID - 1][2] = wt_params.bw_dev;
                            wt_para_table[WT_DEV_ID - 1][3] = wt_params.txpower_dev;
                            wt_para_table[WT_DEV_ID - 1][4] = wt_params.channel_dev_add;
                            for(i=0;i<6;i++)
                            {
                                if(wt_para_table[i][0] != 0)
                                {
                                    switch(wt_para_table[i][0])
                                    {
                                        case 1:
                                            for(j=0;j<4;j++)
                                            {
                                                business_frame.valid_data[8*cnt+4+2*j] = wt_para_table[i][j];
                                                if(j > 0)
                                                {
                                                    business_frame.valid_data[8*cnt+3+2*j] = 0xa0 | j;
                                                }
                                                if(j == 3)
                                                {
                                                    business_frame.valid_data[8*cnt+11] = wt_para_table[i][4];
                                                }
                                            }
                                            break;
                                        case 2:
                                            for(j=0;j<4;j++)
                                            {
                                                business_frame.valid_data[8*cnt+4+2*j] = wt_para_table[i][j];
                                                if(j > 0)
                                                {
                                                    business_frame.valid_data[8*cnt+3+2*j] = 0xb0 | j;
                                                }
                                                if(j == 3)
                                                {
                                                    business_frame.valid_data[8*cnt+11] = wt_para_table[i][4];
                                                }
                                            }
                                            break;
                                        case 3:
                                            for(j=0;j<4;j++)
                                            {
                                                business_frame.valid_data[8*cnt+4+2*j] = wt_para_table[i][j];
                                                if(j > 0)
                                                {
                                                    business_frame.valid_data[8*cnt+3+2*j] = 0xc0 | j;
                                                }
                                                if(j == 3)
                                                {
                                                    business_frame.valid_data[8*cnt+11] = wt_para_table[i][4];
                                                }
                                            }
                                            break;
                                        case 4:
                                            for(j=0;j<4;j++)
                                            {
                                                business_frame.valid_data[8*cnt+4+2*j] = wt_para_table[i][j];
                                                if(j > 0)
                                                {
                                                    business_frame.valid_data[8*cnt+3+2*j] = 0xd0 | j;
                                                }
                                                if(j == 3)
                                                {
                                                    business_frame.valid_data[8*cnt+11] = wt_para_table[i][4];
                                                }
                                            }
                                            break;
                                        case 5:
                                            for(j=0;j<4;j++)
                                            {
                                                business_frame.valid_data[8*cnt+4+2*j] = wt_para_table[i][j];
                                                if(j > 0)
                                                {
                                                    business_frame.valid_data[8*cnt+3+2*j] = 0xe0 | j;
                                                }
                                                if(j == 3)
                                                {
                                                    business_frame.valid_data[8*cnt+11] = wt_para_table[i][4];
                                                }
                                            }
                                            break;
                                        case 6:
                                            for(j=0;j<4;j++)
                                            {
                                                business_frame.valid_data[8*cnt+4+2*j] = wt_para_table[i][j];
                                                if(j > 0)
                                                {
                                                    business_frame.valid_data[8*cnt+3+2*j] = 0xf0 | j;
                                                }
                                                if(j == 3)
                                                {
                                                    business_frame.valid_data[8*cnt+11] = wt_para_table[i][4];
                                                }
                                            }
                                            break;
                                        default:
                                            break;
                                    }
                                    cnt++;
                                }
                            }
                            cnt = 0;
                            business_frame.len = 15 + cnt * 8;
                            wt_params.busi_type = 0;
                            business_frame.valid_data[2] = wt_params.busi_num;
                            business_frame.valid_data[3] = BUSINESS_WTPARA_GETRESP;
                        }
                        else if(wt_params.busi_type == BUSINESS_WTPARA_GETSNR_RESP)
                        {
                            business_frame.valid_data[2] = wt_params.busi_num;
                            business_frame.valid_data[3] = BUSINESS_WTPARA_GETSNR_RESP;
                            business_frame.valid_data[4] = WT_DEV_ID;
                            business_frame.valid_data[5] = BUSINESS_WTPARA_RSSI;
                            business_frame.valid_data[6] = 0xff;    //TODO
                            business_frame.valid_data[7] = BUSINESS_WTPARA_NOISE;
                            business_frame.valid_data[8] = 0xff;    //TODO
                            business_frame.valid_data[9] = BUSINESS_WTPARA_SNR;
                            business_frame.valid_data[10] = 0xff;   //TODO
                            business_frame.valid_data[11] = 0;      //TODO  
                            business_frame.len = 15;
                            wt_params.busi_type = 0;
                        }
                        /** 网关给子网内其余节点发送业务请求　**/
                        else if(wt_params.busi_type == BUSINESS_WTPARA_SETREQ) 
                        {
                            business_frame.valid_data[2] = wt_params.busi_num;
                            business_frame.valid_data[3] = BUSINESS_WTPARA_SETREQ;
                            business_frame.valid_data[4] = WT_DEV_ID;
                            business_frame.valid_data[5] = wt_params.channel;
                            business_frame.valid_data[6] = wt_params.channel_num;
                            business_frame.valid_data[7] = wt_params.bw;
                            business_frame.valid_data[8] = wt_params.bw_value;
                            business_frame.valid_data[9] = wt_params.txpower;
                            business_frame.valid_data[10] = wt_params.txpower_value;
                            business_frame.valid_data[11] = wt_params.channel_num_add;
                            business_frame.len = 15;
                            wt_params.busi_type = 0;
                        }
                        else if(wt_params.busi_type == BUSINESS_WTPARA_GETREQ)
                        {
                            business_frame.valid_data[2] = wt_params.busi_num;
                            business_frame.valid_data[3] = BUSINESS_WTPARA_GETREQ;
                            business_frame.valid_data[4] = WT_DEV_ID;
                            business_frame.valid_data[5] = wt_params.channel;
                            business_frame.valid_data[6] = 0;
                            business_frame.valid_data[7] = wt_params.bw;
                            business_frame.valid_data[8] = 0;
                            business_frame.valid_data[9] = wt_params.txpower;
                            business_frame.valid_data[10] = 0;
                            business_frame.valid_data[11] = 0;
                            business_frame.len = 15;
                            wt_params.busi_type = 0;
                        }
                        else if(wt_params.busi_type == BUSINESS_WTPARA_GETSNR)
                        {
                            business_frame.valid_data[2] = wt_params.busi_num;
                            business_frame.valid_data[3] = BUSINESS_WTPARA_GETREQ;
                            business_frame.valid_data[4] = WT_DEV_ID;
                            business_frame.valid_data[5] = BUSINESS_WTPARA_RSSI;
                            business_frame.valid_data[6] = 0;
                            business_frame.valid_data[7] = BUSINESS_WTPARA_NOISE;
                            business_frame.valid_data[8] = 0;
                            business_frame.valid_data[9] = BUSINESS_WTPARA_SNR;
                            business_frame.valid_data[10] = 0;
                            business_frame.valid_data[11] = 0;
                            business_frame.len = 15;
                            wt_params.busi_type = 0;
                        }
                        else
                        {
                            continue;
                        }
                        
                    }
                    else    //非高速节点网关业务 TODO (待做)
                    {
                        // business_frame.len = BUSINESS_ACK_LEN;
                        continue;
                    }
                }
                else    //节点业务
                {
                    if(NODE_SEND_WT_BUSINESS == rx_msg.message.msg_type) //高速节点业务
                    {
                        if(wt_params.busi_type == BUSINESS_WTPARA_SETREQ) //节点发送高速组网参数业务帧
                        {
                            business_frame.valid_data[2] = wt_params.busi_num;
                            business_frame.valid_data[3] = BUSINESS_WTPARA_SETREQ;
                            business_frame.valid_data[4] = WT_DEV_ID;
                            business_frame.valid_data[5] = wt_params.channel;
                            business_frame.valid_data[6] = wt_params.channel_num;
                            business_frame.valid_data[7] = wt_params.bw;
                            business_frame.valid_data[8] = wt_params.bw_value;
                            business_frame.valid_data[9] = wt_params.txpower;
                            business_frame.valid_data[10] = wt_params.txpower_value;
                            business_frame.valid_data[11] = wt_params.channel_num_add;
                            business_frame.len = 15;
                            wt_params.busi_type = 0;
                            wt_business_timeout = cur_time; //每次发送设置高速组网参数业务则更新超时时间并等待应答
                            wt_lora_timecnt_flag = true;    //开启超时计数
                        }
                        else if(wt_params.busi_type == BUSINESS_WTPARA_GETREQ)
                        {
                            business_frame.valid_data[2] = wt_params.busi_num;
                            business_frame.valid_data[3] = BUSINESS_WTPARA_GETREQ;
                            business_frame.valid_data[4] = WT_DEV_ID;
                            business_frame.valid_data[5] = wt_params.channel;
                            business_frame.valid_data[6] = 0;
                            business_frame.valid_data[7] = wt_params.bw;
                            business_frame.valid_data[8] = 0;
                            business_frame.valid_data[9] = wt_params.txpower;
                            business_frame.valid_data[10] = 0;
                            business_frame.valid_data[11] = 0;
                            business_frame.len = 15;
                            wt_params.busi_type = 0;
                        }
                        else if(wt_params.busi_type == BUSINESS_WTPARA_GETSNR)
                        {
                            business_frame.valid_data[2] = wt_params.busi_num;
                            business_frame.valid_data[3] = BUSINESS_WTPARA_GETREQ;
                            business_frame.valid_data[4] = WT_DEV_ID;
                            business_frame.valid_data[5] = BUSINESS_WTPARA_RSSI;
                            business_frame.valid_data[6] = 0;
                            business_frame.valid_data[7] = BUSINESS_WTPARA_NOISE;
                            business_frame.valid_data[8] = 0;
                            business_frame.valid_data[9] = BUSINESS_WTPARA_SNR;
                            business_frame.valid_data[10] = 0;
                            business_frame.valid_data[11] = 0;
                            business_frame.len = 15;
                            wt_params.busi_type = 0;
                        }
                        /** 节点响应当前网关的请求数据　**/
                        else if(wt_params.busi_type == BUSINESS_WTPARA_SETRESP) //网关响应高速参数设置业务
                        {
                            business_frame.valid_data[2] = wt_params.busi_num;
                            business_frame.valid_data[3] = BUSINESS_WTPARA_SETRESP;
                            business_frame.valid_data[4] = WT_DEV_ID;
                            business_frame.valid_data[5] = wt_params.channel;
                            business_frame.valid_data[6] = wt_params.channel;
                            business_frame.valid_data[7] = wt_params.bw;
                            business_frame.valid_data[8] = wt_params.bw;
                            business_frame.valid_data[9] = wt_params.txpower;
                            business_frame.valid_data[10] = wt_params.txpower;
                            business_frame.valid_data[11] = 0;
                            business_frame.len = 15;
                            wt_params.busi_type = 0;
                        }
                        else if (wt_params.busi_type == BUSINESS_WTPARA_GETRESP)
                        {
                            business_frame.valid_data[2] = wt_params.busi_num;
                            business_frame.valid_data[3] = BUSINESS_WTPARA_GETRESP;
                            business_frame.valid_data[4] = WT_DEV_ID;
                            business_frame.valid_data[5] = wt_params.channel;
                            business_frame.valid_data[6] = wt_params.channel_dev;
                            business_frame.valid_data[7] = wt_params.bw;
                            business_frame.valid_data[8] = wt_params.bw_dev;
                            business_frame.valid_data[9] = wt_params.txpower;
                            business_frame.valid_data[10] = wt_params.txpower_dev;
                            business_frame.valid_data[11] = wt_params.channel_dev_add;
                            business_frame.len = 15;
                            wt_params.busi_type = 0;
                        }
                        else if(wt_params.busi_type == BUSINESS_WTPARA_GETSNR_RESP)
                        {
                            business_frame.valid_data[2] = wt_params.busi_num;
                            business_frame.valid_data[3] = BUSINESS_WTPARA_GETSNR_RESP;
                            business_frame.valid_data[4] = WT_DEV_ID;
                            business_frame.valid_data[5] = BUSINESS_WTPARA_RSSI;
                            business_frame.valid_data[6] = 0xff;    //TODO
                            business_frame.valid_data[7] = BUSINESS_WTPARA_NOISE;
                            business_frame.valid_data[8] = 0xff;    //TODO
                            business_frame.valid_data[9] = BUSINESS_WTPARA_SNR;
                            business_frame.valid_data[10] = 0xff;   //TODO
                            business_frame.valid_data[11] = 0;      //TODO
                            business_frame.len = 15;
                            wt_params.busi_type = 0;
                        }
                        else
                        {
                            continue;
                        }
                    }
                    else    //非高速节点业务(TODO待做)
                    {
                        continue;
                        // business_frame.len = 7; //固定长度
                    }
                }
            }

            sem_wait(&semaphore_tx);
            business_frame.frame_sn = gateway_cnt++;
            crc = crc_8((const uint8_t *)&business_frame, HEAD_LEN + business_frame.len);
            memcpy(txpkt.payload, (void *)&business_frame, HEAD_LEN + business_frame.len);
            txpkt.payload[HEAD_LEN + business_frame.len] = crc;
            txpkt.size = HEAD_LEN + business_frame.len + 1; //头信�?+有效数据+crc
            /* 获取信号�?/�? */
            TX1278TxProcess(&txpkt.payload[0],txpkt.size,speed,send_frq); // 切频发�?
            /* 释放信号�?/�? */
            sem_post(&semaphore_tx);
            TXLORAState = TXLORA_STATE_TX_IDLE;
            /**********  数据打印信息start **********/
            printf("\nTX:type:req_0x%x freq_hz:%d rf_chain:%d rf_power:%d size:%3d cnt:%4d"\
                    ,business_frame.frame_type,send_frq,txpkt.rf_chain,xp,txpkt.size,business_frame.frame_sn);
            if(SEND_SPEED_3==speed)
                printf(" datarate:1.2Kbps\n");
            else if(SEND_SPEED_1==speed)
                printf(" datarate:0.5Kbps\n");
            else if(SEND_SPEED_10==speed)
                printf(" datarate:2.4Kbps\n");
            else
                printf("datarate:?\n");
            /**********  数据打印信息end **********/
        }
    }
}

void *ex_send_broadcast_server(void *args)
{
    int32_t i;
    uint8_t crc;
    uint32_t cycle_cnt = 0;
    uint32_t broadcast_send_frq = ft;
    struct lora_frame broadcast_send_data;
    uint32_t send_flag = 0;

    sleep(9); // 上电静默两个周期多才发送广播
    broadcast_send_data.frame_head_1 = GATEWAY_FRAME_HEAD_1;
    broadcast_send_data.frame_head_2 = GATEWAY_FRAME_HEAD_2;
    broadcast_send_data.frame_type = GATEWAY_FRAME_TYPE_BRODCAST;
    broadcast_send_data.des_id_1 = TERMINAL_NET_ID;
    broadcast_send_data.des_id_2 = TERMINAL_MAC_ID;
    broadcast_send_data.len = BROADCAST_LEN;
    broadcast_send_data.src_id_1 = TERMINAL_NET_ID;
    broadcast_send_data.src_id_2 = TERMINAL_MAC_ID;
    broadcast_send_data.valid_data[0] = 0x01;

    while(1)
    {
        pthread_rwlock_wrlock(&rwlock);
        if(restart_rx) // 重新启动接收广播
        {
            restart_rx = false;
            sleep(9);
        }
        node_or_gateway_flag = false;
        broadcast_send_data.valid_data[1] = chanel_status;
        printf("**********0x%x\n",chanel_status);

        for(i = 0; i < 6; i++)
        {
            broadcast_send_data.valid_data[i + 2] = rx_freq_table[i];
            broadcast_send_data.valid_data[i + 2 + 6] = SEND_SPEED_2;
            broadcast_send_data.valid_data[i + 2 + 6 + 6] = connected_terminor_id[i]; //接入ID表
            broadcast_send_data.valid_data[i + 2 + 6 + 6 + 6] = connected_id_business_status[i]; //接入ID状态
        }
        broadcast_send_data.valid_data[6 + 2 + 6 + 6 + 6] = gateway_id+1; //接入ID状态
        cycle_cnt++;
        if(0==cycle_cnt%5)
        {
            broadcast_send_data.frame_type = GATEWAY_TO_GATEWAY_TYPE;
            //broadcast_send_data.valid_data[0] = net_id; // 第0字节表示子网编号 子网表gateway_table
            for(i = 2; i < 8; i++)
            {
                broadcast_send_data.valid_data[i] = context_id_table[i-2][0]; // 第2-7字节表示当前网关接入的id
            }
            broadcast_send_data.valid_data[8] = (fb - 470000000) / 200000; // 第8字节表示当前网关接收频点
            if(net_id > 1)
            {
                if(fb==gateway_table[0])
                {
                    broadcast_send_frq = gateway_table[1] - 400000; // 长通知频点都是通过470.4发
                }
                else if(fb==gateway_table[1])
                {
                    broadcast_send_frq = gateway_table[0] - 400000; // 长通知频点都是通过470.4发
                } 
            }
            else if(net_id > 2)
            {
                if(fb==gateway_table[0])
                {
                    if(0==send_flag)
                    {
                        send_flag = 1;
                        broadcast_send_frq = gateway_table[1] - 400000; // 长通知频点都是通过480.4发
                    }
                    else 
                    {
                        send_flag = 0;
                        broadcast_send_frq = gateway_table[2] - 400000; // 长通知频点都是通过490.4发
                    }
                }
                else if(fb==gateway_table[1])
                {
                    if(0==send_flag)
                    {
                        send_flag = 1;
                        broadcast_send_frq = gateway_table[0] - 400000; // 长通知频点都是通过470.4发
                    }
                    else 
                    {
                        send_flag = 0;
                        broadcast_send_frq = gateway_table[2] - 400000; // 长通知频点都是通过490.4发
                    }
                }
                else if(fb==gateway_table[2])
                {
                    if(0==send_flag)
                    {
                        send_flag = 1;
                        broadcast_send_frq = gateway_table[0] - 400000; // 长通知频点都是通过470.4发
                    }
                    else 
                    {
                        send_flag = 0;
                        broadcast_send_frq = gateway_table[1] - 400000; // 长通知频点都是通过480.4发
                    }
                }
            }
            else
            {
                continue; // 长通知频点都是通过470.4发
            }  
        }
        else
        {
            broadcast_send_data.frame_type = GATEWAY_FRAME_TYPE_BRODCAST;
            broadcast_send_frq = ft;
        }
        sem_wait(&semaphore_tx);
        broadcast_send_data.frame_sn = gateway_cnt++;
        crc = crc_8((const uint8_t *)&broadcast_send_data, HEAD_LEN + broadcast_send_data.len);   
        memcpy(broadcast_txpkt.payload, (void *)&broadcast_send_data, HEAD_LEN + broadcast_send_data.len);       
        broadcast_txpkt.payload[HEAD_LEN + broadcast_send_data.len] = crc;
        broadcast_txpkt.size = HEAD_LEN + broadcast_send_data.len + 1; //头信息+有效数据+crc
        TX1278TxProcess(&broadcast_txpkt.payload[0],broadcast_txpkt.size,speed,broadcast_send_frq);
        sem_post(&semaphore_tx);/* 释放信号*/
        TXLORAState = TXLORA_STATE_TX_IDLE;
        pthread_rwlock_unlock(&rwlock);//解锁
        /**********  数据打印信息start **********/
        switch (broadcast_send_data.frame_type) 
        {
            case GATEWAY_FRAME_TYPE_BRODCAST: printf("\nTX:type:broadcast_0xFF"); break;
            case GATEWAY_TO_GATEWAY_TYPE: printf("\nTX:type:broadcast_0xCF"); break;
            default: printf("\nTX:type:data:?");
        }
        printf(" freq_hz:%d rf_chain:%d rf_power:%d size:%3d cnt:%4d"\
        ,broadcast_send_frq,broadcast_txpkt.rf_chain,xp,broadcast_txpkt.size,broadcast_send_data.frame_sn);
        if(SEND_SPEED_3==speed)
            printf(" datarate:1.2Kbps\n");
        else if(SEND_SPEED_1==speed)
            printf(" datarate:0.5Kbps\n");
        else if(SEND_SPEED_10==speed)
            printf(" datarate:2.4Kbps\n");
        else
            printf("datarate:?\n");
        /**********  数据打印信息end **********/
        usleep(4000000);  //TODO当节点ID到两个之后增大广播发送时间间隔
        
    }
}

void *ex_recv_server(void *args)
{
    uint32_t i = 0,j = 0;
    uint32_t wt_len = 0,node_cnt = 0,id_cnt = 0;
    int32_t nb_pkt,wt_busi_num = 0;
    uint8_t crc;
    struct tx_config_msgbuf tx_msg;
    struct lora_frame *rcv_data_p;
    struct lgw_pkt_rx_s rxpkt[4]; /* array containing up to 4 inbound packets metadata */
    int32_t msg_id;
    uint8_t buf[256];
    uint32_t rx_frame_cnt[256] = {0};
    uint32_t temp = 0;
    uint8_t time_out_cnt[6] = {0};


    msg_id = msgget(MSG_ID, 0);
    while(1)
    {
        /* fetch N packets */
        sem_wait(&semaphore_rx);
        nb_pkt = lgw_receive(ARRAY_SIZE(rxpkt), rxpkt);
        sem_post(&semaphore_rx);

        if (nb_pkt == 0) 
        {
            usleep(20000);//wait_ms(300);
        } 
        else 
        {
            /* display received packets */
            for(i=0; i < nb_pkt; ++i) 
            {
                p = &rxpkt[i];
                if (p->status == STAT_CRC_OK) 
                {
                    p->status = STAT_UNDEFINED;
                    memcpy(buf, p->payload, 256);
                    rcv_data_p = (struct lora_frame *)buf;
/*************************************  网关处理部分 *************************************/
                    //节点通信帧帧头校验DAC8，作为网关时收到该帧头
                    if((NODE_FRAME_HEAD_1 == rcv_data_p->frame_head_1) && \
                        (NODE_FRAME_HEAD_2 == rcv_data_p->frame_head_2))
                    {
                        crc = crc_8(p->payload, rcv_data_p->len + HEAD_LEN);
                        if(crc == p->payload[rcv_data_p->len + HEAD_LEN])
                        {
                            if(TERMINAL_MAC_ID == rcv_data_p->src_id_2) //过滤本机id
                            {
                                continue;
                            }
                            //帧类型判断
                            switch (rcv_data_p->frame_type)
                            {
                                case NODE_NET_REQ:  //作为网关收到节点入网请求0x01
                                    /**********  数据打印信息start **********/
                                    printf("\nRX:id:%3d type:",rcv_data_p->src_id_2);
                                    switch (rcv_data_p->frame_type) {
                                        case 0x01: printf("node_req_01"); break;
                                        case 0x02: printf("node_req_02"); break;
                                        case 0x06: printf("node_data_06"); break;
                                        case 0x5A: printf("node_heartbeat"); break;
                                        default: printf("node_data:?");
                                    }
                                    if(DR_LORA_SF9==p->datarate&&p->coderate==CR_LORA_4_5)
                                        printf(" datarate:1.2Kbps");
                                    else if(DR_LORA_SF11==p->datarate&&p->coderate==CR_LORA_4_5)
                                        printf(" datarate:0.5Kbps");
                                    else if(DR_LORA_SF8==p->datarate&&p->coderate==CR_LORA_4_6)
                                        printf(" datarate:2.4Kbps");
                                    else
                                        printf("datarate:?");
                                    printf(" cnt:%4d sn:%3d freq_hz:%d if_chain:%2d rssi:%3.3f snr:%2.3f size:%3u\n" \
                                            ,++rx_frame_cnt[rcv_data_p->src_id_2],rcv_data_p->frame_sn,fb-400000, \
                                            p->if_chain,p->rssi,p->snr,p->size);
                                    /**********  数据打印信息end **********/
                                    //作为网关接收到节点数据并将数据队列发送到发送线程 收到0x01回复0xB1
                                    if(false == node_or_gateway_flag) // TODO:多子网过滤非入本子网节点
                                    {
                                        printf("connecting internet\n");//表示收到请求正在连接，分配入网信道
                                        tx_msg.message.rx_id = rcv_data_p->src_id_2;
                                        tx_msg.message.msg_type = GATEWAY_FRAME_TYPE_CONNECT;
                                        tx_msg.mtype = GATEWAY_FRAME_TYPE_CONNECT;
                                        if(msgsnd(msg_id, (void *)&tx_msg, sizeof(struct msg), 0) < 0)
                                        {
                                            printf("msg net req send failed\n");
                                        }
                                    }
                                    break;
                                //TODO
                                case NODE_NET_ACK_REQ://作为网关收到节点确认入网请求0x02
                                    if(true == node_or_gateway_flag)
                                        continue;
                                    /**********  数据打印信息start **********/
                                    printf("\nRX:id:%3d type:",rcv_data_p->src_id_2);
                                    switch (rcv_data_p->frame_type) {
                                        case 0x01: printf("node_req_01"); break;
                                        case 0x02: printf("node_req_02"); break;
                                        case 0x06: printf("node_data_06"); break;
                                        case 0x5A: printf("node_heartbeat"); break;
                                        default: printf("node_data:?"); break;
                                    }
                                    if(DR_LORA_SF9==p->datarate&&p->coderate==CR_LORA_4_5)
                                        printf(" datarate:1.2Kbps");
                                    else if(DR_LORA_SF11==p->datarate&&p->coderate==CR_LORA_4_5)
                                        printf(" datarate:0.5Kbps");
                                    else if(DR_LORA_SF8==p->datarate&&p->coderate==CR_LORA_4_6)
                                        printf(" datarate:2.4Kbps");
                                    else
                                        printf("datarate:?");
                                    printf(" cnt:%4d sn:%3d freq_hz:%d if_chain:%2d rssi:%3.3f snr:%2.3f size:%3u\n"\
                                        ,++rx_frame_cnt[rcv_data_p->src_id_2],rcv_data_p->frame_sn,\
                                        gateway_get_node_frq(rcv_data_p->src_id_2),p->if_chain,p->rssi,p->snr,p->size);
                                    /**********  数据打印信息end **********/
                                    
                                    printf("connected internet\n");//表示已连接上                                   
                                    for(i = 0; i < 6; i++) // 重复入网过滤掉
                                    {
                                        if(connected_terminor_id[i]==TERMINAL_MAC_ID)
                                        {
                                            connected_id_business_status[i] = 1;
                                            break;
                                        }
                                    }
                                    //过滤掉重复ID后，对入网ID进行登记，并设置ID已连接
                                    if(i==6)
                                    {
                                        for(i = 0; i < 6; i++) // 入网成功设置广播信息中连接状态为1
                                        {
                                            if(connected_terminor_id[i]==0) 
                                            {
                                                connected_terminor_id[i] = rcv_data_p->src_id_2;//ID登记
                                                connected_id_business_status[i] = 1;
                                                // 从本次接收开始计算超时，初始设置超时为100s
                                                node_timeout[i] = cur_time + 100; 
                                                break;
                                            }
                                        }
                                    }
                                    //收到0x02后发送0xB2
                                    tx_msg.message.rx_id = rcv_data_p->src_id_2;
                                    tx_msg.message.msg_type = GATEWAY_FRAME_TYPE_CONNECT_ACK;
                                    tx_msg.mtype = GATEWAY_FRAME_TYPE_CONNECT_ACK;
                                    if(msgsnd(msg_id, (void *)&tx_msg, sizeof(struct msg), 0) < 0)
                                    {
                                        printf("msg ack send failed\n");
                                    }
                                    break;
                                case NODE_SEND_BUSINESS:    //网关收到节点发送来的业务数据0x06
                                    if(true == node_or_gateway_flag)    //身份是节点则跳过
                                        continue;
                                    /**********  数据打印信息start **********/
                                    printf("\nRX:id:%3d type:",rcv_data_p->src_id_2);
                                    switch (rcv_data_p->frame_type) {
                                        case 0x01: printf("node_req_01"); break;
                                        case 0x02: printf("node_req_02"); break;
                                        case 0x06: printf("node_data_06"); break;
                                        case 0x07: printf("node_data_07"); break;
                                        case 0x5A: printf("node_heartbeat"); break;
                                        default: printf("node_data:?"); break;
                                    }
                                    if(DR_LORA_SF9==p->datarate&&p->coderate==CR_LORA_4_5)
                                        printf(" datarate:1.2Kbps");
                                    else if(DR_LORA_SF11==p->datarate&&p->coderate==CR_LORA_4_5)
                                        printf(" datarate:0.5Kbps");
                                    else if(DR_LORA_SF8==p->datarate&&p->coderate==CR_LORA_4_6)
                                        printf(" datarate:2.4Kbps");
                                    else
                                        printf("datarate:?");
                                    printf(" cnt:%4d sn:%3d freq_hz:%d if_chain:%2d rssi:%3.3f snr:%2.3f size:%3u\n"\
                                            ,++rx_frame_cnt[rcv_data_p->src_id_2],rcv_data_p->frame_sn,\
                                            gateway_get_node_frq(rcv_data_p->src_id_2),p->if_chain,p->rssi,p->snr,p->size);
                                    /**********  数据打印信息end **********/
                                    //若身份是网关则每次接收到数据后为每个连接上的节点更新超时时间
                                    if(false == node_or_gateway_flag)
                                    {
                                        for(i = 0; i < 6; i++)
                                        {
                                            if(connected_terminor_id[i]==rcv_data_p->src_id_2)
                                            {
                                                connected_id_business_status[i] = 1;
                                                node_timeout[i] = cur_time + 100; // 更新超时时间
                                                //printf("BUS_NODE TIME out %d ID:%d\n",node_timeout[i], rcv_data_p->src_id_2);
                                                break;
                                            }
                                        }
                                    }
                                    break;
                                case NODE_SEND_WT_BUSINESS:    //网关收到节点发送来的业务数据0x06
                                    if(true == node_or_gateway_flag)    //身份是节点则跳过
                                        continue;
                                    /**********  数据打印信息start **********/
                                    printf("\nRX:id:%3d type:",rcv_data_p->src_id_2);
                                    switch (rcv_data_p->frame_type) {
                                        case 0x01: printf("node_req_01"); break;
                                        case 0x02: printf("node_req_02"); break;
                                        case 0x06: printf("node_data_06"); break;
                                        case 0x07: printf("node_data_07"); break;
                                        case 0x5A: printf("node_heartbeat"); break;
                                        default: printf("node_data:?"); break;
                                    }
                                    if(DR_LORA_SF9==p->datarate&&p->coderate==CR_LORA_4_5)
                                        printf(" datarate:1.2Kbps");
                                    else if(DR_LORA_SF11==p->datarate&&p->coderate==CR_LORA_4_5)
                                        printf(" datarate:0.5Kbps");
                                    else if(DR_LORA_SF8==p->datarate&&p->coderate==CR_LORA_4_6)
                                        printf(" datarate:2.4Kbps");
                                    else
                                        printf("datarate:?");
                                    printf(" cnt:%4d sn:%3d freq_hz:%d if_chain:%2d rssi:%3.3f snr:%2.3f size:%3u\n"\
                                            ,++rx_frame_cnt[rcv_data_p->src_id_2],rcv_data_p->frame_sn,\
                                            gateway_get_node_frq(rcv_data_p->src_id_2),p->if_chain,p->rssi,p->snr,p->size);
                                    /**********  数据打印信息end **********/
                                    //若身份是网关则每次接收到数据后为每个连接上的节点更新超时时间
                                    if(false == node_or_gateway_flag)
                                    {
                                        for(i = 0; i < 6; i++)
                                        {
                                            if(connected_terminor_id[i]==rcv_data_p->src_id_2)
                                            {
                                                connected_id_business_status[i] = 1;
                                                node_timeout[i] = cur_time + 100; // 更新超时时间
                                                //printf("BUS_NODE TIME out %d ID:%d\n",node_timeout[i], rcv_data_p->src_id_2);
                                                break;
                                            }
                                        }
                    /********** 网关解析节点发送的业务数据内容 **********/
                    //网关收到节点发送的高速组网业务参数请求，并将参数更新到本地全局（）
                    wt_busi_num = rcv_data_p->valid_data[2] - wt_params.busi_num;
                    if(wt_busi_num > 200)   //判断是否进位溢出
                    {
                        wt_busi_num = wt_params.busi_num + 255;
                    }
                    else if(wt_busi_num < -200)
                    {
                        wt_busi_num = rcv_data_p->valid_data[2] + 255;
                    }
                    else
                    {
                        wt_busi_num = rcv_data_p->valid_data[2];
                    }
                    if(wt_params.busi_num < wt_busi_num)
                    {
                        wt_params.busi_num = rcv_data_p->valid_data[2];
                        wt_busi_broadcast++;
                    }
                    switch (rcv_data_p->valid_data[3])
                    {
                        case BUSINESS_WTPARA_SETREQ:
                            if(wt_busi_broadcast > 0)
                            {
                                wt_meshpara_change_flag = true; //获取参数设置命令，此时关闭查询本机
                                switch (rcv_data_p->valid_data[4])  //根据id号接收相应数据参数并判断是否需要修改参数
                                {
                                    case 1:
                                        if((rcv_data_p->valid_data[5] == 0xa1) && \
                                        (wt_params.channel_num != rcv_data_p->valid_data[6]))
                                        {
                                            wt_params.channel_num = rcv_data_p->valid_data[6];
                                            wt_params.channel_num_add = rcv_data_p->valid_data[11];
                                        }
                                        if((rcv_data_p->valid_data[7] == 0xa2) && \
                                        (wt_params.bw_value != rcv_data_p->valid_data[8]))
                                        {
                                            wt_params.bw_value = rcv_data_p->valid_data[8];
                                        }
                                        if((rcv_data_p->valid_data[9] == 0xa3) && \
                                        (wt_params.txpower_value != rcv_data_p->valid_data[10]))
                                        {
                                            wt_params.txpower_value = rcv_data_p->valid_data[10];
                                        }
                                        break;
                                    case 2:
                                        if((rcv_data_p->valid_data[5] == 0xb1) && \
                                        (wt_params.channel_num != rcv_data_p->valid_data[6]))
                                        {
                                            wt_params.channel_num = rcv_data_p->valid_data[6];
                                            wt_params.channel_num_add = rcv_data_p->valid_data[11];
                                        }
                                        if((rcv_data_p->valid_data[7] == 0xb2) && \
                                        (wt_params.bw_value != rcv_data_p->valid_data[8]))
                                        {
                                            wt_params.bw_value = rcv_data_p->valid_data[8];
                                        }
                                        if((rcv_data_p->valid_data[9] == 0xb3) && \
                                        (wt_params.txpower_value != rcv_data_p->valid_data[10]))
                                        {
                                            wt_params.txpower_value = rcv_data_p->valid_data[10];
                                        }
                                        break;
                                    case 3:
                                        if((rcv_data_p->valid_data[5] == 0xc1) && \
                                        (wt_params.channel_num != rcv_data_p->valid_data[6]))
                                        {
                                            wt_params.channel_num = rcv_data_p->valid_data[6];
                                            wt_params.channel_num_add = rcv_data_p->valid_data[11];
                                        }
                                        if((rcv_data_p->valid_data[7] == 0xc2) && \
                                        (wt_params.bw_value != rcv_data_p->valid_data[8]))
                                        {
                                            wt_params.bw_value = rcv_data_p->valid_data[8];
                                        }
                                        if((rcv_data_p->valid_data[9] == 0xc3) && \
                                        (wt_params.txpower_value != rcv_data_p->valid_data[10]))
                                        {
                                            wt_params.txpower_value = rcv_data_p->valid_data[10];
                                        }
                                        break;
                                    case 4:
                                        if((rcv_data_p->valid_data[5] == 0xd1) && \
                                        (wt_params.channel_num != rcv_data_p->valid_data[6]))
                                        {
                                            wt_params.channel_num = rcv_data_p->valid_data[6];
                                            wt_params.channel_num_add = rcv_data_p->valid_data[11];
                                        }
                                        if((rcv_data_p->valid_data[7] == 0xd2) && \
                                        (wt_params.bw_value != rcv_data_p->valid_data[8]))
                                        {
                                            wt_params.bw_value = rcv_data_p->valid_data[8];
                                        }
                                        if((rcv_data_p->valid_data[9] == 0xd3) && \
                                        (wt_params.txpower_value != rcv_data_p->valid_data[10]))
                                        {
                                            wt_params.txpower_value = rcv_data_p->valid_data[10];
                                        }
                                        break;
                                    case 5:
                                        if((rcv_data_p->valid_data[5] == 0xe1) && \
                                        (wt_params.channel_num != rcv_data_p->valid_data[6]))
                                        {
                                            wt_params.channel_num = rcv_data_p->valid_data[6];
                                            wt_params.channel_num_add = rcv_data_p->valid_data[11];
                                        }
                                        if((rcv_data_p->valid_data[7] == 0xe2) && \
                                        (wt_params.bw_value != rcv_data_p->valid_data[8]))
                                        {
                                            wt_params.bw_value = rcv_data_p->valid_data[8];
                                        }
                                        if((rcv_data_p->valid_data[9] == 0xe3) && \
                                        (wt_params.txpower_value != rcv_data_p->valid_data[10]))
                                        {
                                            wt_params.txpower_value = rcv_data_p->valid_data[10];
                                        }
                                        break;
                                    case 6:
                                        if((rcv_data_p->valid_data[5] == 0xf1) && \
                                        (wt_params.channel_num != rcv_data_p->valid_data[6]))
                                        {
                                            wt_params.channel_num = rcv_data_p->valid_data[6];
                                            wt_params.channel_num_add = rcv_data_p->valid_data[11];
                                        }
                                        if((rcv_data_p->valid_data[7] == 0xf2) && \
                                        (wt_params.bw_value != rcv_data_p->valid_data[8]))
                                        {
                                            wt_params.bw_value = rcv_data_p->valid_data[8];
                                        }
                                        if((rcv_data_p->valid_data[9] == 0xf3) && \
                                        (wt_params.txpower_value != rcv_data_p->valid_data[10]))
                                        {
                                            wt_params.txpower_value = rcv_data_p->valid_data[10];
                                        }
                                        break;
                                    default:
                                        break;
                                }
                                if(rcv_data_p->valid_data[11] == 0)
                                {
                                    printf("wt_id:%d wt_channel:%d wt_bw:%d wt_power:%d\n",\
                                    rcv_data_p->valid_data[4],wt_params.channel_num,\
                                    wt_params.bw_value,wt_params.txpower_value);
                                }
                                else
                                {
                                    printf("wt_id:%d wt_channel:%d%x wt_bw:%d wt_power:%d\n",\
                                    rcv_data_p->valid_data[4],wt_params.channel_num,\
                                    wt_params.channel_num_add,wt_params.bw_value,wt_params.txpower_value);
                                }
                                
                                wt_params.source_node = rcv_data_p->src_id_2;
                                wt_params.busi_type = BUSINESS_WTPARA_SETRESP;
                            }
                            break;
                        case BUSINESS_WTPARA_GETREQ: //网关收到节点查询业务，将最近一次获取的参数在send_ack线程中返回给节点
                            //收到0x07，本机设置成功后，返回0x70，返回内容中包含业务应答
                            tx_msg.message.rx_id = rcv_data_p->src_id_2;
                            tx_msg.message.msg_type = GATEWAY_SEND_WT_BUSINESS;
                            tx_msg.mtype = GATEWAY_SEND_WT_BUSINESS;
                            if(msgsnd(msg_id, (void *)&tx_msg, sizeof(struct msg), 0) < 0)
                            {
                                printf("msg ack send failed\n");
                            }
                            wt_params.busi_type = BUSINESS_WTPARA_GETRESP;
                            break;
                        case BUSINESS_WTPARA_GETSNR:
                            //TODO  
                            wt_params.busi_type = BUSINESS_WTPARA_GETSNR_RESP;
                            break;
                        //网关收到节点发送的高速组网业务参数应答，并将参数更新到本地全局（）
                        case BUSINESS_WTPARA_SETRESP:   //收到设置参数的应答，则复位发送标志位，结束流程
                            //收到所有节点的应答数据后才能结束流程
                            for(i=0;i<6;i++)
                            {
                                if(context_id_table[i][0] != 0)
                                {
                                    node_cnt++;
                                }
                            }
                            for(j=0;j<6;j++)
                            {
                                if(rcv_data_p->valid_data[4] == i+1)
                                {
                                    id_cnt++;
                                }
                            }
                            if(node_cnt == id_cnt)
                            {
                                wt_para_send_flag = false;  //结束本次请求应答流程
                                wt_busi_broadcast = 0; //应在网关设置完，发送给所有子网id获取响应后清零
                                node_cnt = 0;
                                id_cnt = 0;
                            }
                            break;
                        case BUSINESS_WTPARA_GETRESP:   //网关收到响应参数，打印并保存到本地列表
                            for(i=0;i<6;i++)
                            {
                                if(rcv_data_p->valid_data[4] == i+1)
                                {
                                    wt_para_table[i][0] = rcv_data_p->valid_data[4];
                                    j = i;
                                }
                            }
                            wt_para_table[j][1] = rcv_data_p->valid_data[6];
                            wt_para_table[j][2] = rcv_data_p->valid_data[8];
                            wt_para_table[j][3] = rcv_data_p->valid_data[10];
                            wt_para_table[j][4] = rcv_data_p->valid_data[11];
                            if(wt_para_table[j][4] == 0)
                            {
                                printf("\nwt_id_%d channel:%d bw:%d power:%d\n",wt_para_table[j][0],\
                                wt_para_table[j][1],wt_para_table[j][2],wt_para_table[j][3]);
                            }
                            else
                            {
                                printf("\nwt_id_%d channel:%d%x bw:%d power:%d\n",wt_para_table[j][0],\
                                wt_para_table[j][1],wt_para_table[j][4],wt_para_table[j][2],wt_para_table[j][3]);
                            }
                            
                            j = 0;
                            break;
                        case BUSINESS_WTPARA_GETSNR_RESP:   //TODO 与上一处理类似

                            break;
                        default:
                            break;
                    }
                                    }
                                    break;
                                case NODE_SEND_HEARTBEAT:   //网关收到节点发送的心跳数据帧
                                    if(true == node_or_gateway_flag)
                                        continue;
                                    /**********  数据打印信息start **********/
                                    printf("\nRX:id:%3d type:",rcv_data_p->src_id_2);
                                    switch (rcv_data_p->frame_type) {
                                        case 0x01: printf("node_req_01"); break;
                                        case 0x02: printf("node_req_02"); break;
                                        case 0x06: printf("node_data_06"); break;
                                        case 0x5A: printf("node_heartbeat"); break;
                                        default: printf("node_data:?"); break;
                                    }
                                    if(DR_LORA_SF9==p->datarate&&p->coderate==CR_LORA_4_5)
                                        printf(" datarate:1.2Kbps");
                                    else if(DR_LORA_SF11==p->datarate&&p->coderate==CR_LORA_4_5)
                                        printf(" datarate:0.5Kbps");
                                    else if(DR_LORA_SF8==p->datarate&&p->coderate==CR_LORA_4_6)
                                        printf(" datarate:2.4Kbps");
                                    else
                                        printf("datarate:?");
                                    printf(" cnt:%4d sn:%3d freq_hz:%d if_chain:%2d rssi:%3.3f snr:%2.3f size:%3u\n"\
                                            ,++rx_frame_cnt[rcv_data_p->src_id_2],rcv_data_p->frame_sn,\
                                            gateway_get_node_frq(rcv_data_p->src_id_2),p->if_chain,p->rssi,p->snr,p->size);
                                    /**********  数据打印信息end **********/
                                    //网关每次接收到节点的心跳数据帧后为每个连接上的节点更新超时时间
                                    for(i = 0; i < 6; i++)
                                    {
                                        if(connected_terminor_id[i]==rcv_data_p->src_id_2)
                                        {
                                            connected_id_business_status[i] = 1;
                                            node_timeout[i] = cur_time + 100; // 更新超时时间
                                            //printf("HEART NODE TIME out %d\n",node_timeout[i]);
                                            break;
                                        }
                                    }
                                    break;
                                default:
                                    break;
                            }
                        }
                        else
                            printf("\nCRC_ERR_RX_id:%d\n",rcv_data_p->src_id_2);
                    }
/*************************************  节点处理部分 *************************************/
                    //网关通信帧帧头校验EB60，作为节点时收到该帧头      
                    else if((GATEWAY_FRAME_HEAD_1 == rcv_data_p->frame_head_1) && \
                            (GATEWAY_FRAME_HEAD_2 == rcv_data_p->frame_head_2))
                    {
                        crc = crc_8(p->payload, rcv_data_p->len + HEAD_LEN);
                        if(crc == p->payload[rcv_data_p->len + HEAD_LEN])
                        {
                            if(TERMINAL_MAC_ID == rcv_data_p->src_id_2)
                                continue;
                            switch (rcv_data_p->frame_type)
                            {
                            case GATEWAY_FRAME_TYPE_BRODCAST:
                                // 广播471.4M
                                if(TERMINAL_MAC_ID < rcv_data_p->src_id_2)//发现接收广播的ID号大于本机ID
                                {
                                    continue;
                                } 
                                /**********  数据打印信息start **********/
                                printf("\nRX:id:%3d type:",rcv_data_p->src_id_2);
                                switch (rcv_data_p->frame_type) {
                                    case GATEWAY_FRAME_TYPE_CONNECT:        printf("gateway_req_B1"); break;
                                    case GATEWAY_FRAME_TYPE_CONNECT_ACK:    printf("gateway_req_B2"); break;
                                    case GATEWAY_FRAME_TYPE_BRODCAST:       printf("gateway_boradcast_FF"); break;
                                    case GATEWAY_TO_GATEWAY_TYPE:           printf("gateway_boradcast_CF"); break;
                                    case GATEWAY_SEND_BUSINESS:             printf("gateway_business"); break;
                                    case GATEWAY_SEND_WT_BUSINESS:          printf("gateway_wt_business"); break;
                                    default: printf("gateway_data:?"); break;
                                }
                                if(DR_LORA_SF9==p->datarate&&p->coderate==CR_LORA_4_5)
                                    printf(" datarate:1.2Kbps");
                                else if(DR_LORA_SF11==p->datarate&&p->coderate==CR_LORA_4_5)
                                    printf(" datarate:0.5Kbps");
                                else if(DR_LORA_SF8==p->datarate&&p->coderate==CR_LORA_4_6)
                                    printf(" datarate:2.4Kbps");
                                else
                                    printf("datarate:?");
                                printf(" cnt:%4d sn:%3d freq_hz:%d if_chain:%2d rssi:%3.3f snr:%2.3f size:%3u\n"\
                                        ,++rx_frame_cnt[rcv_data_p->src_id_2],rcv_data_p->frame_sn,fb+400000,\
                                        p->if_chain,p->rssi,p->snr,p->size);
                                /**********  数据打印信息end **********/
                                //此时接收到广播的ID小于本机ID
                                if(false == node_connecting)    //若从未作为节点连接过
                                {
                                    node_connecting_time = cur_time + 10;//更新连接时间
                                    node_connecting = true; //节点已连接
                                    node_or_gateway_flag = true;    //本机身份为节点
                                    if(connected_status)    //初始状态下为节点true
                                    {
                                        printf("##########:0x%x\n",rcv_data_p->valid_data[1]);
                                        //当接收到数据帧里发现节点处于空闲状态
                                        if(0xC0==rcv_data_p->valid_data[1]) //valid_data[1]信道状态
                                        {
                                            if(TERMINAL_MAC_ID < rcv_data_p->src_id_2)//比较ID大小，若ID小
                                            {
                                                node_or_gateway_flag = false;   //赋值本机作为网关
                                                node_connecting = false;    //作为网关后复位节点连接标志位
                                                node_connecting_time = 0xFFFFFFFF; //连接时间初始化
                                                continue;   //确定网关身份后进入下一次接收
                                            } 
                                        }
                                        else if(0xFF==rcv_data_p->valid_data[1]) //发现信道已满
                                        {
                                            printf("chanel out!!!\n");
                                            //printf("start boradcast!!!\n");
                                            //net_id = rcv_data_p->valid_data[6 + 2 + 6 + 6 + 6];
                                            printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^%d\n",net_id);
                                            net_id++;   //子网编号
                                            gateway_id++;   //网关编号
                                            fb = gateway_table[gateway_id];
                                            fa = fb + 9000000; // 接收通道增加9M;
                                            set_switch_frq_rx(fa, fb); // 设置接收频点 该函数内重启1301
                                            ft = fb + 400000; // 发射增加400K
                                            restart_rx = true; // 重新启动接收
                                            continue;
                                        }
                                        printf("通道状态：0x%x\n",rcv_data_p->valid_data[1]);
                                        pthread_rwlock_wrlock(&rwlock);
                                        printf("connecting internet_01\n");
                                        tx_msg.message.rx_id = rcv_data_p->src_id_2;
                                        tx_msg.message.msg_type = NODE_NET_REQ; // 广播对应
                                        tx_msg.mtype = NODE_NET_REQ;
                                        if(msgsnd(msg_id, (void *)&tx_msg, sizeof(struct msg), 0) < 0)
                                        {
                                            printf("msg net req send failed\n");
                                        }
                                    }
                                    //connected_status标志位在接收到0xB2入网后置为false，其余时间都为true
                                    //作为节点当入网后节点与网关断开连接后，继续接收到网关的广播中包含自身ID和接入状态，
                                    //则停止向网关端发送业务并将自身连接状态初始化
                                    else //重入网
                                    {    
                                        for(i = 0; i < 6; i++)
                                        {
                                            //判断接入ID
                                            if(rcv_data_p->valid_data[i + 2 + 6 + 6]==TERMINAL_MAC_ID)
                                            {
                                                //判断接入ID状态
                                                if(rcv_data_p->valid_data[i + 2 + 6 + 6 + 6]==0)
                                                {
                                                    printf("&&&网关接收异常！！！\n");
                                                    {
                                                        connected_status = true;
                                                        sem_wait(&semaphore_all); // 停止业务
                                                    }
                                                }
                                            }
                                        }
                                    } 
                                }
 
                                break;
                            case GATEWAY_FRAME_TYPE_CONNECT://接收到网关发送来的入网请求应答0xB1
                                // 广播471.4M
                                /**********  数据打印信息start **********/
                                printf("\nRX:id:%3d type:",rcv_data_p->src_id_2);
                                switch (rcv_data_p->frame_type) {
                                    case GATEWAY_FRAME_TYPE_CONNECT:        printf("gateway_req_B1"); break;
                                    case GATEWAY_FRAME_TYPE_CONNECT_ACK:    printf("gateway_req_B2"); break;
                                    case GATEWAY_FRAME_TYPE_BRODCAST:       printf("gateway_boradcast_FF"); break;
                                    case GATEWAY_TO_GATEWAY_TYPE:           printf("gateway_boradcast_CF"); break;
                                    case GATEWAY_SEND_BUSINESS:             printf("gateway_business"); break;
                                    case GATEWAY_SEND_WT_BUSINESS:          printf("gateway_wt_business"); break;
                                    default: printf("gateway_data:?"); break;
                                }
                                if(DR_LORA_SF9==p->datarate&&p->coderate==CR_LORA_4_5)
                                    printf(" datarate:1.2Kbps");
                                else if(DR_LORA_SF11==p->datarate&&p->coderate==CR_LORA_4_5)
                                    printf(" datarate:0.5Kbps");
                                else if(DR_LORA_SF8==p->datarate&&p->coderate==CR_LORA_4_6)
                                    printf(" datarate:2.4Kbps");
                                else
                                    printf("datarate:?");
                                printf(" cnt:%4d sn:%3d freq_hz:%d if_chain:%2d rssi:%3.3f snr:%2.3f size:%3u\n"\
                                        ,++rx_frame_cnt[rcv_data_p->src_id_2],rcv_data_p->frame_sn,\
                                        fb+400000,p->if_chain,p->rssi,p->snr,p->size);
                                /**********  数据打印信息end **********/
                                //如果处于未连接状态且识别到接收的0xB1包含本机ID信息，则回复网关0x02
                                if(connected_status&&(TERMINAL_MAC_ID==rcv_data_p->des_id_2))
                                {
                                    printf("connecting internet_02\n");
                                    busniess_frq = rcv_data_p->valid_data[rcv_data_p->valid_data[0]+1]*200000 + 470000000; // 通过信道号找到对应的频点表对应位置数�?
                                    printf("\nbusniess_frq:%d\n",busniess_frq);
                                    tx_msg.message.rx_id = rcv_data_p->src_id_2;
                                    tx_msg.message.msg_type = NODE_NET_ACK_REQ; //发送入网确认，第二次握手
                                    tx_msg.mtype = NODE_NET_ACK_REQ;
                                    if(msgsnd(msg_id, (void *)&tx_msg, sizeof(struct msg), 0) < 0)
                                    {
                                        printf("msg ack send failed\n");
                                    }
                                }
                                break;
                            case GATEWAY_FRAME_TYPE_CONNECT_ACK:    //接收到网关发送来的0x02
                                // 分配的频点
                                /**********  数据打印信息start **********/
                                printf("\nRX:id:%3d type:",rcv_data_p->src_id_2);
                                switch (rcv_data_p->frame_type) {
                                    case GATEWAY_FRAME_TYPE_CONNECT:        printf("gateway_req_B1"); break;
                                    case GATEWAY_FRAME_TYPE_CONNECT_ACK:    printf("gateway_req_B2"); break;
                                    case GATEWAY_FRAME_TYPE_BRODCAST:       printf("gateway_boradcast_FF"); break;
                                    case GATEWAY_TO_GATEWAY_TYPE:           printf("gateway_boradcast_CF"); break;
                                    case GATEWAY_SEND_BUSINESS:             printf("gateway_business"); break;
                                    case GATEWAY_SEND_WT_BUSINESS:          printf("gateway_wt_business"); break;
                                    default: printf("gateway_data:?"); break;
                                }
                                if(DR_LORA_SF9==p->datarate&&p->coderate==CR_LORA_4_5)
                                    printf(" datarate:1.2Kbps");
                                else if(DR_LORA_SF11==p->datarate&&p->coderate==CR_LORA_4_5)
                                    printf(" datarate:0.5Kbps");
                                else if(DR_LORA_SF8==p->datarate&&p->coderate==CR_LORA_4_6)
                                    printf(" datarate:2.4Kbps");
                                else
                                    printf("datarate:?");
                                printf(" cnt:%4d sn:%3d freq_hz:%d if_chain:%2d rssi:%3.3f snr:%2.3f size:%3u\n"\
                                        ,++rx_frame_cnt[rcv_data_p->src_id_2],rcv_data_p->frame_sn,\
                                        fb+400000,p->if_chain,p->rssi,p->snr,p->size);
                                /**********  数据打印信息end **********/
                                //如果处于未连接状态且识别到接收的0xB2包含本机ID信息，则确认入网状态和节点连接状态
                                if(connected_status&&(TERMINAL_MAC_ID==rcv_data_p->des_id_2))
                                {
                                    printf("connected internet_ack\n");
                                    node_connecting = false;    //该标志位在入网过程中置位，入网动作完成后复位
                                    connected_status = false;   //节点已连接
                                    sem_post(&semaphore_all);  //开始发送业务
                                }
                                break;
                            case GATEWAY_SEND_BUSINESS: //节点接收到网关业务0x60，通过471.4发送
//TODO：网关发网关业务，发业务
                                /**********  数据打印信息start **********/
                                printf("\nRX:id:%3d type:",rcv_data_p->src_id_2);
                                switch (rcv_data_p->frame_type) {
                                    case GATEWAY_FRAME_TYPE_CONNECT:        printf("gateway_req_B1"); break;
                                    case GATEWAY_FRAME_TYPE_CONNECT_ACK:    printf("gateway_req_B2"); break;
                                    case GATEWAY_FRAME_TYPE_BRODCAST:       printf("gateway_boradcast_FF"); break;
                                    case GATEWAY_TO_GATEWAY_TYPE:           printf("gateway_boradcast_CF"); break;
                                    case GATEWAY_SEND_BUSINESS:             printf("gateway_business"); break;
                                    case GATEWAY_SEND_WT_BUSINESS:          printf("gateway_wt_business"); break;
                                    default: printf("gateway_data:?"); break;
                                }
                                if(DR_LORA_SF9==p->datarate&&p->coderate==CR_LORA_4_5)
                                    printf(" datarate:1.2Kbps");
                                else if(DR_LORA_SF11==p->datarate&&p->coderate==CR_LORA_4_5)
                                    printf(" datarate:0.5Kbps");
                                else if(DR_LORA_SF8==p->datarate&&p->coderate==CR_LORA_4_6)
                                    printf(" datarate:2.4Kbps");
                                else
                                    printf("datarate:?");
                                printf(" cnt:%4d sn:%3d freq_hz:%d if_chain:%2d rssi:%3.3f snr:%2.3f size:%3u\n"\
                                        ,++rx_frame_cnt[rcv_data_p->src_id_2],rcv_data_p->frame_sn,\
                                        fb+400000,p->if_chain,p->rssi,p->snr,p->size);
                                break;
                                /**********  数据打印信息end **********/
                            case GATEWAY_SEND_WT_BUSINESS: //节点接收到网关业务0x70，通过471.4发送
                                /**********  数据打印信息start **********/
                                printf("\nRX:id:%3d type:",rcv_data_p->src_id_2);
                                switch (rcv_data_p->frame_type) {
                                    case GATEWAY_FRAME_TYPE_CONNECT:        printf("gateway_req_B1"); break;
                                    case GATEWAY_FRAME_TYPE_CONNECT_ACK:    printf("gateway_req_B2"); break;
                                    case GATEWAY_FRAME_TYPE_BRODCAST:       printf("gateway_boradcast_FF"); break;
                                    case GATEWAY_TO_GATEWAY_TYPE:           printf("gateway_boradcast_CF"); break;
                                    case GATEWAY_SEND_BUSINESS:             printf("gateway_business"); break;
                                    case GATEWAY_SEND_WT_BUSINESS:          printf("gateway_wt_business"); break;
                                    default: printf("gateway_data:?"); break;
                                }
                                if(DR_LORA_SF9==p->datarate&&p->coderate==CR_LORA_4_5)
                                    printf(" datarate:1.2Kbps");
                                else if(DR_LORA_SF11==p->datarate&&p->coderate==CR_LORA_4_5)
                                    printf(" datarate:0.5Kbps");
                                else if(DR_LORA_SF8==p->datarate&&p->coderate==CR_LORA_4_6)
                                    printf(" datarate:2.4Kbps");
                                else
                                    printf("datarate:?");
                                printf(" cnt:%4d sn:%3d freq_hz:%d if_chain:%2d rssi:%3.3f snr:%2.3f size:%3u\n"\
                                        ,++rx_frame_cnt[rcv_data_p->src_id_2],rcv_data_p->frame_sn,\
                                        fb+400000,p->if_chain,p->rssi,p->snr,p->size);
                                /**********  数据打印信息end **********/
                    /********** 节点解析节点发送的业务数据内容 **********/
                    //节点收到网关发送的高速组网业务参数请求，并将参数更新到本地全局（）
                    wt_busi_num = rcv_data_p->valid_data[2] - wt_params.busi_num;
                    if(wt_busi_num > 200)   //判断是否进位溢出
                    {
                        wt_busi_num = wt_params.busi_num + 255;
                    }
                    else if(wt_busi_num < -200)
                    {
                        wt_busi_num = rcv_data_p->valid_data[2] + 255;
                    }
                    else
                    {
                        wt_busi_num = rcv_data_p->valid_data[2];
                    }
                    if(wt_params.busi_num < wt_busi_num)
                    {
                        wt_params.busi_num = rcv_data_p->valid_data[2];
                        wt_busi_broadcast++;
                    }
                    switch (rcv_data_p->valid_data[3])
                    {
                        case BUSINESS_WTPARA_SETREQ:
                            if(wt_busi_broadcast > 0)
                            {
                                wt_meshpara_change_flag = true; //获取参数设置命令，此时关闭查询本机
                                switch (rcv_data_p->valid_data[4])  //根据id号接收相应数据参数并判断是否需要修改参数
                                {
                                    case 1:
                                        if((rcv_data_p->valid_data[5] == 0xa1) && \
                                        (wt_params.channel_num != rcv_data_p->valid_data[6]))
                                        {
                                            wt_params.channel_num = rcv_data_p->valid_data[6];
                                            wt_params.channel_num_add = rcv_data_p->valid_data[11];
                                        }
                                        if((rcv_data_p->valid_data[7] == 0xa2) && \
                                        (wt_params.bw_value != rcv_data_p->valid_data[8]))
                                        {
                                            wt_params.bw_value = rcv_data_p->valid_data[8];
                                        }
                                        if((rcv_data_p->valid_data[9] == 0xa3) && \
                                        (wt_params.txpower_value != rcv_data_p->valid_data[10]))
                                        {
                                            wt_params.txpower_value = rcv_data_p->valid_data[10];
                                        }
                                        break;
                                    case 2:
                                        if((rcv_data_p->valid_data[5] == 0xb1) && \
                                        (wt_params.channel_num != rcv_data_p->valid_data[6]))
                                        {
                                            wt_params.channel_num = rcv_data_p->valid_data[6];
                                            wt_params.channel_num_add = rcv_data_p->valid_data[11];
                                        }
                                        if((rcv_data_p->valid_data[7] == 0xb2) && \
                                        (wt_params.bw_value != rcv_data_p->valid_data[8]))
                                        {
                                            wt_params.bw_value = rcv_data_p->valid_data[8];
                                        }
                                        if((rcv_data_p->valid_data[9] == 0xb3) && \
                                        (wt_params.txpower_value != rcv_data_p->valid_data[10]))
                                        {
                                            wt_params.txpower_value = rcv_data_p->valid_data[10];
                                        }
                                        break;
                                    case 3:
                                        if((rcv_data_p->valid_data[5] == 0xc1) && \
                                        (wt_params.channel_num != rcv_data_p->valid_data[6]))
                                        {
                                            wt_params.channel_num = rcv_data_p->valid_data[6];
                                            wt_params.channel_num_add = rcv_data_p->valid_data[11];
                                        }
                                        if((rcv_data_p->valid_data[7] == 0xc2) && \
                                        (wt_params.bw_value != rcv_data_p->valid_data[8]))
                                        {
                                            wt_params.bw_value = rcv_data_p->valid_data[8];
                                        }
                                        if((rcv_data_p->valid_data[9] == 0xc3) && \
                                        (wt_params.txpower_value != rcv_data_p->valid_data[10]))
                                        {
                                            wt_params.txpower_value = rcv_data_p->valid_data[10];
                                        }
                                        break;
                                    case 4:
                                        if((rcv_data_p->valid_data[5] == 0xd1) && \
                                        (wt_params.channel_num != rcv_data_p->valid_data[6]))
                                        {
                                            wt_params.channel_num = rcv_data_p->valid_data[6];
                                            wt_params.channel_num_add = rcv_data_p->valid_data[11];
                                        }
                                        if((rcv_data_p->valid_data[7] == 0xd2) && \
                                        (wt_params.bw_value != rcv_data_p->valid_data[8]))
                                        {
                                            wt_params.bw_value = rcv_data_p->valid_data[8];
                                        }
                                        if((rcv_data_p->valid_data[9] == 0xd3) && \
                                        (wt_params.txpower_value != rcv_data_p->valid_data[10]))
                                        {
                                            wt_params.txpower_value = rcv_data_p->valid_data[10];
                                        }
                                        break;
                                    case 5:
                                        if((rcv_data_p->valid_data[5] == 0xe1) && \
                                        (wt_params.channel_num != rcv_data_p->valid_data[6]))
                                        {
                                            wt_params.channel_num = rcv_data_p->valid_data[6];
                                            wt_params.channel_num_add = rcv_data_p->valid_data[11];
                                        }
                                        if((rcv_data_p->valid_data[7] == 0xe2) && \
                                        (wt_params.bw_value != rcv_data_p->valid_data[8]))
                                        {
                                            wt_params.bw_value = rcv_data_p->valid_data[8];
                                        }
                                        if((rcv_data_p->valid_data[9] == 0xe3) && \
                                        (wt_params.txpower_value != rcv_data_p->valid_data[10]))
                                        {
                                            wt_params.txpower_value = rcv_data_p->valid_data[10];
                                        }
                                        break;
                                    case 6:
                                        if((rcv_data_p->valid_data[5] == 0xf1) && \
                                        (wt_params.channel_num != rcv_data_p->valid_data[6]))
                                        {
                                            wt_params.channel_num = rcv_data_p->valid_data[6];
                                            wt_params.channel_num_add = rcv_data_p->valid_data[11];
                                        }
                                        if((rcv_data_p->valid_data[7] == 0xf2) && \
                                        (wt_params.bw_value != rcv_data_p->valid_data[8]))
                                        {
                                            wt_params.bw_value = rcv_data_p->valid_data[8];
                                        }
                                        if((rcv_data_p->valid_data[9] == 0xf3) && \
                                        (wt_params.txpower_value != rcv_data_p->valid_data[10]))
                                        {
                                            wt_params.txpower_value = rcv_data_p->valid_data[10];
                                        }
                                        break;
                                    default:
                                        break;
                                }
                                if(rcv_data_p->valid_data[11] == 0)
                                {
                                    printf("wt_id:%d wt_channel:%d wt_bw:%d wt_power:%d\n",\
                                    rcv_data_p->valid_data[4],wt_params.channel_num,\
                                    wt_params.bw_value,wt_params.txpower_value);
                                }
                                else
                                {
                                    printf("wt_id:%d wt_channel:%d%x wt_bw:%d wt_power:%d\n",\
                                    rcv_data_p->valid_data[4],wt_params.channel_num,\
                                    wt_params.channel_num_add,wt_params.bw_value,wt_params.txpower_value);
                                }
                                
                                wt_params.source_node = rcv_data_p->src_id_2;
                                wt_params.busi_type = BUSINESS_WTPARA_SETRESP;
                            }
                            break;
                        case BUSINESS_WTPARA_GETREQ: //节点收到网关查询业务，将最近一次获取的参数在send_ack线程中返回给节点
                            //收到0x70，本机设置成功后，返回0x07，返回内容中包含业务应答
                            wt_params.busi_type = BUSINESS_WTPARA_GETRESP;
                            tx_msg.message.rx_id = rcv_data_p->src_id_2;
                            tx_msg.message.msg_type = NODE_SEND_WT_BUSINESS;
                            tx_msg.mtype = NODE_SEND_WT_BUSINESS;
                            if(msgsnd(msg_id, (void *)&tx_msg, sizeof(struct msg), 0) < 0)
                            {
                                printf("msg ack send failed\n");
                            }
                            break;
                        case BUSINESS_WTPARA_GETSNR:
                            
                            wt_params.busi_type = BUSINESS_WTPARA_GETSNR_RESP;
                            break;
                        //节点收到网关发送的高速组网业务参数应答，并将参数更新到本地全局（）
                        case BUSINESS_WTPARA_SETRESP:   //收到设置参数的应答，则复位发送标志位，结束流程
                            if(TERMINAL_MAC_ID == rcv_data_p->des_id_2) //节点收到网关发送给本机的业务
                            {
                                wt_lora_timecnt_flag = false;
                                wt_para_send_flag = false;
                                wt_lora_timeout_flag = false;
                            }
                            break;
                        case BUSINESS_WTPARA_GETRESP:   //节点收到网关响应参数，打印并保存到本地列表
                            if(TERMINAL_MAC_ID == rcv_data_p->des_id_2) //节点收到网关发送给本机的业务
                            {
                                if(rcv_data_p->len >= 15)
                                {
                                    wt_len = (rcv_data_p->len - 7) / 8; //计算节点个数
                                }
                                for(i=0;i<wt_len;i++)
                                {
                                    for(j=0;j<6;j++)
                                    {
                                        if(rcv_data_p->valid_data[4+8*i] == j+1)
                                        {
                                            switch (rcv_data_p->valid_data[4+8*i])
                                            {
                                                case 1:
                                                    wt_para_table[i][0] = rcv_data_p->valid_data[4+8*i];
                                                    if(rcv_data_p->valid_data[5+8*i] == 0xa1)
                                                    {
                                                        wt_para_table[i][1] = rcv_data_p->valid_data[6+8*i];
                                                        wt_para_table[i][4] = rcv_data_p->valid_data[11+8*i];
                                                    }
                                                    if(rcv_data_p->valid_data[7+8*i] == 0xa2)
                                                    {
                                                        wt_para_table[i][2] = rcv_data_p->valid_data[8+8*i];
                                                    }
                                                    if(rcv_data_p->valid_data[9+8*i] == 0xa3)
                                                    {
                                                        wt_para_table[i][3] = rcv_data_p->valid_data[10+8*i];
                                                    }
                                                    break;
                                                case 2:
                                                    wt_para_table[i][0] = rcv_data_p->valid_data[4+8*i];
                                                    if(rcv_data_p->valid_data[5+8*i] == 0xb1)
                                                    {
                                                        wt_para_table[i][1] = rcv_data_p->valid_data[6+8*i];
                                                        wt_para_table[i][4] = rcv_data_p->valid_data[11+8*i];
                                                    }
                                                    if(rcv_data_p->valid_data[7+8*i] == 0xb2)
                                                    {
                                                        wt_para_table[i][2] = rcv_data_p->valid_data[8+8*i];
                                                    }
                                                    if(rcv_data_p->valid_data[9+8*i] == 0xb3)
                                                    {
                                                        wt_para_table[i][3] = rcv_data_p->valid_data[10+8*i];
                                                    }
                                                    break;
                                                case 3:
                                                    wt_para_table[i][0] = rcv_data_p->valid_data[4+8*i];
                                                    if(rcv_data_p->valid_data[5+8*i] == 0xc1)
                                                    {
                                                        wt_para_table[i][1] = rcv_data_p->valid_data[6+8*i];
                                                        wt_para_table[i][4] = rcv_data_p->valid_data[11+8*i];
                                                    }
                                                    if(rcv_data_p->valid_data[7+8*i] == 0xc2)
                                                    {
                                                        wt_para_table[i][2] = rcv_data_p->valid_data[8+8*i];
                                                    }
                                                    if(rcv_data_p->valid_data[9+8*i] == 0xc3)
                                                    {
                                                        wt_para_table[i][3] = rcv_data_p->valid_data[10+8*i];
                                                    }
                                                    break;
                                                case 4:
                                                    wt_para_table[i][0] = rcv_data_p->valid_data[4+8*i];
                                                    if(rcv_data_p->valid_data[5+8*i] == 0xd1)
                                                    {
                                                        wt_para_table[i][1] = rcv_data_p->valid_data[6+8*i];
                                                        wt_para_table[i][4] = rcv_data_p->valid_data[11+8*i];
                                                    }
                                                    if(rcv_data_p->valid_data[7+8*i] == 0xd2)
                                                    {
                                                        wt_para_table[i][2] = rcv_data_p->valid_data[8+8*i];
                                                    }
                                                    if(rcv_data_p->valid_data[9+8*i] == 0xd3)
                                                    {
                                                        wt_para_table[i][3] = rcv_data_p->valid_data[10+8*i];
                                                    }
                                                    break;
                                                case 5:
                                                    wt_para_table[i][0] = rcv_data_p->valid_data[4+8*i];
                                                    if(rcv_data_p->valid_data[5+8*i] == 0xe1)
                                                    {
                                                        wt_para_table[i][1] = rcv_data_p->valid_data[6+8*i];
                                                        wt_para_table[i][4] = rcv_data_p->valid_data[11+8*i];
                                                    }
                                                    if(rcv_data_p->valid_data[7+8*i] == 0xe2)
                                                    {
                                                        wt_para_table[i][2] = rcv_data_p->valid_data[8+8*i];
                                                    }
                                                    if(rcv_data_p->valid_data[9+8*i] == 0xe3)
                                                    {
                                                        wt_para_table[i][3] = rcv_data_p->valid_data[10+8*i];
                                                    }
                                                    break;
                                                case 6:
                                                    wt_para_table[i][0] = rcv_data_p->valid_data[4+8*i];
                                                    if(rcv_data_p->valid_data[5+8*i] == 0xf1)
                                                    {
                                                        wt_para_table[i][1] = rcv_data_p->valid_data[6+8*i];
                                                        wt_para_table[i][4] = rcv_data_p->valid_data[11+8*i];
                                                    }
                                                    if(rcv_data_p->valid_data[7+8*i] == 0xf2)
                                                    {
                                                        wt_para_table[i][2] = rcv_data_p->valid_data[8+8*i];
                                                    }
                                                    if(rcv_data_p->valid_data[9+8*i] == 0xf3)
                                                    {
                                                        wt_para_table[i][3] = rcv_data_p->valid_data[10+8*i];
                                                    }
                                                    break;
                                                default:
                                                    break;
                                            }
                                            if(wt_para_table[i][4] == 0)
                                            {
                                                printf("\nwt_id_%d channel:%d bw:%d power:%d\n",wt_para_table[i][0],\
                                                wt_para_table[i][1],wt_para_table[i][2],wt_para_table[i][3]);
                                            }
                                            else
                                            {
                                                printf("\nwt_id_%d channel:%d%x bw:%d power:%d\n",wt_para_table[i][0],\
                                                wt_para_table[i][1],wt_para_table[i][4],wt_para_table[i][2],wt_para_table[i][3]);
                                            }
                                        }
                                    }
                                }
                            }
                            break;
                        case BUSINESS_WTPARA_GETSNR_RESP:   //TODO 与上一处理类似
                            if(TERMINAL_MAC_ID == rcv_data_p->des_id_2) //节点收到网关发送给本机的业务
                            {

                            }
                            break;
                        default:
                            break;
                    }
                                break;
                            case GATEWAY_TO_GATEWAY_TYPE: //TODO:帧内容中需要增加自身接收频点，待做
                                if(node_or_gateway_flag==false) // 过滤节点，仅网关接收另一网关参数
                                {
                                    // 长通知470.4M
                                    /**********  数据打印信息start **********/
                                    printf("\nRX:id:%3d type:",rcv_data_p->src_id_2);
                                    switch (rcv_data_p->frame_type) {
                                        case GATEWAY_FRAME_TYPE_CONNECT:        printf("gateway_req_B1"); break;
                                        case GATEWAY_FRAME_TYPE_CONNECT_ACK:    printf("gateway_req_B2"); break;
                                        case GATEWAY_FRAME_TYPE_BRODCAST:       printf("gateway_boradcast_FF"); break;
                                        case GATEWAY_TO_GATEWAY_TYPE:           printf("gateway_boradcast_CF"); break;
                                        case GATEWAY_SEND_BUSINESS:             printf("gateway_business"); break;
                                        case GATEWAY_SEND_WT_BUSINESS:          printf("gateway_wt_business"); break;
                                        default: printf("gateway_data:?"); break;
                                    }
                                    if(DR_LORA_SF9==p->datarate&&p->coderate==CR_LORA_4_5)
                                        printf(" datarate:1.2Kbps");
                                    else if(DR_LORA_SF11==p->datarate&&p->coderate==CR_LORA_4_5)
                                        printf(" datarate:0.5Kbps");
                                    else if(DR_LORA_SF8==p->datarate&&p->coderate==CR_LORA_4_6)
                                        printf(" datarate:2.4Kbps");
                                    else
                                        printf("datarate:?");
                                    printf(" cnt:%4d sn:%3d freq_hz:%d if_chain:%2d rssi:%3.3f snr:%2.3f size:%3u\n"\
                                            ,++rx_frame_cnt[rcv_data_p->src_id_2],rcv_data_p->frame_sn,fb-400000,\
                                            p->if_chain,p->rssi,p->snr,p->size);
                                    /**********  数据打印信息end **********/
                                
                                    if(net_id < rcv_data_p->valid_data[6 + 2 + 6 + 6 + 6])
                                    {
                                        net_id = rcv_data_p->valid_data[6 + 2 + 6 + 6 + 6];
                                    }

                                    /* 保存ID */
                                    temp = rcv_data_p->valid_data[8]*200000 + 470000000; // 计算真实频点
                                    for(i = 0; i < GATEWAY_LEN; i++)    //读取接收到相邻子网的接入ID和频点参数
                                    {
                                        if(temp==gateway_table[i])
                                        {
                                            printf("FRQ:%d\n",temp); 
                                            gateway_id_table[i].gateway_frq = temp;
                                            for(j = 0; j < 6; j++)
                                            {
                                                gateway_id_table[i].id[j] = rcv_data_p->valid_data[2+j];
                                                printf("ID:%d\n",rcv_data_p->valid_data[2+j]);
                                            }
                                            break;
                                        }
                                    }
                                }
                                break;
                            default:
                                break;
                            }
                        }
                    }
                    else
                    {
                        printf("Frame head err\n");
                    }
                }
                else
                {
                    printf("CRC_ERROR\n");
                }
            }
        }
    }
}

/*
*   brief:  根据id判断并设置业务帧类别
*   retval: None
*   params: buf 参数类型
            dev_id id号
*/
static void wt_idtype_set(uint8_t *buf,uint8_t dev_id)
{
    int i = 0;
    switch (dev_id)
    {
        case 1:
            for(i=0;i<3;i++)
            {
                buf[i] = 0xa0 | (i+1);
            }
            break;
        case 2:
            for(i=0;i<3;i++)
            {
                buf[i] = 0xb0 | (i+1);
            }
            break;
        case 3:
            for(i=0;i<3;i++)
            {
                buf[i] = 0xc0 | (i+1);
            }
            break;
        case 4:
            for(i=0;i<3;i++)
            {
                buf[i] = 0xd0 | (i+1);
            }
            break;
        case 5:
            for(i=0;i<3;i++)
            {
                buf[i] = 0xe0 | (i+1);
            }
            break;
        case 6:
            for(i=0;i<3;i++)
            {
                buf[i] = 0xf0 | (i+1);
            }
            break;
        default:
            break;
    }
}

/*
*   brief:  更新高速设备的参数状态，发生变化则更改自身高速设备组网参数
            (发生在启动和接收到对方的参数后，wt_params结构体中值变化)
*   retval: true success
            false failed
*   params: None
*/
static bool wt_input_refresh(void)
{
    static uint8_t input[4] = {0};
    bool wt_sta = false;

    if((input[0] == 0) && (input[1] == 0) && (input[2] == 0)) //初始化后第一次设置自身参数，已在主线程中实现
    {
        wt_sta = false;
    }
    else  //当设备在运行，接收到外部更改命令时，三变量值不为0,任何一个变量值发生变化则标志位置位
    {
        if((input[0] == wt_params.channel_num) && (input[1] == wt_params.bw_value) && \
        (input[2] == wt_params.txpower_value) && (input[3] == wt_params.channel_num_add))
        {
            wt_sta = false;
        }
        else
        {
            wt_sta = true;
        }
    }
    input[0] = wt_params.channel_num;
    input[1] = wt_params.bw_value;
    input[2] = wt_params.txpower_value;
    input[3] = wt_params.channel_num_add;

    return wt_sta;
}

static uint32_t gateway_get_node_frq(uint8_t id)
{
    uint32_t i;

    for( i = 0; i < 6; i++)
    {
        if(id == context_id_table[i][0])
            return 470000000 + context_id_table[i][1] * 200000;
    }
    printf("\n!!!id:%d isn't in gateway\n",id);
    return 0;
}

static void time_out_func(union sigval v)
{
    uint32_t i,j,k;
    struct tx_config_msgbuf tx_msg;
    int32_t msg_id;
    uint32_t time = 0;

    cur_time++;
    frq_cyc++;
    for(i = 0; i < MAX_TIMEOUT_LEN; i++)
    {
        if(time_table[i][1] <= cur_time && 0 != time_table[i][1])
        {
            time_table[i][1] = 0; // 清空数据
            time_table[i][0] = 0; // 清空数据
            set_chanel_status(time_table[i][2], 0);
            context_id_table[time_table[i][2] - 1][0] = 0;
            for(j = 0; j < 6; j++)
            {
                printf("ID:%3d,Freq:%d\n", context_id_table[j][0], 470000000 + context_id_table[j][1] * 200000);
            }
        }
    }
    
    for(i = 0; i < 6; i++)
    {
        if(node_timeout[i] <= cur_time) // 查询连接通道超时
        {
            connected_id_business_status[i] = 0;
            //匹配频点表中ID
            for(j = 0; j < 6; j++)
            {
                if((context_id_table[j][0]!=connected_terminor_id[i]) && \
                   (connected_terminor_id[i]!=0))
                {
                    set_chanel_status(j+1, 0);
                    context_id_table[j][0] = 0;
                    printf("status:0x%x\n");
                    for(k = 0; k < 6; k++)
                    {
                        printf("ID:%3d,Freq:%d\n", context_id_table[k][0], 470000000 + context_id_table[k][1] * 200000);
                    }
                }
            }
            connected_terminor_id[i] = 0;
            node_timeout[i] = 0xffffffff;
        }
    }

    if(node_connecting_time <= cur_time)
    {
        node_connecting = false;
        node_connecting_time =0xFFFFFFFF;
    }

    if(wt_lora_timecnt_flag)
    {
        if(SelfAddJudge(cur_time,wt_business_timeout,40,&time)) //40s超时等待时间
        {
            wt_lora_timeout_flag = true;
            printf("wt_timeout=%d\n",time);
        }
    }
}

static void set_chanel_status(uint8_t chanel, uint8_t flag)
{
    uint8_t num = chanel - 1;

    if(num <= 5)
    {
        if(flag)
            chanel_status = chanel_status | (0x01<<num);
        else
            chanel_status = chanel_status & ((0xFF)^(0x01<<num));
    }
}

static uint8_t get_chanel_status(void)
{
    uint8_t i = 0;
    uint8_t temp = chanel_status;

    while(1==((temp>>i)&0x01))
    {
        i++;
        if(i >= 6)
        {
            return 0xff;
        }
    }
    return i + 1;
}

static void set_rx_chanel_config(uint32_t rx_fa, uint32_t rx_fb , enum lgw_radio_type_e rx_radio_type)
{
    /* set configuration for RF chains */
    memset(&rfconf, 0, sizeof(rfconf));

    rfconf.enable = true;
    rfconf.freq_hz = rx_fa;
    rfconf.rssi_offset = DEFAULT_RSSI_OFFSET;
    rfconf.type = rx_radio_type;
    rfconf.tx_enable = true;
    rfconf.tx_notch_freq = DEFAULT_NOTCH_FREQ;
    lgw_rxrf_setconf(0, rfconf); /* radio A, f0 */

    rfconf.enable = true;
    rfconf.freq_hz = rx_fb;
    rfconf.rssi_offset = DEFAULT_RSSI_OFFSET;
    rfconf.type = rx_radio_type;
    rfconf.tx_enable = false;
    lgw_rxrf_setconf(1, rfconf); /* radio B, f1 */

    /* set configuration for LoRa multi-SF channels (bandwidth cannot be set) */
    memset(&ifconf, 0, sizeof(ifconf));

    ifconf.enable = true;
    ifconf.rf_chain = 1;
    ifconf.freq_hz = -400000;
    ifconf.datarate = DR_LORA_MULTI;
    lgw_rxif_setconf(0, ifconf); /* chain 0: LoRa 125kHz, all SF, on f1 - 0.4 MHz */
    //rx_freq_table[7] = (rx_fb - 470000000 - 600000) / 200000;

    ifconf.enable = true;
    ifconf.rf_chain = 1;
    ifconf.freq_hz = 400000;
    ifconf.datarate = DR_LORA_MULTI;
    lgw_rxif_setconf(1, ifconf); /* chain 1: LoRa 125kHz, all SF, on f1 - 0.2 MHz */
    //rx_freq_table[6] = (rx_fb - 470000000 + 600000) / 200000;

    ifconf.enable = true;
    ifconf.rf_chain = 0;
    ifconf.freq_hz = -600000;
    ifconf.datarate = DR_LORA_MULTI;
    lgw_rxif_setconf(2, ifconf); /* chain 2: LoRa 125kHz, all SF, on f1 - 0.0 MHz */
    rx_freq_table[2] = (rx_fa - 470000000 - 600000) / 200000;

    ifconf.enable = true;
    ifconf.rf_chain = 0;
    ifconf.freq_hz = -400000;
    ifconf.datarate = DR_LORA_MULTI;
    lgw_rxif_setconf(3, ifconf); /* chain 3: LoRa 125kHz, all SF, on f0 - 0.4 MHz */
    rx_freq_table[3] = (rx_fa - 470000000 - 400000) / 200000;

    ifconf.enable = true;
    ifconf.rf_chain = 0;
    ifconf.freq_hz = -200000;
    ifconf.datarate = DR_LORA_MULTI;
    lgw_rxif_setconf(4, ifconf); /* chain 4: LoRa 125kHz, all SF, on f0 - 0.2 MHz */
    rx_freq_table[4] = (rx_fa - 470000000 - 200000) / 200000;

    ifconf.enable = true;
    ifconf.rf_chain = 0;
    ifconf.freq_hz = 0;
    ifconf.datarate = DR_LORA_MULTI;
    lgw_rxif_setconf(5, ifconf); /* chain 5: LoRa 125kHz, all SF, on f0 + 0.0 MHz */
    rx_freq_table[5] = (rx_fa - 470000000) / 200000;

    ifconf.enable = true;
    ifconf.rf_chain = 0;
    ifconf.freq_hz = 200000;
    ifconf.datarate = DR_LORA_MULTI;
    lgw_rxif_setconf(6, ifconf); /* chain 6: LoRa 125kHz, all SF, on f0 + 0.2 MHz */
    rx_freq_table[1] = (rx_fa - 470000000 + 200000) / 200000;

    ifconf.enable = true;
    ifconf.rf_chain = 0;
    ifconf.freq_hz = 400000;
    ifconf.datarate = DR_LORA_MULTI;
    lgw_rxif_setconf(7, ifconf); /* chain 7: LoRa 125kHz, all SF, on f0 + 0.4 MHz */
    rx_freq_table[0] = (rx_fa - 470000000 + 400000) / 200000;

    /* set configuration for LoRa 'stand alone' channel */
    memset(&ifconf, 0, sizeof(ifconf));
    ifconf.enable = false;
    ifconf.rf_chain = 0;
    ifconf.freq_hz = 0;
    ifconf.bandwidth = BW_250KHZ;
    ifconf.datarate = DR_LORA_SF10;
    lgw_rxif_setconf(8, ifconf); /* chain 8: LoRa 250kHz, SF10, on f0 MHz */

    /* set configuration for FSK channel */
    memset(&ifconf, 0, sizeof(ifconf));
    ifconf.enable = false;
    ifconf.rf_chain = 1;
    ifconf.freq_hz = 0;
    ifconf.bandwidth = BW_250KHZ;
    ifconf.datarate = 64000;
    lgw_rxif_setconf(9, ifconf); /* chain 9: FSK 64kbps, on f1 MHz */
  
}

static void set_txpkt_config(uint32_t send_ft, int32_t send_xp, int32_t send_xq, uint8_t cr_lora)
{
    printf("ft:%d, xp:%d,xq:%d, cal:%d\n", send_ft, send_xp, send_xq, cr_lora);
    memset(&txpkt, 0, sizeof(txpkt));
    txpkt.freq_hz = send_ft;
    txpkt.tx_mode = IMMEDIATE;
    txpkt.rf_power = send_xp;
    txpkt.modulation = MOD_LORA;
    txpkt.bandwidth = BW_125KHZ;
    txpkt.datarate = send_xq;
    txpkt.coderate = cr_lora;
    txpkt.preamble = 8;
    txpkt.rf_chain = 0;
    memset(&broadcast_txpkt, 0, sizeof(broadcast_txpkt));
    broadcast_txpkt.freq_hz = send_ft;
    broadcast_txpkt.tx_mode = IMMEDIATE;
    broadcast_txpkt.rf_power = send_xp;
    broadcast_txpkt.modulation = MOD_LORA;
    broadcast_txpkt.bandwidth = BW_125KHZ;
    broadcast_txpkt.datarate = send_xq;
    broadcast_txpkt.coderate = cr_lora;
    broadcast_txpkt.preamble = 8;
    broadcast_txpkt.rf_chain = 0;
}

static void set_switch_frq_rx(uint32_t fa_frq, uint32_t fb_frq)
{
    uint32_t i;

    sem_wait(&semaphore_rx);
    lgw_stop();
    usleep(1);
    boardconf.lorawan_public = false;
    boardconf.clksrc = 1;
    lgw_board_setconf(boardconf);
    set_rx_chanel_config(fa_frq, fb_frq , LGW_RADIO_TYPE_SX1255);
    lgw_start();
    FILE * reg_dump = NULL;
    reg_dump = fopen("reg_dump.log", "w");
    if (reg_dump != NULL) {
        lgw_reg_check(reg_dump);
        fclose(reg_dump);
    }
    for ( i = 0; i < 6; i++)
    {
        context_id_table[i][1] = rx_freq_table[i];
    }
    sem_post(&semaphore_rx);
    
}


// /** 信号�? **/
// static int32_t set_semvalue()
// {
// 	//用于初始化信号量，在使用信号量前必须这样�?
// 	union semun sem_union;
 
// 	sem_union.val = 1;
// 	if(semctl(sem_id, 0, SETVAL, sem_union) == -1)
// 		return 0;
// 	return 1;
// }
 
// static void del_semvalue()
// {
// 	//删除信号�?
// 	union semun sem_union;
 
// 	if(semctl(sem_id, 0, IPC_RMID, sem_union) == -1)
// 		printf("Failed to delete semaphore\n");
// }
 
// static int32_t semaphore_p()
// {
// 	//对信号量做减1操作，即等待P（sv�?
// 	struct sembuf sem_b;
// 	sem_b.sem_num = 0;
// 	sem_b.sem_op = -1;//P()
// 	sem_b.sem_flg = SEM_UNDO;
// 	if(semop(sem_id, &sem_b, 1) == -1)
// 	{
// 		printf("semaphore_p failed\n");
// 		return 0;
// 	}
// 	return 1;
// }
 
// static int32_t semaphore_v()
// {
// 	//这是一个释放操作，它使信号量变为可用，即发送信号V（sv�?
// 	struct sembuf sem_b;
// 	sem_b.sem_num = 0;
// 	sem_b.sem_op = 1;//V()
// 	sem_b.sem_flg = SEM_UNDO;
// 	if(semop(sem_id, &sem_b, 1) == -1)
// 	{
// 		printf("semaphore_v failed\n");
// 		return 0;
// 	}
// 	return 1;
// }





















