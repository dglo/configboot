/* we exist in a very minimal environment,
 * no interrupts, no ptes, etc...
 */
#define NULL ((void*) 0)

#include "hal/DOM_MB_hal.h"

/* FIXME: this should be factored out into hal... */
#include "../iceboot/flashdrv.c"

static int getbyte(void) {
   static unsigned char buffer[4096];
   static int bi=0, nb=0;
   
   /* no mas? */
   if (bi==nb) {
      int type;
      hal_FPGA_TEST_receive(&type, &nb, buffer);
      bi = 0;
   }

   {  int ret = buffer[bi];
      bi++;
      return ret;
   }
}

static int ndorerrors = 0;

static void putdor(const char *msg, int len) {
   if (len==0) {
      ndorerrors++;
      return;
   }

   hal_FPGA_TEST_send(0, len, msg);
}

static void putch(int c) {
   char ch = (char) c;
   putdor(&ch, 1);
}

static void putst(const char *p) {
   int n = 0;
   while (p[n]!=0) n++;
   putdor(p, n);
}

static void puti(unsigned i) {
   int nonzero = 0;
   unsigned sdig = 1000000000;

   while (sdig>0) {
      int dig = i/sdig;
      i -= dig*sdig;
      if (dig>0 || nonzero || sdig==1) putch(0x30 + dig);
      sdig/=10;
   }
}

static int nUnlockErrors=0;
static int nLockErrors=0;
static int nEraseErrors=0;
static int nWriteErrors=0;
static int nCksumErrors=0;
static int nParseErrors=0;

static void prtErrors(int nerrors, const char *msg) {
   putst("  "); 
   puti(nerrors);
   putst(" flash ");
   putst(msg);
   putst(" errors\r\n");
}

static void prtNumErrors(void) {
   prtErrors(nUnlockErrors, "unlock");
   prtErrors(nLockErrors, "lock");
   prtErrors(nEraseErrors, "erase");
   prtErrors(nWriteErrors, "write");
   prtErrors(nCksumErrors, "checksum");
   prtErrors(nParseErrors, "parse");
}

static void programFlash(int schip, int echip);

static void prtCmds(void) {
   putst("\r\n");
   putst("Commands:\r\n");
   putst("  r               : reboot\r\n");
   putst("  d               : display current parameters\r\n");
   putst("  f               : set boot from flash mode\r\n");
   putst("  s               : set boot from serial mode\r\n");
   putst("  b               : set swap flash A and B mode\r\n");
   putst("  a               : set canonical flash A and B mode\r\n");
   putst("  p [schip echip] : program flash starting at schip (0 or 1) to"
	  " echip (0 or 1)\r\n");
   putst("  ?               : show commands\r\n");
}

static inline void setBootFlash(void) { halSetFlashBoot(); }
static inline void clrBootFlash(void) { halClrFlashBoot(); }
static inline int isBootFlash(void)  { return halFlashBootState(); }

static void prtBootFlash(void) {
   putst("  Boot from ");
   if (isBootFlash()) {
      putst("flash\r\n");
   }
   else {
      putst("serial\r\n");
   }
}

static inline void setSwapFlash(void) { halSetSwapFlashChips(); }
static inline void clrSwapFlash(void) { halClrSwapFlashChips(); }
static inline int  isSwapFlash(void) { return halSwapFlashChipsState(); }

static void prtSwapFlash(void) {
   if (!isSwapFlash()) {
      putst("  Don't s");
   }
   else putst("  S");
   
   putst("wap flash A and B\r\n");
}

static int nibble(char c) {
   if (c>='0' && c<='9') return c - '0';
   if (c>='a' && c<='f') return c - 'a' + 10;
   if (c>='A' && c<='F') return c - 'A' + 10;
   nParseErrors++;
   return -1;
}

static void reboot(void) { halBoardReboot(); }

static void initComm(void) {
   /* wait for comm avail... */
   while (!hal_FPGA_TEST_is_comm_avail()) ;
}

int memcmp(const void *s1, const void *s2, unsigned n) {
   const unsigned char *c1 = (const unsigned char *) s1;
   const unsigned char *c2 = (const unsigned char *) s2;
   unsigned i;
   
   for (i=0; i<n; i++, c1++, c2++) {
      if (*c1<*c2) return -1;
      if (*c1>*c2) return 1;
   }
   return 0;
}

void *memcpy(void *dest, const void *src, unsigned n) {
   unsigned char *cdest = (unsigned char *) dest;
   const unsigned char *csrc = (const unsigned char *) src;
   unsigned i;
   for (i=0; i<n; i++, cdest++, csrc++) *cdest = *csrc;
   return dest;
}

