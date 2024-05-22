#include "stm32f0xx.h" 
#include <stdint.h>

#define FLASH_PAGE_SIZE           ((uint32_t)0x400)   // Flash page size for STM32F0
#define FLASH_BASE_ADDRESS        ((uint32_t)0x08000000) // Flash base address for STM32F0
#define USER_FLASH_START_ADDRESS (FLASH_BASE_ADDRESS + (2 * FLASH_PAGE_SIZE)) 
#define USER_FLASH_END_ADDRESS   (FLASH_BASE_ADDRESS + (64 * FLASH_PAGE_SIZE)) 




// Function to erase a flash page
void Flash_PageErase(uint32_t Page_Address) {
    // Clear pending flags (if any)
    FLASH->SR |= FLASH_SR_EOP | FLASH_SR_WRPERR | FLASH_SR_PGERR;

    // Set the PER bit in the CR register to start page erase
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR = Page_Address; // Set the page address to be erased
    FLASH->CR |= FLASH_CR_STRT; // Start erase operation
    while ((FLASH->SR & FLASH_SR_BSY) != 0); // Wait for the operation to complete
    FLASH->CR &= ~FLASH_CR_PER; // Clear PER bit
}

// Function to write data to flash memory
void Flash_WriteData(uint32_t Address, uint32_t Data) {
    // Ensure the Flash programming is enabled
    if ((FLASH->CR & FLASH_CR_PG) == 0) {
        FLASH->CR |= FLASH_CR_PG; // Set the PG bit to enable programming
    }

    *(volatile uint32_t*)Address = Data; // Write data to flash address

    // Wait for the write operation to complete
    while ((FLASH->SR & FLASH_SR_BSY) != 0);

    // Clear PG bit to disable programming
    FLASH->CR &= ~FLASH_CR_PG;
}
