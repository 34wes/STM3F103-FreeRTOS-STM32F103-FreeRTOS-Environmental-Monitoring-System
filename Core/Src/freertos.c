/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "i2c.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
#include <string.h>

volatile uint16_t g_light_lux = 0;
uint8_t esp8266_rx_buf[128];
uint8_t esp8266_tx_buf[128];

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BH1750_ADDR 0x23
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
void BH1750_Init(void);
uint16_t BH1750_ReadLux(void);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
/* USER CODE END Variables */
osThreadId Task_LightColleHandle;
osThreadId Task_UartPrintHandle;
osThreadId Task_AlarmHandle;
osThreadId Task_esp8266_seHandle;
osThreadId Task_esp8266_reHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartTask_LightCollect(void const * argument);
void StartTask_UartPrint(void const * argument);
void StartTask_Alarm(void const * argument);
void StartTask_esp8266_send(void const * argument);
void StartTask_esp8266_recv(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of Task_LightColle */
  osThreadDef(Task_LightColle, StartTask_LightCollect, osPriorityAboveNormal, 0, 128);
  Task_LightColleHandle = osThreadCreate(osThread(Task_LightColle), NULL);

  /* definition and creation of Task_UartPrint */
  osThreadDef(Task_UartPrint, StartTask_UartPrint, osPriorityNormal, 0, 256);
  Task_UartPrintHandle = osThreadCreate(osThread(Task_UartPrint), NULL);

  /* definition and creation of Task_Alarm */
  osThreadDef(Task_Alarm, StartTask_Alarm, osPriorityHigh, 0, 256);
  Task_AlarmHandle = osThreadCreate(osThread(Task_Alarm), NULL);

  /* definition and creation of Task_esp8266_se */
  osThreadDef(Task_esp8266_se, StartTask_esp8266_send, osPriorityNormal, 0, 256);
  Task_esp8266_seHandle = osThreadCreate(osThread(Task_esp8266_se), NULL);

  /* definition and creation of Task_esp8266_re */
  osThreadDef(Task_esp8266_re, StartTask_esp8266_recv, osPriorityBelowNormal, 0, 256);
  Task_esp8266_reHandle = osThreadCreate(osThread(Task_esp8266_re), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  // 开启中断接收（只开这一次！）
  HAL_UART_Receive_IT(&huart2, esp8266_rx_buf, 1);
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartTask_LightCollect */
/**
  * @brief  Function implementing the Task_LightColle thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartTask_LightCollect */
void StartTask_LightCollect(void const * argument)
{
  /* USER CODE BEGIN StartTask_LightCollect */
  BH1750_Init();
  for(;;)
  {
    g_light_lux = BH1750_ReadLux();
    osDelay(1000);
  }
  /* USER CODE END StartTask_LightCollect */
}

/* USER CODE BEGIN Header_StartTask_UartPrint */
/**
* @brief Function implementing the Task_UartPrint thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask_UartPrint */
void StartTask_UartPrint(void const * argument)
{
  /* USER CODE BEGIN StartTask_UartPrint */
  char buf[64];
  for(;;)
  {
    sprintf(buf, "Light: %d lux\r\n", g_light_lux);
    HAL_UART_Transmit(&huart1, (uint8_t*)buf, strlen(buf), 100);
    osDelay(1000);
  }
  /* USER CODE END StartTask_UartPrint */
}

/* USER CODE BEGIN Header_StartTask_Alarm */
/**
* @brief Function implementing the Task_Alarm thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask_Alarm */
void StartTask_Alarm(void const * argument)
{
  /* USER CODE BEGIN StartTask_Alarm */
  for(;;)
  {
    if(g_light_lux < 5)
    {
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
      HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
      osDelay(30);
      HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_SET);
      osDelay(200);
    }
    else if(g_light_lux > 600)
    {
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
      HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
      osDelay(30);
      HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_SET);
      osDelay(200);
    }
    else
    {
      HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_SET);
      osDelay(500);
    }
  }
  /* USER CODE END StartTask_Alarm */
}

