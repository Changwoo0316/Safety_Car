/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#ifdef __GNUC__
/* With GCC, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
#define ARR_CNT 5
#define CMD_SIZE 50

#define LEFT ((uint8_t)rx1Data[1])
#define CENTER ((uint8_t)rx1Data[2])
#define RIGHT ((uint8_t)rx1Data[3])
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart6;

/* USER CODE BEGIN PV */
uint8_t rx1char;
uint8_t rx2char;
uint8_t rx6char;

volatile uint8_t frontViewFlag = 0xFF;
volatile uint8_t rxFrontDataFlag = 0;
volatile uint8_t rxSideDataFlag = 0;
volatile uint8_t uart1StopFlag = 0;
volatile uint8_t rx1Data[4];
volatile uint8_t rx2Data[4];
volatile uint8_t frontData;
volatile uint8_t collisionFlag = 0x00;

volatile uint8_t btnPressedFlag = 0; 

volatile uint8_t driveDirection = 0xFF;

volatile int rx_idx = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

void setMotorSpeed(int motor_idx, int speed);
void colision_Event();
void side_Event();
void front_Event(uint8_t dist);
void go_Straight();
void turn_Left();
void turn_Right();

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_TIM4_Init();
	MX_USART2_UART_Init();
	MX_USART6_UART_Init();
	MX_USART1_UART_Init();
	/* USER CODE BEGIN 2 */
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1); // 모터 1
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2); // 모터 2
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3); // 모터 3
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4); // 모터 4

	HAL_UART_Receive_IT(&huart1, &rx1char, 1);
	HAL_UART_Receive_IT(&huart2, &rx2char, 1);
	HAL_UART_Receive_IT(&huart6, &rx6char, 1); 



	go_Straight();    // 일단 출발
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{


		if (btnPressedFlag == 1)
		{
			btnPressedFlag = 0;
			HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);

			driveDirection = 0xFF;
			HAL_UART_Transmit(&huart6, (uint8_t*)&driveDirection, 1, 10); 
		}

		if (collisionFlag) {

			colision_Event();

		}

		else {

			if (rxFrontDataFlag)
			{
				uart1StopFlag = 0xFF;
				uint8_t currentDist = frontData;

				rxFrontDataFlag = 0; 

				front_Event(currentDist);
				uart1StopFlag = 0x00;
			}

			if (rxSideDataFlag)
			{
				uart1StopFlag = 0xFF;
				rxSideDataFlag = 0;
				side_Event();
				uart1StopFlag = 0x00;
			}
		}

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	*/
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	* in the RCC_OscInitTypeDef structure.
	*/
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 4;
	RCC_OscInitStruct.PLL.PLLN = 100;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	*/
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
		| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

	/* USER CODE BEGIN TIM4_Init 0 */

	/* USER CODE END TIM4_Init 0 */

	TIM_MasterConfigTypeDef sMasterConfig = { 0 };
	TIM_OC_InitTypeDef sConfigOC = { 0 };

	/* USER CODE BEGIN TIM4_Init 1 */

	/* USER CODE END TIM4_Init 1 */
	htim4.Instance = TIM4;
	htim4.Init.Prescaler = 100 - 1;
	htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim4.Init.Period = 50 - 1;
	htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
	{
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = 0;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN TIM4_Init 2 */

	/* USER CODE END TIM4_Init 2 */
	HAL_TIM_MspPostInit(&htim4);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

	/* USER CODE BEGIN USART1_Init 0 */

	/* USER CODE END USART1_Init 0 */

	/* USER CODE BEGIN USART1_Init 1 */

	/* USER CODE END USART1_Init 1 */
	huart1.Instance = USART1;
	huart1.Init.BaudRate = 9600;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart1) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN USART1_Init 2 */

	/* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

	/* USER CODE BEGIN USART2_Init 0 */

	/* USER CODE END USART2_Init 0 */

	/* USER CODE BEGIN USART2_Init 1 */

	/* USER CODE END USART2_Init 1 */
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 115200;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart2) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN USART2_Init 2 */

	/* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

	/* USER CODE BEGIN USART6_Init 0 */

	/* USER CODE END USART6_Init 0 */

	/* USER CODE BEGIN USART6_Init 1 */

	/* USER CODE END USART6_Init 1 */
	huart6.Instance = USART6;
	huart6.Init.BaudRate = 9600;
	huart6.Init.WordLength = UART_WORDLENGTH_8B;
	huart6.Init.StopBits = UART_STOPBITS_1;
	huart6.Init.Parity = UART_PARITY_NONE;
	huart6.Init.Mode = UART_MODE_TX_RX;
	huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart6.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart6) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN USART6_Init 2 */

	/* USER CODE END USART6_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	/* USER CODE BEGIN MX_GPIO_Init_1 */

	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOC, REAR_IN1_Pin | REAR_IN2_Pin | REAR_IN3_Pin | REAR_IN4_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, FRONT_IN1_Pin | FRONT_IN2_Pin | FRONT_IN3_Pin | FRONT_IN4_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : B1_Pin */
	GPIO_InitStruct.Pin = B1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : REAR_IN1_Pin REAR_IN2_Pin REAR_IN3_Pin REAR_IN4_Pin */
	GPIO_InitStruct.Pin = REAR_IN1_Pin | REAR_IN2_Pin | REAR_IN3_Pin | REAR_IN4_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pin : LD2_Pin */
	GPIO_InitStruct.Pin = LD2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : FRONT_IN1_Pin FRONT_IN2_Pin FRONT_IN3_Pin FRONT_IN4_Pin */
	GPIO_InitStruct.Pin = FRONT_IN1_Pin | FRONT_IN2_Pin | FRONT_IN3_Pin | FRONT_IN4_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

	/* USER CODE BEGIN MX_GPIO_Init_2 */

	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void setMotorSpeed(int motor_idx, int speed) {
	if (speed > 50) speed = 50; // ARR 최대값 제한
	if (speed < 0) speed = 0;

	switch (motor_idx) {
	case 1:
		__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, speed);
		break;
	case 2:
		__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, speed);
		break;
	case 3:
		__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, speed);
		break;
	case 4:
		__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, speed);
		break;
	}
}

