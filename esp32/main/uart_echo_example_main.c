
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "string.h"

#define UART_0_TX  (GPIO_NUM_1)
#define UART_0_RX  (GPIO_NUM_3)
#define UART_1_TX  (GPIO_NUM_5)
#define UART_1_RX  (GPIO_NUM_4)
#define UART_2_TX  (GPIO_NUM_27)
#define UART_2_RX  (GPIO_NUM_26)
#define UART_RTS  (UART_PIN_NO_CHANGE)
#define UART_CTS  (UART_PIN_NO_CHANGE)
#define SIM800_RST  (GPIO_NUM_18)

#define BUF_SIZE (1024)

const char* COMMAND_AT = "AT\r\n";
const char* COMMAND_ATE = "ATE1\r\n";
const char* COMMAND_CGATT = "AT+CGATT?\r\n";
const char* COMMAND_CSTT = "AT+CSTT=\"m-wap\",\"mms\",\"mms\"\r\n";
const char* COMMAND_CIICR = "AT+CIICR\r\n";
const char* COMMAND_CIFSR = "AT+CIFSR\r\n";
//const char* COMMAND_CIPSTART = "AT+CIPSTART=\"TCP\",\"113.190.237.43\",\"30000\"\r\n";
const char* COMMAND_CIPSTART = "AT+CIPSTART=\"TCP\",\"tcp://0.tcp.ap.ngrok.io\",\"10848\"\r\n";
const char* COMMAND_CIPSEND = "AT+CIPSEND\r\n";
const char* COMMAND_CIPCLOSE = "AT+CIPCLOSE\r\n";

unsigned char buf[1024];

static void uart_config()
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_0_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    uart_config_t uart_1_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_1_config);
    uart_set_pin(UART_NUM_1, UART_1_TX, UART_1_RX, UART_RTS, UART_CTS);

    uart_driver_install(UART_NUM_0, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_0_config);
    uart_set_pin(UART_NUM_0, UART_0_TX, UART_0_RX, UART_RTS, UART_CTS);

    uart_driver_install(UART_NUM_2, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_2, &uart_0_config);
    uart_set_pin(UART_NUM_2, UART_2_TX, UART_2_RX, UART_RTS, UART_CTS);
}

static void send_AT_command(uart_port_t uart_num_transmit, uart_port_t uart_num_received, const char* command)
{
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    uart_write_bytes(uart_num_transmit, command, strlen(command));
    vTaskDelay(1000 / portTICK_RATE_MS);
    int len = uart_read_bytes(uart_num_transmit, data, BUF_SIZE, 20 / portTICK_RATE_MS);
    uart_write_bytes(uart_num_received, (const char *) data, len);  
}

static void sim800_deinit(){
    gpio_pad_select_gpio(SIM800_RST);
    gpio_set_direction(SIM800_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(SIM800_RST, 0);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    gpio_set_level(SIM800_RST, 1);
    vTaskDelay(15000 / portTICK_PERIOD_MS);
}

static void sim800_init(){
    sim800_deinit();
    send_AT_command(UART_NUM_1, UART_NUM_0, COMMAND_AT);
    send_AT_command(UART_NUM_1, UART_NUM_0, COMMAND_ATE);
    send_AT_command(UART_NUM_1, UART_NUM_0, COMMAND_CGATT);
    send_AT_command(UART_NUM_1, UART_NUM_0, COMMAND_CSTT);
    send_AT_command(UART_NUM_1, UART_NUM_0, COMMAND_CIICR);
    send_AT_command(UART_NUM_1, UART_NUM_0, COMMAND_CIFSR);
    send_AT_command(UART_NUM_1, UART_NUM_0, COMMAND_CIPSTART);
    vTaskDelay(2000 / portTICK_RATE_MS);
}

static void send_GPS_coordination(unsigned char gll[]){
    send_AT_command(UART_NUM_1, UART_NUM_0, COMMAND_CIPSEND);
    uart_write_bytes(UART_NUM_1, (const char *) gll, strlen( (const char *) gll));
    uart_write_bytes(UART_NUM_1, "\x1a\r\n", 3);
}


static void gps_coordination_task(void *arg)
{
    while (1) {
        // Read data from the UART0
        int len = uart_read_bytes(UART_NUM_2, buf, BUF_SIZE, 20 / portTICK_RATE_MS);
        unsigned char gll[100];
        unsigned char gps_cmd[6];

        for(int i = 0; i < strlen((const char *)buf); i++)
        {
            if(buf[i] == '$'){
                for(int j = 0; j < 6; j++){
                    gps_cmd[j] = buf[i+j];
                }

                if(!strcmp((const char*) gps_cmd, "$GPGLL")){
                    int count = i;
                    for(int j = 0; j < 100; j++){
                        gll[j] = buf[count];
                        if(buf[count] == '\n' || buf[count] == '\r'){
                            gll[j] = '\n';
                            gll[j+1] = '\0';
                            break;
                        }
                        count++;
                    }
                }
            }
        }

        //Write data back to the UART0
        uart_write_bytes(UART_NUM_0, (const char *) gll, strlen( (const char *) gll));
        //uart_write_bytes(UART_NUM_0, (const char *) buf, len);
        send_GPS_coordination(gll);
        vTaskDelay(3000 / portTICK_RATE_MS);
    }
}

void app_main(void)
{
    uart_config();
    sim800_init();
    xTaskCreate(gps_coordination_task, "uart_gps_coordination_task", 1024, NULL, 10, NULL);
}