int main(int argc, char *argv[]) {
   int schip = 0;
   enum {
      ST_READY,
      ST_SCHIP,
      ST_ECHIP,
   } state = ST_READY;

   /* initialize comm... */
   initComm();

   /* default to boot from flash... */
   setBootFlash();
   
   putst("\r\nconfigboot v2.7\r\n");
   putst("type ? for help\r\n");
   putst("# ");

#if 0
   putst("stack: ");
   puti((int)&schip);
   putst("\r\n");
#endif

   while (1) {
      const int c = getbyte();

      if (c>=0x20 && c<=0x7f) putch(c);

      if (state==ST_READY) {
	 if (c=='?') { 
	    prtCmds();
	    putst("# ");
	 }
	 else if (c=='d') {
	    putst("\r\nParameters:\r\n");
	    prtBootFlash();
	    prtSwapFlash();
	    prtNumErrors();

	    putst("# ");
	 }
	 else if (c=='f') {
	    setBootFlash();
	    putst("\r\n");
	    prtBootFlash();
	    putst("# ");
	 }
	 else if (c=='s') {
	    clrBootFlash();
	    putst("\r\n");
	    prtBootFlash();
	    putst("# ");
	 }
	 else if (c=='b') {
	    setSwapFlash();
	    putst("\r\n");
	    prtSwapFlash();
	    putst("# ");
	 }
	 else if (c=='a') {
	    clrSwapFlash();
	    putst("\r\n");
	    prtSwapFlash();
	    putst("# ");
	 }
	 else if (c=='p') {
	    state = ST_SCHIP;
	 }
	 else if (c=='r') {
            putst("\r\n");
            {  /* wait a bit for tx to drain... */
               volatile int i;
               for (i=0; i<1000000; i++) ;
            }
	    reboot();
	    /* doesn't return ... */
	 }
	 else if ('\r') {
	    putst("\r\n# ");
	 }
      }
      else if (state==ST_SCHIP) {
	 if (c=='0')      { schip = 0; state = ST_ECHIP; }
	 else if (c=='1') { schip = 1; state = ST_ECHIP; }
	 else if (c=='\r' || c=='\n') {
            putst("\r\n");
	    programFlash(0, 1);
	    state=ST_READY;
	    putst("\r\n");
	    prtNumErrors();
	    putst("# ");
	 }
      }
      else if (state==ST_ECHIP) {
	 if (c=='0' && schip==0) {
            putst("\r\n");
	    programFlash(0, 0);
	    state = ST_READY; 
	    putst("\r\n");
	    prtNumErrors();
	    putst("# ");
	 }
	 if (c=='1') { 
            putst("\r\n");
	    programFlash(schip, 1);
	    state = ST_READY; 
	    putst("\r\n");
	    prtNumErrors();
	    putst("# ");
	 }
      }
   }

   return 0;
}

static int erase_chip(int chip) {
   if (chip<0 || chip>1) return 1;
   return flash_erase(flash_chip_addr(chip), 0x00400000);
}

static int unlock_chip(int chip) {
   if (chip<0 || chip>1) return 1;
   return flash_unlock(flash_chip_addr(chip), 0x00400000);
}

static int lock_chip(int chip) {
   if (chip<0 || chip>1) return 1;
   return flash_unlock(flash_chip_addr(chip), 0x00400000);
}

