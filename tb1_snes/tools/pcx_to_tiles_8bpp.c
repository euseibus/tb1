/* Converts a 24-bit PCX file to 256-color SNES background tiles */

#include <stdio.h>  /* For FILE I/O */
#include <string.h> /* For strncmp */
#include <fcntl.h>  /* for open()  */
#include <unistd.h> /* for lseek() */
#include <sys/stat.h> /* for file modes */
#include <stdlib.h>  /* exit() */

/* Convert to 15-bpp bgr */
static int rgb2bgr(int r,int g, int b) {
  int r2,g2,b2,bgr;

  r2=(r>>3)&0x1f;
  g2=(g>>3)&0x1f;
  b2=(b>>3)&0x1f;

  bgr=(b2<<10)|(g2<<5)|r2;

  return bgr;
}

static char symbol_name[BUFSIZ]="temp";

/* File already open */
static int vmwLoadPCX(int pcx_fd) {

   int debug=1,bpp;
   int x,y;
   int i,numacross,planes=0;
   int xsize,ysize,plane;
   int xmin,ymin,xmax,ymax,version;
   unsigned char pcx_header[128];
   unsigned char temp_byte;

    /*************** DECODE THE HEADER *************************/

    read(pcx_fd,&pcx_header,128);

    xmin=(pcx_header[5]<<8)+pcx_header[4];
    ymin=(pcx_header[7]<<8)+pcx_header[6];

    xmax=(pcx_header[9]<<8)+pcx_header[8];
    ymax=(pcx_header[11]<<8)+pcx_header[10];

    version=pcx_header[1];
    bpp=pcx_header[3];

    if (debug) {

       fprintf(stderr,"Manufacturer: ");
       if (pcx_header[0]==10) fprintf(stderr,"Zsoft\n");
       else fprintf(stderr,"Unknown %i\n",pcx_header[0]);

       fprintf(stderr,"Version: ");

       switch(version) {
        case 0: fprintf(stderr,"2.5\n"); break;
        case 2: fprintf(stderr,"2.8 w palette\n"); break;
        case 3: fprintf(stderr,"2.8 w/o palette\n"); break;
        case 4: fprintf(stderr,"Paintbrush for Windows\n"); break;
        case 5: fprintf(stderr,"3.0+\n"); break;
        default: fprintf(stderr,"Unknown %i\n",version);
       }
       fprintf(stderr,"Encoding: ");
       if (pcx_header[2]==1) fprintf(stderr,"RLE\n");
       else fprintf(stderr,"Unknown %i\n",pcx_header[2]);

       fprintf(stderr,"BitsPerPixelPerPlane: %i\n",bpp);
       fprintf(stderr,"File goes from %i,%i to %i,%i\n",xmin,ymin,xmax,ymax);

       fprintf(stderr,"Horizontal DPI: %i\n",(pcx_header[13]<<8)+pcx_header[12]);
       fprintf(stderr,"Vertical   DPI: %i\n",(pcx_header[15]<<8)+pcx_header[14]);

       fprintf(stderr,"Number of colored planes: %i\n",pcx_header[65]);
       fprintf(stderr,"Bytes per line: %i\n",(pcx_header[67]<<8)+pcx_header[66]);
       fprintf(stderr,"Palette Type: %i\n",(pcx_header[69]<<8)+pcx_header[68]);
       fprintf(stderr,"Hscreen Size: %i\n",(pcx_header[71]<<8)+pcx_header[70]);
       fprintf(stderr,"Vscreen Size: %i\n",(pcx_header[73]<<8)+pcx_header[72]);

    }
    planes=pcx_header[65];

    xsize=((xmax-xmin)+1);
    ysize=((ymax-ymin)+1);

    char *output;

    output=calloc((xsize*ysize),sizeof(unsigned int));
    if (output==NULL) return -1;

   x=0; y=0;

   while(y<ysize) {
      for(plane=0;plane<planes;plane++) {
      x=0;
       while (x<xsize) {
          read(pcx_fd,&temp_byte,1);
          if (0xc0 == (temp_byte&0xc0)) {
	     numacross=temp_byte&0x3f;
	     read(pcx_fd,&temp_byte,1);
//	     fprintf(stderr,"%i pixels of %i\n",numacross,temp_byte);
	     for(i=0;i<numacross;i++) {
	       output[(y*xsize)+x]=temp_byte;
	       //	        printf("%x ",temp_byte);
		x++;
	     }
          }
          else {
//	     fprintf(stderr,"%i, %i Color=%i\n",x,y,temp_byte);
//	    printf("%x ",temp_byte);
	    output[(y*xsize)+x]=temp_byte;
	     x++;
          }
       }

      }
      y++;
   }

#define X_CHUNKSIZE 8
#define Y_CHUNKSIZE 8


   unsigned int plane0,plane1,plane2,plane3;
   unsigned int plane4,plane5,plane6,plane7,offset;

   printf("%s_tile_data:\n",symbol_name);
   int ychunk,xchunk;
   for(ychunk=0;ychunk<ysize/Y_CHUNKSIZE;ychunk++) {
      for(xchunk=0;xchunk<xsize/X_CHUNKSIZE;xchunk++) {

	printf("\t; Tile %d %d, Plane 0 Plane 1\n",xchunk,ychunk);

        for(y=0;y<Y_CHUNKSIZE;y++){
           plane0=0;plane1=0;
           for(x=0;x<X_CHUNKSIZE;x++) {
              plane0<<=1;
              plane1<<=1;
	      offset=((ychunk*Y_CHUNKSIZE+y)*xsize)+(xchunk*X_CHUNKSIZE)+x;
              plane0|=(output[offset])&1;
              plane1|=(((output[offset])&2)>>1);
	   }
           printf("\t.word $%02x%02x\n",plane1,plane0);
        }

        printf("\t; Plane 2 Plane 3\n");
        for(y=0;y<Y_CHUNKSIZE;y++){
           plane2=0;plane3=0;
           for(x=0;x<X_CHUNKSIZE;x++) {
              plane2<<=1;
              plane3<<=1;

	      offset=((ychunk*Y_CHUNKSIZE+y)*xsize)+(xchunk*X_CHUNKSIZE)+x;
              plane2|=(((output[offset])&4)>>2);
              plane3|=(((output[offset])&8)>>3);
	   }
	   printf("\t.word $%02x%02x\n",plane3,plane2);
	}

        printf("\t; Plane 4 Plane 5\n");
        for(y=0;y<Y_CHUNKSIZE;y++){
           plane4=0;plane5=0;
           for(x=0;x<X_CHUNKSIZE;x++) {
              plane4<<=1;
              plane5<<=1;

	      offset=((ychunk*Y_CHUNKSIZE+y)*xsize)+(xchunk*X_CHUNKSIZE)+x;
              plane4|=(((output[offset])&16)>>4);
              plane5|=(((output[offset])&32)>>5);
	   }
           printf("\t.word $%02x%02x\n",plane5,plane4);
        }

        printf("\t; Plane 6 Plane 7\n");
        for(y=0;y<Y_CHUNKSIZE;y++){
           plane6=0;plane7=0;
           for(x=0;x<X_CHUNKSIZE;x++) {
              plane6<<=1;
              plane7<<=1;

	      offset=((ychunk*Y_CHUNKSIZE+y)*xsize)+(xchunk*X_CHUNKSIZE)+x;
              plane6|=(((output[offset])&64)>>6);
              plane7|=(((output[offset])&128)>>7);
	   }
	   printf("\t.word $%02x%02x\n",plane7,plane6);
	}

      }
   }

   printf("%s_palette:\n",symbol_name);

   /* read in palette */
   read(pcx_fd,&temp_byte,1);
   if (temp_byte!=12) {
     fprintf(stderr,"Error!  No palette found!\n");
   }
   else {
     int r,g,b;
     for(i=0;i<256;i++) {
       read(pcx_fd,&temp_byte,1);
       r=temp_byte;
       read(pcx_fd,&temp_byte,1);
       g=temp_byte;
       read(pcx_fd,&temp_byte,1);
       b=temp_byte;
       printf("\t.word $%x\t; r=%x g=%x b=%x\n",rgb2bgr(r,g,b),r,g,b);
     }
   }

    return 0;
}


int main(int argc, char **argv) {

    int result;

    if (argc>1) {
       strncpy(symbol_name,argv[1],BUFSIZ);
    }

    /* read from stdin */

    result=vmwLoadPCX(fileno(stdin));

    if (result<0) {
       fprintf(stderr,"Error reading PCX from stdin\n");
       exit(1);
    }

    return 0;
}
