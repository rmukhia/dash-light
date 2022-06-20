/**************************************************
 * Author: rmukhia
 * Creation Date: 20/6/22
 * Description: 
 **************************************************/

#ifndef FREQB_BANDS_H
#define FREQB_BANDS_H

static const int band_3_freq[] = {120, 1000, 16000};
static const int band_4_freq[] = {120, 800, 2000, 16000};
static const int band_10_freq[] = {60, 120, 250, 355, 710, 1420, 2840, 5680, 11360, 16000};

static const int *freqb_get_num_bands(unsigned int num_bands)
{
    switch (num_bands) {
        case 3:
            return band_3_freq;
        case 4:
            return band_4_freq;
        case 10:
        default:
            return band_10_freq;
    }
}

#endif //FREQB_BANDS_H