/* USER CODE BEGIN Header_StartTask_esp8266_send */
/**
* @brief Function implementing the Task_esp8266_se thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask_esp8266_send */
void StartTask_esp8266_send(void const * argument)
{
  /* USER CODE BEGIN StartTask_esp8266_send */
  uint8_t tx_buf[128];
  osDelay(2000);  // 等待ESP8266上电稳定

  // 测试AT
  sprintf((char*)tx_buf, "AT\r\n");
  HAL_UART_Transmit(&huart2, tx_buf, strlen((char*)tx_buf), 100);
  osDelay(1000);

  // 关闭回显（关键！防止乱数据）
  sprintf((char*)tx_buf, "ATE0\r\n");
  HAL_UART_Transmit(&huart2, tx_buf, strlen((char*)tx_buf), 100);
  osDelay(1000);

  // 设置Station模式
  sprintf((char*)tx_buf, "AT+CWMODE=1\r\n");
  HAL_UART_Transmit(&huart2, tx_buf, strlen((char*)tx_buf), 100);
  osDelay(2000);

  // 重启生效
  sprintf((char*)tx_buf, "AT+RST\r\n");
  HAL_UART_Transmit(&huart2, tx_buf, strlen((char*)tx_buf), 100);
  osDelay(3000);

  // 连接WiFi
  sprintf((char*)tx_buf, "AT+CWJAP=\"iQOO Z10 Turbo+\",\"12345678\"\r\n");
  HAL_UART_Transmit(&huart2, tx_buf, strlen((char*)tx_buf), 100);
  osDelay(8000);

  // 单连接模式
  sprintf((char*)tx_buf, "AT+CIPMUX=0\r\n");
  HAL_UART_Transmit(&huart2, tx_buf, strlen((char*)tx_buf), 100);
  osDelay(1000);

  // 连接TCP服务器
  sprintf((char*)tx_buf, "AT+CIPSTART=\"TCP\",\"192.168.94.228\",8081\r\n");
  HAL_UART_Transmit(&huart2, tx_buf, strlen((char*)tx_buf), 100);
  osDelay(3000);

  for(;;)
  {
    // 发送数据
    sprintf((char*)tx_buf, "AT+CIPSEND=%d\r\n", 16);
    HAL_UART_Transmit(&huart2, tx_buf, strlen((char*)tx_buf), 100);
    osDelay(100);

    sprintf((char*)tx_buf, "Light: %d lux\r\n", g_light_lux);
    HAL_UART_Transmit(&huart2, tx_buf, strlen((char*)tx_buf), 100);

    osDelay(2000);
  }
  /* USER CODE END StartTask_esp8266_send */
}

/* USER CODE BEGIN Header_StartTask_esp8266_recv */
/**
* @brief Function implementing the Task_esp8266_re thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask_esp8266_recv */
void StartTask_esp8266_recv(void const * argument)
{
  /* USER CODE BEGIN StartTask_esp8266_recv */
  for(;;)
  {
    // 空任务，只用来让系统调度，接收全部由中断完成
    osDelay(1000);
  }
  /* USER CODE END StartTask_esp8266_recv */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void BH1750_Init(void)
{
  uint8_t cmd = 0x01;
  HAL_I2C_Master_Transmit(&hi2c2, BH1750_ADDR<<1, &cmd, 1, 100);
}

uint16_t BH1750_ReadLux(void)
{
  uint8_t buf[2] = {0};
  uint8_t cmd = 0x20;
  HAL_I2C_Master_Transmit(&hi2c2, BH1750_ADDR<<1, &cmd, 1, 100);
  HAL_Delay(180);
  HAL_I2C_Master_Receive(&hi2c2, BH1750_ADDR<<1, buf, 2, 100);
  return (buf[0] << 8 | buf[1]) / 1.2f;
}

// 中断接收（正常转发）
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if(huart->Instance == USART2)
  {
    HAL_UART_Transmit(&huart1, esp8266_rx_buf, 1, 100);
    HAL_UART_Receive_IT(&huart2, esp8266_rx_buf, 1);
  }
}
/* USER CODE END Application */

