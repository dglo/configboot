/* this C file should have no
 * external dependencies.  i'm trying
 * to make these 1k lines or so as bug-
 * free as possible.
 *
 * we can exist in a very minimal environment,
 * no interrupts, no ptes, etc...
 */
#define NULL ((void*) 0)

#define DOM_PLD_BASE (0x50000000)
#define DOM_PLD_REBOOT_CONTROL   (DOM_PLD_BASE + 0x0000000e)
#define   DOM_PLD_REBOOT_CONTROL_INITIATE_REBOOT 0x01

#define DOM_PLD_BOOT_CONTROL     (DOM_PLD_BASE + 0x0000000f)
#define   DOM_PLD_BOOT_CONTROL_ALTERNATE_FLASH 0x01
#define   DOM_PLD_BOOT_CONTROL_BOOT_FROM_FLASH 0x02
#define   DOM_PLD_BOOT_CONTROL_NCONFIG         0x08

#define DOM_PLD_BOOT_STATUS      (DOM_PLD_BASE + 0x0000000f)
#define   DOM_PLD_BOOT_STATUS_ALTERNATE_FLASH 0x01
#define   DOM_PLD_BOOT_STATUS_BOOT_FROM_FLASH 0x02
#define   DOM_PLD_BOOT_STATUS_INIT_DONE       0x03
#define   DOM_PLD_BOOT_STATUS_NCONFIG         0x04


#define PLD(a) ( *(volatile unsigned char *) DOM_PLD_##a )
#define PLDBIT(a, b) (DOM_PLD_##a##_##b)
#define PLDBIT2(a, b, c) (DOM_PLD_##a##_##b | DOM_PLD_##a##_##c)

/* usage: RPLDBIT(BOOT_STATUS, ALTERNATE_FLASH) */
#define RPLDBIT(a, b) ( PLD(a) & PLDBIT(a, b) )
#define RPLDBIT2(a, b, c) ( PLD(a) & (PLDBIT(a, b) | PLDBIT(a, c)) )
#define RPLDBIT3(a, b, c, d) ( PLD(a) & (PLDBIT(a, b) | PLDBIT(a, c) | PLDBIT(a, d)) )

static int isSerialPower(void) {
   static int isInit = 0;
   static int ret = 0;
   
   if (!isInit) {
      ret = (*(volatile unsigned char *)0x5000000b) & 1;
      isInit=1;
   }
   return ret;
}

static void initComm(void) {
   if (isSerialPower()) {
      *(volatile unsigned *)0x7fffc2a8 = 3;
      *(volatile unsigned *)0x7fffc2ac = 0xc3;
      *(volatile unsigned *)0x7fffc294 = 0;
      *(volatile unsigned *)0x7fffc298 = 0;
      *(volatile unsigned *)0x7fffc2b4 = 0x2b;
      *(volatile unsigned *)0x7fffc2b8 = 0;
   }
}

#define FPGA(a) ( *(volatile unsigned *) DOM_FPGA_##a )
#define FPGABIT(a, b) (DOM_FPGA_##a##_##b)
#define RFPGABIT(a, b) ( FPGA(a) & FPGABIT(a, b) )
#define DOM_FPGA_BASE (0x90000000)
#define DOM_FPGA_TEST_BASE (DOM_FPGA_BASE + 0x00080000) 
#define DOM_FPGA_TEST_COM_RX_DATA (DOM_FPGA_TEST_BASE + 0x103C)
#define DOM_FPGA_TEST_COM_STATUS (DOM_FPGA_TEST_BASE + 0x1034)
#define DOM_FPGA_TEST_COM_STATUS_RX_MSG_READY     0x00000001
#define DOM_FPGA_TEST_COM_STATUS_TX_FIFO_ALMOST_FULL  0x00020000
#define DOM_FPGA_TEST_COM_CTRL (DOM_FPGA_TEST_BASE + 0x1030)
#define DOM_FPGA_TEST_COM_CTRL_RX_DONE     0x00000001
#define DOM_FPGA_TEST_COM_TX_DATA (DOM_FPGA_TEST_BASE + 0x1038)

