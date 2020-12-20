#ifndef BSP_H_
#define BSP_H_

void initializeGpios();

#define PWMA_H_CCR_REGISTER TIMER_B_CAPTURECOMPARE_REGISTER_6
#define PWMB_H_CCR_REGISTER TIMER_B_CAPTURECOMPARE_REGISTER_4
#define PWMC_H_CCR_REGISTER TIMER_B_CAPTURECOMPARE_REGISTER_2
#define ADC_CCR_REGISTER    TIMER_B_CAPTURECOMPARE_REGISTER_1

#define IPHASE_A_ADC_CHAN    ADC12_B_INPUT_A2
#define IPHASE_B_ADC_CHAN    ADC12_B_INPUT_A12
#define IPHASE_C_ADC_CHAN    ADC12_B_INPUT_A13

#define READ_HALL_U     (P2IN & GPIO_PIN6)
#define READ_HALL_V     (P2IN & GPIO_PIN5)
#define READ_HALL_W     (P4IN & GPIO_PIN3)

#define READ_ADDR1      (P4IN & GPIO_PIN0)
#define READ_ADDR2      (P4IN & GPIO_PIN1)

#endif /* BSP_H_ */