void colision_Event()
{

	//모터 정지
	setMotorSpeed(1, 0);
	setMotorSpeed(2, 0);
	setMotorSpeed(3, 0);
	setMotorSpeed(4, 0);


}
PUTCHAR_PROTOTYPE
{
	/* Place your implementation of fputc here */
	/* e.g. write a character to the USART6 and Loop until the end of transmission */
	HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, 0xFFFF);

	return ch;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{


	if (huart->Instance == USART1) {

		if (!uart1StopFlag)
		{
			if (frontViewFlag == 0xFF) {
				frontData = rx1char;
				rxFrontDataFlag = 0xFF;
			}
			else {
				rx1Data[rx_idx] = rx1char;
				if (rx1Data[0] == 0xFF) {
					if (rx_idx == 3)
					{
						rxSideDataFlag = 0xFF;
						rx_idx = 0;
					}
					else
					{
						rx_idx++;
					}
				}
				else {
					rx_idx = 0;
				}
			}

		}

		HAL_UART_Receive_IT(&huart1, &rx1char, 1);

	}
	if (huart->Instance == USART6)
	{
		if (rx6char == 0xFF && collisionFlag == 0x00) {
			collisionFlag = 0xFF;

			colision_Event();
		}
		rx6char = 0x00;

		HAL_UART_Receive_IT(&huart6, &rx6char, 1);
	}
}

void side_Event() {

	uint32_t startTick;

	if (CENTER > 30) {

		frontViewFlag = 0xFF;

		HAL_UART_Transmit(&huart1, (uint8_t*)&frontViewFlag, 1, 10);

		//정상 주행
		driveDirection = 0xFF;
		HAL_UART_Transmit(&huart6, (uint8_t*)&driveDirection, 1, 10);

		go_Straight();

		rx1Data[0] = 0;
		rx1Data[1] = 0;
		rx1Data[2] = 0;
		rx1Data[3] = 0;
		return;

	}

	else {

		// 1. 왼쪽이 더 가까운 경우 (왼쪽이 위험 -> 오른쪽으로 회피)
		if (LEFT <= RIGHT)
		{

			//오른쪽 회피기동
			driveDirection = 0x0F;

			HAL_UART_Transmit(&huart6, (uint8_t*)&driveDirection, 1, 20);

			turn_Right();

			startTick = HAL_GetTick();
			while ((HAL_GetTick() - startTick) < 500) {
				if (collisionFlag == 0xFF) return; // 대기 중 충돌 감지 시 즉시 탈출
			}

			// 모터 정지
			setMotorSpeed(1, 0);
			setMotorSpeed(2, 0);
			setMotorSpeed(3, 0);
			setMotorSpeed(4, 0);

			startTick = HAL_GetTick();
			while ((HAL_GetTick() - startTick) < 400) {
				if (collisionFlag == 0xFF) return; // 대기 중 충돌 감지 시 즉시 탈출
			}

		}
		// 2. 오른쪽이 더 가까운 경우 (오른쪽이 위험 -> 왼쪽으로 회피)
		else
		{
			//왼쪽 회피기동
			driveDirection = 0xF0;

			//IMU 아두이노로 전송
			HAL_UART_Transmit(&huart6, (uint8_t*)&driveDirection, 1, 20);

			turn_Left();

			startTick = HAL_GetTick();
			while ((HAL_GetTick() - startTick) < 500) {
				if (collisionFlag == 0xFF) return; // 대기 중 충돌 감지 시 즉시 탈출
			}

			// 모터 정지
			setMotorSpeed(1, 0);
			setMotorSpeed(2, 0);
			setMotorSpeed(3, 0);
			setMotorSpeed(4, 0);

			startTick = HAL_GetTick();
			while ((HAL_GetTick() - startTick) < 400) {
				if (collisionFlag == 0xFF) return; // 대기 중 충돌 감지 시 즉시 탈출
			}

		}

	}

	rx1Data[0] = 0;
	rx1Data[1] = 0;
	rx1Data[2] = 0;
	rx1Data[3] = 0;

	return;
}