static int getbyte(void) {
   
   if (isSerialPower()) {
      while ( ((*(volatile unsigned *) 0x7fffc280) & 0x1f) == 0) ;
      return *(volatile unsigned *) 0x7fffc288;
   }
   else {
      static int pending = 0;
      int ret = -1;

      while (pending==0) {
	 unsigned bytes[2];

	 /* wait for msg */
	 while (!RFPGABIT(TEST_COM_STATUS, RX_MSG_READY)) ;
	 
	 /* read type -- and drop it on the floor... */
	 bytes[0] = FPGA(TEST_COM_RX_DATA)&0xff;
	 bytes[1] = FPGA(TEST_COM_RX_DATA)&0xff;
	 
	 /* read length */
	 bytes[0] = FPGA(TEST_COM_RX_DATA)&0xff;
	 bytes[1] = FPGA(TEST_COM_RX_DATA)&0xff;
	 pending = (bytes[1]<<8) | bytes[0];
      }

      /* read next byte... */
      ret = FPGA(TEST_COM_RX_DATA)&0xff;
      pending--;

      if (pending==0) {
	 unsigned reg = FPGA(TEST_COM_CTRL);
	 FPGA(TEST_COM_CTRL) = reg | FPGABIT(TEST_COM_CTRL, RX_DONE);
	 FPGA(TEST_COM_CTRL) = reg & (~FPGABIT(TEST_COM_CTRL, RX_DONE));
      }
      return ret;
   }
   return 0;
}

static void putdor(const char *msg, int len) {
   int type = 0;
   int i;
   
   /* wait for Tx fifo almost full to be low */
   while (RFPGABIT(TEST_COM_STATUS, TX_FIFO_ALMOST_FULL)) ;
   
   /* send data */
   FPGA(TEST_COM_TX_DATA) = type&0xff; 
   FPGA(TEST_COM_TX_DATA) = (type>>8)&0xff;  
   FPGA(TEST_COM_TX_DATA) = len&0xff; 
   FPGA(TEST_COM_TX_DATA) = (len>>8)&0xff; 

   for (i=0; i<len; i++) FPGA(TEST_COM_TX_DATA) = msg[i];
}

static void putch(int c) {
   if (isSerialPower()) {
      while ( ((*(volatile unsigned *) 0x7fffc28c) & 0x1f) >= 15) ;
      *(volatile unsigned *) 0x7fffc290 = c;
   }
   else {
      char ch = (char) c;
      putdor(&ch, 1);
   }
}

