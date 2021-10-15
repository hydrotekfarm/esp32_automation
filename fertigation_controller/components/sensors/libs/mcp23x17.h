/**
 * @file mcp23x17.h
 * @defgroup mcp23x17 mcp23x17
 * @{
 *
 * ESP-IDF driver for I2C/SPI 16 bit GPIO expanders MCP23017/MCP23S17
 *
 * Copyright (C) 2018 Ruslan V. Uss <https://github.com/UncleRus>
 *
 * BSD Licensed as described in the file LICENSE
 */
#ifndef __MCP23X17_H__
#define __MCP23X17_H__

#include <stddef.h>
#include <stdbool.h>
#include <i2cdev.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_err.h>

#define MCP23X17_ADDR_BASE 0x20

#ifdef __cplusplus
extern "C" {
#endif

typedef i2c_dev_t mcp23x17_t;

/**
 * GPIO mode
 */
typedef enum
{
    MCP23X17_GPIO_OUTPUT = 0,
    MCP23X17_GPIO_INPUT
} mcp23x17_gpio_mode_t;

/**
 * INTA/INTB pins mode
 */
typedef enum
{
    MCP23X17_ACTIVE_LOW = 0, //!< Low level on interrupt
    MCP23X17_ACTIVE_HIGH,    //!< High level on interrupt
    MCP23X17_OPEN_DRAIN      //!< Open drain
} mcp23x17_int_out_mode_t;

/**
 * Interrupt mode
 */
typedef enum
{
    MCP23X17_INT_DISABLED = 0, //!< No interrupt
    MCP23X17_INT_LOW_EDGE,     //!< Interrupt on low edge
    MCP23X17_INT_HIGH_EDGE,    //!< Interrupt on high edge
    MCP23X17_INT_ANY_EDGE      //!< Interrupt on any edge
} mcp23x17_gpio_intr_t;


/**
 * @brief Initialize device descriptior
 * SCL frequency is 1MHz
 * @param dev Pointer to device descriptor
 * @param port I2C port number
 * @param addr I2C address
 * @param sda_gpio SDA GPIO
 * @param scl_gpio SCL GPIO
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_init_desc(mcp23x17_t *dev, i2c_port_t port, uint8_t addr, gpio_num_t sda_gpio, gpio_num_t scl_gpio);

/**
 * @brief Free device descriptor
 * @param dev Pointer to device descriptor
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_free_desc(mcp23x17_t *dev);



esp_err_t mcp23x17_get_int_out_mode(mcp23x17_t *dev, mcp23x17_int_out_mode_t *mode);

/**
 * Set INTA/INTB pins mode
 * @param dev Pointer to device descriptor
 * @param mode INTA/INTB pins mode
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_set_int_out_mode(mcp23x17_t *dev, mcp23x17_int_out_mode_t mode);

/**
 * @brief Get GPIO pins mode
 *
 * 0 - output, 1 - input for each bit in `val`
 *
 * @param dev Pointer to device descriptor
 * @param[out] val Buffer to store mode, 0 bit for PORTA/GPIO0..15 bit for PORTB/GPIO7
 * @return
 */
esp_err_t mcp23x17_port_get_mode(mcp23x17_t *dev, uint16_t *val);

/**
 * @brief Set GPIO pins mode
 *
 * 0 - output, 1 - input for each bit in `val`
 *
 * @param dev Pointer to device descriptor
 * @param val Mode, 0 bit for PORTA/GPIO0..15 bit for PORTB/GPIO7
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_port_set_mode(mcp23x17_t *dev, uint16_t val);

/**
 * @brief Get GPIO pullups status
 *
 * 0 - pullup disabled, 1 - pullup enabled for each bit in `val`
 *
 * @param dev Pointer to device descriptor
 * @param[out] val Pullup status, 0 bit for PORTA/GPIO0..15 bit for PORTB/GPIO7
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_port_get_pullup(mcp23x17_t *dev, uint16_t *val);

/**
 * @brief Set GPIO pullups status
 *
 * 0 - pullup disabled, 1 - pullup enabled for each bit in `val`
 *
 * @param dev Pointer to device descriptor
 * @param val Pullup status, 0 bit for PORTA/GPIO0..15 bit for PORTB/GPIO7
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_port_set_pullup(mcp23x17_t *dev, uint16_t val);

/**
 * @brief Read GPIO port value
 * @param dev Pointer to device descriptor
 * @param[out] val 16-bit GPIO port value, 0 bit for PORTA/GPIO0..15 bit for PORTB/GPIO7
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_port_read(mcp23x17_t *dev, uint16_t *val);

/**
 * @brief Write value to GPIO port
 * @param dev Pointer to device descriptor
 * @param val GPIO port value, 0 bit for PORTA/GPIO0..15 bit for PORTB/GPIO7
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_port_write(mcp23x17_t *dev, uint16_t val);

/**
 * Get GPIO pin mode
 * @param dev Pointer to device descriptor
 * @param pin Pin number, 0 for PORTA/GPIO0..15 for PORTB/GPIO7
 * @param[out] mode GPIO pin mode
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_get_mode(mcp23x17_t *dev, uint8_t pin, mcp23x17_gpio_mode_t *mode);

/**
 * Set GPIO pin mode
 * @param dev Pointer to device descriptor
 * @param pin Pin number, 0 for PORTA/GPIO0..15 for PORTB/GPIO7
 * @param mode GPIO pin mode
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_set_mode(mcp23x17_t *dev, uint8_t pin, mcp23x17_gpio_mode_t mode);

/**
 * @brief Get pullup mode of GPIO pin
 * @param dev Pointer to device descriptor
 * @param pin Pin number, 0 for PORTA/GPIO0..15 for PORTB/GPIO7
 * @param[out] enable pullup mode
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_get_pullup(mcp23x17_t *dev, uint8_t pin, bool *enable);

/**
 * @brief Set pullup mode of GPIO pin
 * @param dev Pointer to device descriptor
 * @param pin Pin number, 0 for PORTA/GPIO0..15 for PORTB/GPIO7
 * @param enable `true` to enable pullup
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_set_pullup(mcp23x17_t *dev, uint8_t pin, bool enable);

/**
 * @brief Read GPIO pin level
 * @param dev Pointer to device descriptor
 * @param pin Pin number, 0 for PORTA/GPIO0..15 for PORTB/GPIO7
 * @param[out] val `true` if pin currently in high state
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_get_level(mcp23x17_t *dev, uint8_t pin, uint32_t *val);

/**
 * @brief Set GPIO pin level
 *
 * Pin must be set up as output
 *
 * @param dev Pointer to device descriptor
 * @param pin Pin number, 0 for PORTA/GPIO0..15 for PORTB/GPIO7
 * @param[out] val `true` if pin currently in high state
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_set_level(mcp23x17_t *dev, uint8_t pin, uint32_t val);

/**
 * Setup interrupt for group of GPIO pins
 * @param dev Pointer to device descriptor
 * @param mask Pins to setup
 * @param intr Interrupt mode
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_port_set_interrupt(mcp23x17_t *dev, uint16_t mask, mcp23x17_gpio_intr_t intr);

/**
 * Setup interrupt for GPIO pin
 * @param dev Pointer to device descriptor
 * @param pin Pin number, 0 for PORTA/GPIO0..15 for PORTB/GPIO7
 * @param intr Interrupt mode
 * @return `ESP_OK` on success
 */
esp_err_t mcp23x17_set_interrupt(mcp23x17_t *dev, uint8_t pin, mcp23x17_gpio_intr_t intr);


#ifdef __cplusplus
}
#endif

/**@}*/

#endif /* __MCP23X17_H__ */
