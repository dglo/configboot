#include <stdio.h>

static int nerrors=0;

static int getline(char *p, int n) {
   int i=0;
   char *t = p;
   
   memset(p, 0, n);
   if (fgets(p, n, stdin)==NULL) return 0;

   /* clean initial and final whitespace... */
   n = strlen(p);
   while (*t && (*t==' ' || *t=='\t' || *t=='\r' || *t=='\n')) {
      t++;
      i++;
   }
   n-=i;

   /* poor man's memmove.. */
   if (i>0) {
      int j;
      char *t2 = p;
      for (j=i; j<n+1; j++, t2++, t++) *t2 = *t;
   }

   /* get rid of trailing whitespace... */
   for (i=n; i>=0; i--) {
      if (p[i]==' ' || p[i]=='\t' || p[i]=='\r' || p[i]=='\n') {
	 n--;
	 continue;
      }
      break;
   }

   p[n]=0;
   return n;
}

static void putst(const char *p) {
   printf("%s", p); fflush(stdout);
}

static void prtCmds(void) {
   putst("Commands:\n");
   putst("  r               : reboot\n");
   putst("  d               : display current parameters\n");
   putst("  f               : set boot from flash mode\n");
   putst("  s               : set boot from serial mode\n");
   putst("  b               : set swap flash A and B mode\n");
   putst("  a               : set canonical flash A and B mode\n");
   putst("  p [saddr eaddr] : program flash starting at saddr (hex) to"
	  " eaddr (hex)\n");
   putst("  ?               : show commands\n");
}

int main(int argc, char *argv[]) {
   int flash = 1;
   int swap = 0;
   
   /* FIXME: default to boot from flash */

   putst("configboot v2.7\n");
   putst("type ? for help\n");
   while (1) {
      char line[128];
      int nr;
      
      putst("# ");
      
      nr = getline(line, sizeof(line));
      if (nr>0) {
	 if (line[0]=='?') {
	    prtCmds();
	 }
	 else if (line[0]=='d') {
	    putst("Parameters:\n");
	    
	    putst("  Boot from ");
	    if (flash) {
	       putst("flash\n");
	    }
	    else {
	       putst("serial\n");
	    }
	    
	    if (!swap) {
	       putst("  Don't s");
	    }
	    else putst("  S");
	    
	    putst("wap flash A and B\n");

	    putst("  0 flash programming errors\n");
	 }
	 else if (line[0]=='f') {
	    flash=1;
	 }
	 else if (line[0]=='s') {
	    flash=0;
	 }
	 else if (line[0]=='b') {
	    swap=1;
	 }
	 else if (line[0]=='a') {
	    swap=0;
	 }
	 else if (line[0]=='p') {
	    putst("unlocking... ");
	    putst("erasing... ");
	    putst("programming... ");
	    putst("locking... ");
	    putst("done\n");
	 }
	 else if (line[0]=='r') {
	    break;
	 }
	 else {
	    putst("unknown command\n");
	 }
      }
   }
   return 0;
}







