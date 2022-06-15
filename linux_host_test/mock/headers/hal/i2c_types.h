//
// Created by rmukhia on 2/6/22.
//

#ifndef TEST_APP_I2C_TYPES_H
#define TEST_APP_I2C_TYPES_H

typedef enum i2c_ack_type_e {
    I2C_MASTER_ACK = 0x0,
    I2C_MASTER_NACK = 0x1,
    I2C_MASTER_LAST_NACK = 0x2,
    I2C_MASTER_ACK_MAX
} i2c_ack_type_t;

#endif //TEST_APP_I2C_TYPES_H