void front_Event(uint8_t dist) {

	if (dist < 30) {

		rx_idx = 0;          // 인덱스 초기화 (필수)
		frontViewFlag = 0x01; // 플래그 변경

		// 아두이노로 전송
		HAL_UART_Transmit(&huart1, (uint8_t*)&frontViewFlag, 1, 20);

		// 모터 정지
		setMotorSpeed(1, 0);
		setMotorSpeed(2, 0);
		setMotorSpeed(3, 0);
		setMotorSpeed(4, 0);

	}
	else {
		// 장애물 없음 -> 계속 직진
		HAL_UART_Transmit(&huart1, (uint8_t*)&frontViewFlag, 1, 20);
		go_Straight();
	}

	return;
}

void go_Straight() {

	HAL_GPIO_WritePin(REAR_IN1_GPIO_Port, REAR_IN1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(REAR_IN2_GPIO_Port, REAR_IN2_Pin, GPIO_PIN_RESET);

	HAL_GPIO_WritePin(REAR_IN3_GPIO_Port, REAR_IN3_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(REAR_IN4_GPIO_Port, REAR_IN4_Pin, GPIO_PIN_RESET);

	HAL_GPIO_WritePin(FRONT_IN1_GPIO_Port, FRONT_IN1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(FRONT_IN2_GPIO_Port, FRONT_IN2_Pin, GPIO_PIN_RESET);

	HAL_GPIO_WritePin(FRONT_IN3_GPIO_Port, FRONT_IN3_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(FRONT_IN4_GPIO_Port, FRONT_IN4_Pin, GPIO_PIN_RESET);

	setMotorSpeed(1, 32);
	setMotorSpeed(2, 32);
	setMotorSpeed(3, 32); //33
	setMotorSpeed(4, 32); //33
}

void turn_Left() {

	//4번모터
	HAL_GPIO_WritePin(REAR_IN1_GPIO_Port, REAR_IN1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(REAR_IN2_GPIO_Port, REAR_IN2_Pin, GPIO_PIN_SET);

	//3번 모터 후진
	HAL_GPIO_WritePin(REAR_IN3_GPIO_Port, REAR_IN3_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(REAR_IN4_GPIO_Port, REAR_IN4_Pin, GPIO_PIN_SET);

	//1번 모터
	HAL_GPIO_WritePin(FRONT_IN1_GPIO_Port, FRONT_IN1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(FRONT_IN2_GPIO_Port, FRONT_IN2_Pin, GPIO_PIN_RESET);

	//2번 모터 전진
	HAL_GPIO_WritePin(FRONT_IN3_GPIO_Port, FRONT_IN3_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(FRONT_IN4_GPIO_Port, FRONT_IN4_Pin, GPIO_PIN_RESET);

	setMotorSpeed(1, 0);
	setMotorSpeed(2, 42);
	setMotorSpeed(3, 42);
	setMotorSpeed(4, 0);
}

void turn_Right() {

	//4번 코터 후진
	HAL_GPIO_WritePin(REAR_IN1_GPIO_Port, REAR_IN1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(REAR_IN2_GPIO_Port, REAR_IN2_Pin, GPIO_PIN_SET);

	//3번 모터
	HAL_GPIO_WritePin(REAR_IN3_GPIO_Port, REAR_IN3_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(REAR_IN4_GPIO_Port, REAR_IN4_Pin, GPIO_PIN_RESET);

	//1번 모터 전진
	HAL_GPIO_WritePin(FRONT_IN1_GPIO_Port, FRONT_IN1_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(FRONT_IN2_GPIO_Port, FRONT_IN2_Pin, GPIO_PIN_RESET);

	//2번 모터
	HAL_GPIO_WritePin(FRONT_IN3_GPIO_Port, FRONT_IN3_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(FRONT_IN4_GPIO_Port, FRONT_IN4_Pin, GPIO_PIN_RESET);

	setMotorSpeed(1, 42);
	setMotorSpeed(2, 0);
	setMotorSpeed(3, 0);
	setMotorSpeed(4, 42);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == B1_Pin)
	{
		// 충돌 플래그 해제
		collisionFlag = 0;

		btnPressedFlag = 1;

	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
	if (huart->Instance == USART1)
	{
		HAL_UART_Receive_IT(&huart1, &rx1char, 1);
	}

	if (huart->Instance == USART6)
	{
		HAL_UART_Receive_IT(&huart6, &rx6char, 1);
	}
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1)
	{
	}
	/* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  * where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
	   ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	   /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
