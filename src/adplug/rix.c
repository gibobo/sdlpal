/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2007 Simon Peter, <dn.tlp@gmx.net>, et al.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * rix.cpp - Softstar RIX OPL Format Player by palxex <palxex.ys168.com>
 *                                             BSPAL <BSPAL.ys168.com>
 */

#include "rix.h"
#include "../util.h"
#include "opl.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static const unsigned char adflag[] = {0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1};
static const unsigned char reg_data[] = {0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, 20, 21};
static const unsigned char ad_C0_offs[] = {0, 1, 2, 0, 1, 2, 3, 4, 5, 3, 4, 5, 6, 7, 8, 6, 7, 8};
static const unsigned char modify[] = {0, 3, 1, 4, 2, 5, 6, 9, 7, 10, 8, 11, 12, 15, 13, 16, 14, 17, 12, 15, 16, 0, 14, 0, 17, 0, 13, 0};
static const unsigned char bd_reg_data[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x08, 0x04, 0x02, 0x01,
    0x00, 0x01, 0x01, 0x03, 0x0F, 0x05, 0x00, 0x01, 0x03, 0x0F, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x0F, 0x07, 0x00, 0x02,
    0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A,
    0x04, 0x00, 0x08, 0x0C, 0x0B, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x0D, 0x04, 0x00, 0x06, 0x0F, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x0C, 0x00, 0x0F, 0x0B, 0x00, 0x08, 0x05, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x0F, 0x0B, 0x00,
    0x07, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x0F, 0x0B, 0x00, 0x05, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x0F, 0x0B, 0x00, 0x07, 0x05, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00};

static void *fpMusMKF = NULL;
static unsigned char *rix_buf = NULL; /* rix files' f_buffer */
static int songs = 0;
static unsigned short f_buffer[25 * 12]; // 9C0h-C18h
static unsigned short a0b0_data2[11];
static unsigned char a0b0_data3[18];
static unsigned char a0b0_data4[18];
static unsigned char a0b0_data5[96];
static unsigned char addrs_head[96];
static unsigned short insbuf[28];
static unsigned short displace[11];
static unsigned char reg_bufs[18][14];
static unsigned int pos, length, subsong_id;
static unsigned char for40reg[18];
static unsigned int I, T;
static unsigned short mus_block;
static unsigned short ins_block;
static unsigned char rhythm;
static unsigned char music_on;
static unsigned char pause_flag;
static unsigned short band;
static unsigned char band_low;
static unsigned short e0_reg_flag;
static unsigned char bd_modify;
static int sustain;
static unsigned char play_end;

void ad_20_reg(unsigned short);
void ad_40_reg(unsigned short);
void ad_60_reg(unsigned short);
void ad_80_reg(unsigned short);
void ad_a0b0_reg(unsigned short);
void ad_a0b0l_reg(unsigned short, unsigned short, unsigned short);
void ad_a0b0l_reg_(unsigned short, unsigned short, unsigned short);
void ad_bd_reg();
void ad_bop(unsigned short, unsigned short);
void ad_C0_reg(unsigned short);
void ad_E0_reg(unsigned short);
unsigned short ad_initial();
void data_initial(); /* done */
void ins_to_reg(unsigned short, unsigned short *, unsigned short);
void int_08h_entry();
void music_ctrl();
void prepare_a0b0(unsigned short, unsigned short);
void rix_90_pro(unsigned short);
void rix_A0_pro(unsigned short, unsigned short);
void rix_B0_pro(unsigned short, unsigned short);
void rix_C0_pro(unsigned short, unsigned short);
void rix_get_ins();
unsigned short rix_proc();
void switch_ad_bd(unsigned short);

/*** public methods *************************************/
unsigned char CrixPlayer_update() {
   int_08h_entry();
   return !play_end;
}

void CrixPlayer_deinit(void) {
   UTIL_fclose(fpMusMKF);
   UTIL_free(rix_buf);
   fpMusMKF = NULL;
   rix_buf = NULL;
   subsong_id = 0xFFFFFFFF;
}

unsigned char CrixPlayer_load(const char *filename) {
   fpMusMKF = UTIL_fopen(filename, "rb");
   UTIL_fseek(fpMusMKF, 0, SEEK_SET);
   UTIL_fread(&songs, sizeof(int), 1, fpMusMKF);
   UTIL_fseek(fpMusMKF, songs, SEEK_SET);

   unsigned short signature;
   UTIL_fread(&signature, 2, 1, fpMusMKF);
   if (signature != 0x55aa) {
      UTIL_fclose(fpMusMKF);
      return false;
   }
   songs /= 4;
   subsong_id = 0xFFFFFFFF;
   CrixPlayer_rewind(0, true);
   return true;
}