static void putst(const char *p) {
   if (!isSerialPower()) {
      const char *pp = p;
      while (*pp) pp++;
      putdor(p, pp-p);
   }
   else {
      while (*p) { putch(*p); p++; }
   }
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

static int nerrors=0;
static void prtNumErrors(void) {
   putst("  "); 
   puti(nerrors); 
   putst(" flash programming errors\r\n");
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

static void setBootFlash(void) { 
   const unsigned char reg = PLD(BOOT_CONTROL);
   PLD(BOOT_CONTROL) = PLDBIT(BOOT_CONTROL, BOOT_FROM_FLASH) | reg;
}

static void clrBootFlash(void) { 
   const unsigned char reg = PLD(BOOT_CONTROL);
   PLD(BOOT_CONTROL) = ~PLDBIT(BOOT_CONTROL, BOOT_FROM_FLASH) & reg;
}

static int  isBootFlash(void)  { 
   return RPLDBIT(BOOT_CONTROL, BOOT_FROM_FLASH)!=0;
}

static void prtBootFlash(void) {
   putst("  Boot from ");
   if (isBootFlash()) {
      putst("flash\r\n");
   }
   else {
      putst("serial\r\n");
   }
}

static int swapFlash = 0;
static void setSwapFlash(void) { swapFlash = 1; }
static void clrSwapFlash(void) { swapFlash = 0; }
static int  isSwapFlash(void) { return swapFlash; }
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
   return -1;
}

static int commAvail(void) {
   return 0;
}

static void reboot(void) {
   /* we know fpga is loaded */
 
   /* if comm avail... */
   if (commAvail()) {
      /* request reboot */
      /* wait for reboot request... */
   }
   
   /* hit pld with reboot request */
   PLD(REBOOT_CONTROL) = PLDBIT(REBOOT_CONTROL, INITIATE_REBOOT);
   *(volatile unsigned char *)0x50000000 = 1;
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
   
   putst("configboot v2.7\r\n");
   putst("type ? for help\r\n");
   putst("# ");
   while (1) {
      int c;
      
      c = getbyte();
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
	    putst("rebooting...\r\n\r\n");
	    reboot();
	    /* doesn't return ... */
	 }
	 else if ('\r') {
	    putst("\r\n# ");
	 }
      }
      else if (state==ST_SCHIP) {
	 if (c=='0') { schip = 0; state = ST_ECHIP; }
	 if (c=='1') { schip = 1; state = ST_ECHIP; }
	 if (c=='\r' || c=='\n') {
	    programFlash(0, 1);
	    state=ST_READY;
	    putst("\r\n");
	    prtNumErrors();
	    putst("# ");
	 }
      }
      else if (state==ST_ECHIP) {
	 if (c=='0' && schip==0) {
	    programFlash(0, 0);
	    state = ST_READY; 
	    putst("\r\n");
	    prtNumErrors();
	    putst("# ");
	 }
	 if (c=='1') { 
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

/* block size in 16 bit words 
 */
static int blkSize(int blk) { return (blk<8) ? 4*1024 : 32*1024; }

/* convert address to chip...
 */
static int addrtochip(const void *a) {
   const short *addr = (const short *) a;
   const short *fs0 = (const short *) 0x41000000;
   const short *fe0 = (const short *) (0x41400000-2);
   const short *fs1 = (const short *) 0x41400000;
   const short *fe1 = (const short *) (0x41800000-2);

   if (addr>=fs0 && addr<=fe0)      { return 0; }
   else if (addr>=fs1 && addr<=fe1) { return 1; }
   return -1;
}

/* convert address to block location...
 */
static int addrtoblock(const void *a) {
   const short *addr = (const short *) a;
   const short *fs0 = (const short *) 0x41000000;
   const short *fs1 = (const short *) 0x41400000;
   const int chip = addrtochip(a);
   int idx, bblk;
   if (chip==0)      { idx = addr - fs0; }
   else if (chip==1) { idx = addr - fs1; }
   else { return -1; }
   bblk = idx/(32*1024);
   return (bblk==0) ? (idx/(4*1024)) : (bblk + 7);
}

/* turn chip/blk -> addr
 */
static void *blktoaddr(int chip, int blk) {
   short *fs0 = (short *) 0x41000000;
   short *fs1 = (short *) 0x41400000;
   short *addr;
   
   if (chip==0)      { addr = fs0; }
   else if (chip==1) { addr = fs1; }
   else { return NULL; }

   return addr + ( (blk<8) ? (blk*4*1024) : ( (blk-7)*32*1024) );
}

static void *flash_chip_addr(int chip) {
   return (void *) ((chip==0) ? 0x41000000 : 0x41400000);
}

static int unlock_chip(int chip) {
   const int sblk = addrtoblock(flash_chip_addr(chip));
   const int eblk = addrtoblock((char *)flash_chip_addr(chip) + 0x04000000);
   volatile short *flash;
   int i;
   
   flash = blktoaddr(chip, 0);
   *flash = 0x50; /* clear status register... */
   
   for (i=sblk; i<=eblk; i++) {
      volatile short *block = blktoaddr(chip, i);
      *flash = 0x60;
      *block = 0xd0;
   }

   /* now confirm lock...
    */
   *flash = 0x90;
   for (i=sblk; i<=eblk; i++) {
      volatile short *block = blktoaddr(chip, i);
      const unsigned short v = *(block+2);
      if ( (v&1) != 0 ) break; 
   }
   
   /* back to read array mode... */
   *flash = 0xff;

   /* send back confirmation...
    */
   return (i<=eblk) ? 1 : 0;
}

static int lock_chip(int chip) {
   const int sblk = 0;
   const int eblk = addrtoblock((char *)flash_chip_addr(chip) + 0x04000000);
   volatile short *flash;
   int i;
   
   flash = (volatile short *) blktoaddr(chip, 0);
   *flash = 0x50; /* clear status register... */
   
   for (i=sblk; i<=eblk; i++) {
      volatile short *block = blktoaddr(chip, i);
      *flash = 0x60;
      *block = 0x01;
   }

   /* now confirm lock...
    */
   *flash = 0x90;
   for (i=sblk; i<=eblk; i++) {
      volatile short *block = blktoaddr(chip, i);
      const unsigned short v = *(block+2);
      if ((v&1) != 1) {
	 break;
      }
   }
   
   /* back to read array mode... */
   *flash = 0xff;

   /* send back confirmation...
    */
   return (i<=eblk) ? 1 : 0;
}

static int erase_chip(int chip) {
   const int sblk = addrtoblock(flash_chip_addr(chip));
   const int eblk = addrtoblock((char *)flash_chip_addr(chip) + 0x04000000);
   volatile short *flash;
   int i;

   flash = blktoaddr(chip, 0);
   *flash = 0x50; /* clear status register... */

   for (i=sblk; i<=eblk; i++) {
      volatile short *block = blktoaddr(chip, i);
      unsigned short sr;

      *block = 0x20;
      *block = 0xd0;
      
      while (1) {
	 sr = *block;
	 if (sr&0x80) break;
      }

      /* error? */
      if (sr&(1<<5)) { 
	 if (sr&(1<<3)) {
	    putst("flash: error: vpp range error!\r\n");
	    break;
	 }
	 else if ( (sr&(3<<4)) == (3<<4)) {
	    putst("flash: error: command sequence error!\r\n");
	    break;
	 }
	 else if (sr&2) {
	    putst("flash: error: attempt to erase locked block!\r\n");
	    break;
	 }
	 else {
	    putst("flash: error: unknown error: ");
	    puti(sr);
	    putst("\r\n");
	    break;
	 }
      }
   }

   /* back to read array mode... 
    */
   flash = blktoaddr(chip, 0);
   *flash = 0xff;

   /* send back confirmation...
    */
   for (i=sblk; i<=eblk; i++) {
      volatile unsigned short *block = blktoaddr(chip, i);
      int j;
      const int sz = blkSize(i);
      int err = 0;

      for (j=0; j<sz && !err; j++)
	 if (block[j]!=0xffff) 
	    nerrors++;
      if (err) break;
   }
   
   return (i<=eblk) ? 1 : 0;
}

static int feq(const short *p1, const short *p2, int n) {
	int i;
	for (i=0; i<(n+1)/2; i++) if (p1[i]!=p2[i]) return 1;
	return 0;
}

static int flash_write(void *to, const void *mem, int cnt) {
   volatile short *addr = (volatile short *) to;
   const short *ptr = (const short *) mem;
   const int sblk = addrtoblock(to);
   const int eblk = addrtoblock((const char *)to+cnt-2);
   const int chip = addrtochip(to);
   volatile short *flash;
   int i;
   
   if (chip<0 || sblk<0 || eblk<0 || eblk<sblk) return 1;

   if (chip!=addrtochip((const char *)to + cnt - 2)) return 1;

   flash = blktoaddr(chip, 0);
   *flash = 0x50; /* clear status register... */

   for (i=0; i<(cnt+1)/2; i++) {
      unsigned short v;
      *flash = 0x40;
      addr[i] = ptr[i];
      while (1) {
	 v = *(volatile short *)flash;
	 if (v&0x80) break;
      }
   }

   /* back to read array mode... 
    */
   *flash = 0xff;

   /* send back confirmation...
    */
   return (feq(to, mem, cnt)==0) ? 0 : 1;
}

static void programFlash(int schip, int echip) {
   void *fs, *fe;
   int ck=0, cksum=0;
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
   chip_end[1] = (unsigned) fe;
   fs = (void *)chip_start[schip];

   for (i=schip; i<=echip; i++) {
      /* unlock all data
       */
      putst("unlock chip: "); 
      puti(i);
      putst("... ");
      if (unlock_chip(i)) nerrors++;
      
      /* erase all data -- except for iceboot...
       */
      putst("erase chip: "); 
      puti(i);
      putst("... ");
      if (erase_chip(i)) nerrors++;
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
	    
	    cksum+=addr;
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
	    cksum += offset&0xff;
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
		  if (flash_write(fs + offset + addr, data, ndata)) nerrors++;
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
	    
	    if (ck!=0x100 - (cksum&0xff)) nerrors++;
	    
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
      if (lock_chip(i)) nerrors++;
   }
   putst("\r\n");
}

/* interrupt handlers... */
void CAbtHandler(void) {}
void CUdefHandler(void){}
void CSwiHandler(void){}
void CIrqHandler(void){}
void CPabtHandler(void){}
void CDabtHandler(void){}
void CFiqHandler(void){}

void *memset(void *p, int c, unsigned n) {
	char *cp = (char *) p;
	int i;
	for (i=0; i<n; i++) cp[i] = c;
	return p;
}
