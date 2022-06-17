#include <stdlib.h>
#include "fft/fft_task.h"
#include "fft/fft_buffer.h"
#include "fft/fft_display.h"
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
    float *result;

    for (int i = 0; i < FFT_SAMPLE_SIZE * 4; i++) {
        data[i] = i;
    }
    fft_buffer_init();

    fft_buffer_set_params(400, STEREO, SIXTEEN);
    fft_buffer_print();

    fft_buffer_prepare_data(data, FFT_SAMPLE_SIZE * 4 * sizeof (int16_t));
    fft_buffer_print();
    result = fft_buffer_get_buf();
    {
        float expected[] = { 1025, 0 ,  1027, 0, 1029, 0, 1031, 0};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, result, 8);
    }


    fft_buffer_prepare_data(data, 1 * sizeof (int16_t));
    fft_buffer_print();
    result = fft_buffer_get_buf();
    {
        float expected[] = { 1025, 0, 1027, 0, 1029, 0, 1031};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, result, 7);
    }

    fft_buffer_prepare_data(data, 2048 * sizeof (int16_t));
    fft_buffer_print();
    result = fft_buffer_get_buf();
    {
        float expected[] = { 1025, 0, 1027};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, result, 3);
    }

    fft_buffer_prepare_data(data, 1024* sizeof (int16_t));
    fft_buffer_print();
    result = fft_buffer_get_buf();
    {
        float expected[] = { 1, 0, 3};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, result, 3);
    }

    fft_buffer_prepare_data(data, 1 * sizeof (int16_t));
    fft_buffer_print();
    result = fft_buffer_get_buf();
    {
        float expected[] = { 1, 0, 3};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, result, 3);
    }

    fft_buffer_prepare_data(data, 2048 * sizeof (int16_t));
    fft_buffer_print();
    result = fft_buffer_get_buf();
    {
        float expected[] = { 1025, 0, 1027};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, result, 3);
    }

    fft_buffer_deinit();
}

void test_fft_bucketing(void)
{
    size_t fft_size = FFT_SAMPLE_SIZE/2 + 1;
    float amplitude[fft_size];
    float *amplitude_bins;
    size_t out_bin_len;

    fft_display_set_params(44100, FFT_SAMPLE_SIZE, 10);

    for(size_t n = 0; n < fft_size; n++) {
        amplitude[n] = n;
    }

    amplitude_bins = fft_get_amplitude_bins(amplitude, fft_size, &out_bin_len);

    for (size_t n = 0; n < 10; n ++) {
        printf("band[%i]: %f ", n, amplitude_bins[n]);
    }
}

int main()
{
    system("./pre_test.sh");
    UNITY_BEGIN();
    //RUN_TEST(test_fft_buffer);
    RUN_TEST(test_fft_bucketing);
    UNITY_END();
}