void CrixPlayer_rewind(unsigned int subsong, unsigned char reinit) {
   play_end = 0;
   pos = 0;

   if (reinit) {
      I = 0;
      T = 0;
      mus_block = 0;
      ins_block = 0;
      rhythm = 0;
      music_on = 0;
      pause_flag = 0;
      band = 0;
      band_low = 0;
      e0_reg_flag = 0;
      bd_modify = 0;
      sustain = 0;

      memset(f_buffer, 0, sizeof(f_buffer));
      memset(a0b0_data2, 0, sizeof(a0b0_data2));
      memset(a0b0_data3, 0, sizeof(a0b0_data3));
      memset(a0b0_data4, 0, sizeof(a0b0_data4));
      memset(a0b0_data5, 0, sizeof(a0b0_data5));
      memset(addrs_head, 0, sizeof(addrs_head));
      memset(insbuf, 0, sizeof(insbuf));
      memset(displace, 0, sizeof(displace));
      memset(reg_bufs, 0, sizeof(reg_bufs));
      memset(for40reg, 0x7F, sizeof(for40reg));
   }

   if (subsong != subsong_id) {
      int index[2];
      UTIL_free(rix_buf);
      rix_buf = NULL;
      subsong_id = subsong;
      UTIL_fseek(fpMusMKF, subsong * sizeof(int), SEEK_SET);
      UTIL_fread(index, sizeof(int), 2, fpMusMKF);
      length = index[1] - index[0];

      if (length == 0)
         return;
      rix_buf = (unsigned char *)UTIL_calloc(length, sizeof(unsigned char));
      UTIL_fseek(fpMusMKF, index[0], SEEK_SET);
      UTIL_fread(rix_buf, length, sizeof(unsigned char), fpMusMKF);
   }

   if (reinit) {
      Copl_reset();
      Copl_write(1, 32); // go to OPL2 mode
      ad_initial();
      data_initial();
   }
}

/*----------------------------------------------------------*/
void ad_a0b0l_reg_(unsigned short index, unsigned short p2, unsigned short p3) {
   a0b0_data4[index] = (unsigned char)p3;
   a0b0_data3[index] = (unsigned char)p2;
}

