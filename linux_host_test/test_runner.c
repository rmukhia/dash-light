#include <stdlib.h>
#include <core/fft_buffer.h>
#include <core/fft_dsp.h>
#include "unity.h"

/*******************************************************************************
* Author: Raunak Mukhia @rmukhia
* Date:   29/01/22
*
* File:  test_runner.c
* Descr: test business logic in host
*******************************************************************************/

/* do something before tests start */
void setUp(void)
{
}

/* do something after tests stop */
void tearDown(void)
{
}

void test_fft_buffer(void)
{
    int16_t data[FFT_SAMPLE_SIZE * 4];
    float result[FFT_SAMPLE_SIZE * 2];

    for (int i = 0; i < FFT_SAMPLE_SIZE * 4; i++) {
        data[i] = i;
    }
    fft_buffer_init();

    fft_buffer_set_params(400, STEREO, SIXTEEN);
    fft_buffer_print();

    fft_buffer_prepare_data(data, FFT_SAMPLE_SIZE * 4 * sizeof (int16_t));
    fft_buffer_print();
    fft_buffer_get_buf((float  *)&result);
    {
        float expected[] = { 2049, 0 ,  2051, 0, 2053, 0, 2055, 0};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, result, 8);
    }


    fft_buffer_prepare_data(data, 1 * sizeof (int16_t));
    fft_buffer_print();
    fft_buffer_get_buf((float  *)&result);
    {
        float expected[] = { 2049, 0, 2051, 0, 2053, 0, 2055};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, result, 7);
    }

    fft_buffer_prepare_data(data, 2048 * sizeof (int16_t));
    fft_buffer_print();
    fft_buffer_get_buf((float  *)&result);
    {
        float expected[] = { 1, 0, 3};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, result, 3);
    }

    fft_buffer_prepare_data(data, 1024* sizeof (int16_t));
    fft_buffer_print();
    fft_buffer_get_buf((float  *)&result);
    {
        float expected[] = { 1025, 0, 1027};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, result, 3);
    }

    fft_buffer_prepare_data(data, 1 * sizeof (int16_t));
    fft_buffer_print();
    fft_buffer_get_buf((float  *)&result);
    {
        float expected[] = { 1025, 0, 1027};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, result, 3);
    }

    fft_buffer_prepare_data(data, 2048 * sizeof (int16_t));
    fft_buffer_print();
    fft_buffer_get_buf((float  *)&result);
    {
        float expected[] = { 1, 0, 3};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, result, 3);
    }

    fft_buffer_deinit();
}

int main()
{
    system("./pre_test.sh");
    UNITY_BEGIN();
    RUN_TEST(test_fft_buffer);
    UNITY_END();
}