static void programFlash(int schip, int echip) {
   void *fs;
   signed char ck=0, cksum=0;
   int allDone = 0;
   int len = 0;
   int nb = 0;
   int offset = 0;
   int ndata = 0;
   int type = 0;
   int addr = 0;
   int shift = 0;
   unsigned char data[128];
   int i;
   unsigned chip_start[2], chip_end[2];
   enum {
      ST_START, ST_LEN0, ST_LEN1, /* 0, 1, 2 */
      ST_ADDR0, ST_ADDR1, ST_ADDR2, ST_ADDR3, /* 3, 4, 5, 6 */
      ST_TYPE0, ST_TYPE1, /* 7, 8 */
      ST_DATA, /* 9, 10 */
      ST_CK0, ST_CK1, /* 11, 12 */
      ST_OFFSET0, ST_OFFSET1, ST_OFFSET2, ST_OFFSET3 /* 13, 14, 15, 16, 17 */
   } state = ST_START;

   chip_start[0] = (unsigned) flash_chip_addr(0);
   chip_start[1] = (unsigned) flash_chip_addr(1);
   chip_end[0] = chip_start[1]-1;
   chip_end[1] = (unsigned) (0x41000000 + 0x00800000 - 1);
   fs = (void *)chip_start[schip];

   for (i=schip; i<=echip; i++) {
      /* unlock all data
       */
      putst("unlock chip: "); 
      puti(i);
      putst("... ");
      if (unlock_chip(i)) nUnlockErrors++;
      
      /* erase all data...
       */
      putst("erase chip: "); 
      puti(i);
      putst("... ");
      if (erase_chip(i)) nEraseErrors++;
   }
   putst("\r\nReady...");
   
   while (!allDone) {
      char c = getbyte();

      if (state==ST_START) {
	 if (c==':') {
	    ck=0;
	    cksum=0;
	    state = ST_LEN0;
	 }
      }
      else if (state==ST_LEN0) {
	 int n = nibble(c);
	 if (n>=0) {
	    len = n;
	    state=ST_LEN1;
	 }
      }
      else if (state==ST_LEN1) {
	 int n = nibble(c);
	 if (n>=0) {
	    len<<=4;
	    len+=n;
	    cksum=len;
	    addr = 0;
	    state=ST_ADDR0;
	 }
      }
      else if (state==ST_ADDR0) {
	 int n = nibble(c);
	 if (n>=0) {
	    addr = n;
	    state = ST_ADDR1;
	 }
      }
      else if (state==ST_ADDR1) {
	 int n = nibble(c);
	 if (n>=0) {
	    addr <<= 4;
	    addr += n;
	    cksum += addr;
	    state = ST_ADDR2;
	 }
      }
      else if (state==ST_ADDR2) {
	 int n = nibble(c);
	 if (n>=0) {
	    addr <<= 4;
	    addr += n;
	    state = ST_ADDR3;
	 }
      }
      else if (state==ST_ADDR3) {
	 int n = nibble(c);
	 if (n>=0) {
	    addr <<= 4;
	    addr += n;
	    cksum+= (addr&0xff);
	    type = 0;
	    state = ST_TYPE0;
	 }
      }
      else if (state==ST_TYPE0) {
	 int n = nibble(c);
	 if (n==0) {
	    state = ST_TYPE1;
	 }
      }
      else if (state==ST_TYPE1) {
	 int n = nibble(c);
	 if (n==0) {
	    ndata = 0;
	    nb = 0;
	    state = ST_DATA;
	 }
	 else if (n==1) {
	    /* last record! */
	    allDone = 1;
	    cksum+=1;
	    state = ST_CK0;
	 }
	 else if (n==2) {
	    offset = 0;
	    cksum += 2;
	    shift = 4;
	    state = ST_OFFSET0;
	 }
	 else if (n==4) {
	    offset = 0;
	    cksum += 4;
	    shift = 16;
	    state = ST_OFFSET0;
	 }
      }
      else if (state==ST_OFFSET0) {
	 int n = nibble(c);
	 if (n>=0) {
	    offset = n;
	    state = ST_OFFSET1;
	 }
      }
      else if (state==ST_OFFSET1) {
	 int n = nibble(c);
	 if (n>=0) {
	    offset<<=4;
	    offset += n;
	    cksum += offset;
	    state = ST_OFFSET2;
	 }
      }
      else if (state==ST_OFFSET2) {
	 int n = nibble(c);
	 if (n>=0) {
	    offset <<= 4;
	    offset += n;
	    state = ST_OFFSET3;
	 }
      }
      else if (state==ST_OFFSET3) {
	 int n = nibble(c);
	 if (n>=0) {
	    offset <<= 4;
	    offset += n;
	    ck = 0;
	    cksum += (offset&0xff);
	    offset <<= shift;
	    state = ST_CK0;
	 }
      }
      else if (state==ST_DATA) {
	 int n = nibble(c);
	 if (n>=0) {
	    if (nb==0) {
	       data[ndata] = n;
	       nb++;
	    }
	    else {
	       data[ndata]<<=4;
	       data[ndata]+=n;
	       cksum+=data[ndata];
	       ndata++;
	       if (ndata==len) {
		  ck = 0;
		  state = ST_CK0;
		  /* now program the data... 
		   */
		  if (flash_write(fs + offset + addr, data, ndata)) 
                     nWriteErrors++;
	       }
	       nb=0;
	    }
	 }
      }
      else if (state==ST_CK0) {
	 int n = nibble(c);
	 if (n>=0) {
	    ck = n;
	    state = ST_CK1;
	 }
      }
      else if (state==ST_CK1) {
	 int n = nibble(c);
	 if (n>=0) {
	    ck <<= 4;
	    ck += n;
            cksum += ck;
            
            if ( cksum != 0 ) nCksumErrors++;
            
	    if (allDone) break;
	    else {
	       cksum = 0;
	       state = ST_START;
	    }
	 }
      }
   }

   for (i=schip; i<=echip; i++) {
      putst("lock chip "); puti(i); putst("... ");
      if (lock_chip(i)) nLockErrors++;
   }
   putst("\r\n");
}