void data_initial() {
   if (rix_buf == NULL)
      return;
   if (0x0D < length) {
      rhythm = rix_buf[2];
      mus_block = ((unsigned short)rix_buf[0x0D] << 8) + rix_buf[0x0C];
      ins_block = ((unsigned short)rix_buf[0x09] << 8) + rix_buf[0x08];
      I = mus_block + 1;
   } else
      I = mus_block = length; // file too short; will stop playing immediately
   if (rhythm != 0) {
      ad_a0b0l_reg_(8, 0x18, 0);
      ad_a0b0l_reg_(7, 0x1F, 0);
      Copl_write(0xa8, 87);
      Copl_write(0xb8, 9);
      Copl_write(0xa7, 3);
      Copl_write(0xb7, 15 /*10*/); // Changed from 10 (original value) to 15 for better quality
   }
   bd_modify = 0;
   band = 0;
   music_on = 1;
}
/*----------------------------------------------------------*/
unsigned short ad_initial() {
   unsigned short i, j, k = 0;
   for (i = 0; i < 25; i++) {
      unsigned int res = ((unsigned int)i * 24 + 10000) * 52088 / 250000 * 0x24000 / 0x1B503;
      f_buffer[i * 12] = ((unsigned short)res + 4) >> 3;
      for (int t = 1; t < 12; t++) {
         res = res * 106 / 100;
         f_buffer[i * 12 + t] = ((unsigned short)res + 4) >> 3;
      }
   }
   for (i = 0; i < 8; i++)
      for (j = 0; j < 12; j++) {
         a0b0_data5[k] = (unsigned char)i;
         addrs_head[k] = (unsigned char)j;
         k++;
      }
   // ad_bd_reg();
   // ad_bop(8, 0);
   // for(i=0;i<9;i++) ad_a0b0_reg(i);
   e0_reg_flag = 0x20;
   // for(i=0;i<18;i++) ad_bop(0xE0+reg_data[i],0);
   // ad_bop(1,e0_reg_flag);
   return 1; // ad_test();
}
/*----------------------------------------------------------*/
void ad_bop(unsigned short reg, unsigned short value) {
   Copl_write(reg & 0xff, value & 0xff);
}
/*--------------------------------------------------------------*/
void int_08h_entry() {
   unsigned short band_sus = 1;
   while (band_sus) {
      if (sustain <= 0) {
         band_sus = rix_proc();
         if (band_sus)
         sustain += band_sus;
         else {
         play_end = 1;
         break;
         }
      } else {
         if (band_sus)
         sustain -= 14; /* aging */
         break;
      }
   }
}
/*--------------------------------------------------------------*/
unsigned short rix_proc() {
   unsigned char ctrl = 0;
   if (music_on == 0 || pause_flag == 1 || rix_buf == NULL)
      return 0;
   band = 0;
   while (I < length && rix_buf[I] != 0x80) {
      band_low = rix_buf[I - 1];
      ctrl = rix_buf[I];
      I += 2;
      switch (ctrl & 0xF0) {
      case 0x90:
         rix_get_ins();
         rix_90_pro(ctrl & 0x0F);
         break;
      case 0xA0:
         rix_A0_pro(ctrl & 0x0F, ((unsigned short)band_low) << 6);
         break;
      case 0xB0:
         rix_B0_pro(ctrl & 0x0F, band_low);
         break;
      case 0xC0:
         switch_ad_bd(ctrl & 0x0F);
         if (band_low != 0)
         rix_C0_pro(ctrl & 0x0F, band_low);
         break;
      default:
         band = ((unsigned short)ctrl << 8) + band_low;
         break;
      }
      if (band != 0)
         return band;
   }
   music_ctrl();
   I = mus_block + 1;
   band = 0;
   music_on = 1;
   return 0;
}
/*--------------------------------------------------------------*/
void rix_get_ins() {
   if (rix_buf == NULL)
      return;
   if (ins_block + (band_low << 6) + sizeof(insbuf) >= length)
      return;

   int i;
   unsigned char *baddr = (&rix_buf[ins_block]) + (band_low << 6);

   for (i = 0; i < 28; i++)
      insbuf[i] = ((unsigned short)baddr[i * 2 + 1] << 8) + baddr[i * 2];
}
/*--------------------------------------------------------------*/
void rix_90_pro(unsigned short ctrl_l) {
   if (ctrl_l >= 11)
      return; // modify[] has size 28

   if (rhythm == 0 || ctrl_l < 6) {
      ins_to_reg(modify[ctrl_l * 2], insbuf, insbuf[26]);
      ins_to_reg(modify[ctrl_l * 2 + 1], insbuf + 13, insbuf[27]);
   } else if (ctrl_l > 6) {
      ins_to_reg(modify[ctrl_l * 2 + 6], insbuf, insbuf[26]);
   } else {
      // same effect as 1st case, no need to handle separately
      ins_to_reg(12, insbuf, insbuf[26]);
      ins_to_reg(15, insbuf + 13, insbuf[27]);
   }
}
/*--------------------------------------------------------------*/
void rix_A0_pro(unsigned short ctrl_l, unsigned short index) {
   if (rhythm == 0 || ctrl_l <= 6) {
      prepare_a0b0(ctrl_l, index > 0x3FFF ? 0x3FFF : index);
      ad_a0b0l_reg(ctrl_l, a0b0_data3[ctrl_l], a0b0_data4[ctrl_l]);
   }
}
/*--------------------------------------------------------------*/
void prepare_a0b0(unsigned short index, unsigned short v) /* important !*/
{
   signed short high = 0;
   signed short low = 0;
   unsigned int res;
   int res1 = (v - 0x2000) * 0x19;
   if (res1 == (int)0xff)
      return;
   low = res1 / 0x2000;
   if (low < 0) {
      low = 0x18 - low;
      res = low < 0 ? 0xFFFF : 0;
      res <<= 16;
      res += low;
      low = ((signed short)res) / (signed short)0xFFE7;
      a0b0_data2[index] = low;
      low = res;
      res = low - 0x18;
      high = (signed short)res % 0x19;
      low = (signed short)res / 0x19;
      if (high != 0) {
         low = 0x19;
         low = low - high;
      }
   } else {
      res = high = low;
      low = (signed short)res / (signed short)0x19;
      a0b0_data2[index] = low;
      res = high;
      low = (signed short)res % (signed short)0x19;
   }
   low = (signed short)low * (signed short)0x18;
   displace[index] = low;
}
/*--------------------------------------------------------------*/
void ad_a0b0l_reg(unsigned short index, unsigned short p2, unsigned short p3) {
   unsigned short data;
   unsigned short i = p2 + a0b0_data2[index];
   ad_a0b0l_reg_(index, p2, p3);
   i = ((signed short)i <= 0x5F ? i : 0x5F);
   i = ((signed short)i >= 0 ? i : 0);
   data = f_buffer[addrs_head[i] + displace[index] / 2];
   ad_bop(0xA0 + index, data);
   data = a0b0_data5[i] * 4 + (p3 < 1 ? 0 : 0x20) + ((data >> 8) & 3);
   ad_bop(0xB0 + index, data);
}
/*--------------------------------------------------------------*/
void rix_B0_pro(unsigned short ctrl_l, unsigned short index) {
   if (ctrl_l >= 11)
      return;
   int temp = 0;
   if (rhythm == 0 || ctrl_l < 6)
      temp = modify[ctrl_l * 2 + 1];
   else {
      temp = ctrl_l > 6 ? ctrl_l * 2 : ctrl_l * 2 + 1;
      temp = modify[temp + 6];
   }
   for40reg[temp] = index > 0x7F ? 0x7F : index;
   ad_40_reg(temp);
}
/*--------------------------------------------------------------*/
void rix_C0_pro(unsigned short ctrl_l, unsigned short index) {
   unsigned short i = index >= 12 ? index - 12 : 0;
   if (ctrl_l < 6 || rhythm == 0) {
      ad_a0b0l_reg(ctrl_l, i, 1);
   } else {
      if (ctrl_l == 6)
         ad_a0b0l_reg(ctrl_l, i, 0);
      else if (ctrl_l == 8) {
         ad_a0b0l_reg(ctrl_l, i, 0);
         ad_a0b0l_reg(7, i + 7, 0);
      }
      bd_modify |= bd_reg_data[ctrl_l];
      ad_bd_reg();
   }
}
/*--------------------------------------------------------------*/
void switch_ad_bd(unsigned short index) {
   if (rhythm == 0 || index < 6)
      ad_a0b0l_reg(index, a0b0_data3[index], 0);
   else {
     bd_modify &= (~bd_reg_data[index]);
     ad_bd_reg();
   }
}
/*--------------------------------------------------------------*/
void ins_to_reg(unsigned short index, unsigned short *insb, unsigned short value) {
   unsigned short i;
   for (i = 0; i < 13; i++)
      reg_bufs[index][i] = (unsigned char)insb[i];
   reg_bufs[index][13] = value & 3;
   ad_bd_reg();
   ad_bop(8, 0);
   ad_40_reg(index);
   ad_C0_reg(index);
   ad_60_reg(index);
   ad_80_reg(index);
   ad_20_reg(index);
   ad_E0_reg(index);
}
/*--------------------------------------------------------------*/
void ad_E0_reg(unsigned short index) {
   unsigned short data = e0_reg_flag == 0 ? 0 : (reg_bufs[index][13] & 3);
   ad_bop(0xE0 + reg_data[index], data);
}
/*--------------------------------------------------------------*/
void ad_20_reg(unsigned short index) {
   unsigned short data = (reg_bufs[index][9] < 1 ? 0 : 0x80);
   data += (reg_bufs[index][10] < 1 ? 0 : 0x40);
   data += (reg_bufs[index][5] < 1 ? 0 : 0x20);
   data += (reg_bufs[index][11] < 1 ? 0 : 0x10);
   data += (reg_bufs[index][1] & 0x0F);
   ad_bop(0x20 + reg_data[index], data);
}
/*--------------------------------------------------------------*/
void ad_80_reg(unsigned short index) {
   unsigned short data = (reg_bufs[index][7] & 0x0F), temp = reg_bufs[index][4];
   data |= (temp << 4);
   ad_bop(0x80 + reg_data[index], data);
}
/*--------------------------------------------------------------*/
void ad_60_reg(unsigned short index) {
   unsigned short data = reg_bufs[index][6] & 0x0F, temp = reg_bufs[index][3];
   data |= (temp << 4);
   ad_bop(0x60 + reg_data[index], data);
}
/*--------------------------------------------------------------*/
void ad_C0_reg(unsigned short index) {
   if (adflag[index] == 1)
      return;
   unsigned short data = reg_bufs[index][2];
   data *= 2;
   data |= (reg_bufs[index][12] < 1 ? 1 : 0);
   ad_bop(0xC0 + ad_C0_offs[index], data);
}
/*--------------------------------------------------------------*/
void ad_40_reg(unsigned short index) {
   unsigned int res = 0;
   unsigned short data = 0, temp = reg_bufs[index][0];
   data = 0x3F - (0x3F & reg_bufs[index][8]);
   data *= for40reg[index];
   data *= 2;
   data += 0x7F;
   res = data;
   data = res / 0xFE;
   data -= 0x3F;
   data = -data;
   data |= temp << 6;
   ad_bop(0x40 + reg_data[index], data);
}
/*--------------------------------------------------------------*/
void ad_bd_reg() {
   unsigned short data = rhythm < 1 ? 0 : 0x20;
   data |= bd_modify;
   ad_bop(0xBD, data);
}
/*--------------------------------------------------------------*/
void ad_a0b0_reg(unsigned short index) {
   ad_bop(0xA0 + index, 0);
   ad_bop(0xB0 + index, 0);
}
/*--------------------------------------------------------------*/
void music_ctrl() {
   int i;
   for (i = 0; i < 11; i++)
      switch_ad_bd(i);
}
