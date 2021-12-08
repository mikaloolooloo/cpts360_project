// write_cp

#include "header.h"

extern int dev;
extern PROC   proc[NPROC], *running;

void zero_out(int dev, int blk)
{
    char buf[BLKSIZE];
    bzero(buf, BLKSIZE);
    put_block(dev, blk, buf);
}

int mywrite(int fd, char buf[ ], int nbytes) 
{
    OFT * oftp = running->fd[fd];
    MINODE * mip = oftp->minodePtr;
    int blk, n = nbytes;
    int ibuf[BLKSIZE] = { 0 };

    while (nbytes > 0 ){
    // compute LOGICAL BLOCK (lbk) and the startByte in that lbk:
        int lbk       = oftp->offset / BLKSIZE;
        int startByte = oftp->offset % BLKSIZE;

    // I only show how to write DIRECT data blocks, you figure out how to 
    // write indirect and double-indirect blocks.
        if (lbk < 12){                         // direct block
            if (mip->INODE.i_block[lbk] == 0) {   // if no data block yet
                mip->INODE.i_block[lbk] = balloc(mip->dev);// MUST ALLOCATE a block
            }
            blk = mip->INODE.i_block[lbk];      // blk should be a disk block now
        }
        else if (lbk >= 12 && lbk < 256 + 12){ // INDIRECT blocks 
              // HELP INFO:
            if (mip->INODE.i_block[12] == 0){
                //allocate a block for it;
                mip->INODE.i_block[12] = balloc(mip->dev);
                //zero out the block on disk !!!!
                zero_out(mip->dev, mip->INODE.i_block[12]);
            }

            get_block(mip->dev, mip->INODE.i_block[12], (char *)ibuf);
              // get i_block[12] into an int ibuf[256];
            blk = ibuf[lbk - 12];
            if (blk==0){
                //allocate a disk block;
                blk = ibuf[lbk - 12] = balloc(mip->dev);
                if (blk==0) {
                    printf("no more disk space\n");
                    return 0;
                }
                //record it in i_block[12];
                put_block(mip->dev, mip->INODE.i_block[12], (char *)ibuf);
            }

            
            //.......
        }
        else {
            // double indirect blocks */
            // using mailman's algorithm
            int indirect_blk = (lbk - 256 - 12) / 256;
			int indirect_off = (lbk - 256 - 12) % 256;

            if (mip->INODE.i_block[13] == 0){
                mip->INODE.i_block[13] = balloc(mip->dev);
                zero_out(mip->dev, mip->INODE.i_block[13]);
            }

            get_block(mip->dev, mip->INODE.i_block[13], (char *)ibuf);

            if (ibuf[indirect_blk] == 0) // if there's no data blocks
            {
                ibuf[indirect_blk] = balloc(mip->dev);
                if (ibuf[indirect_blk]==0) {
                    printf("no more disk space\n");
                    return 0;
                }
                zero_out(mip->dev, ibuf[indirect_blk]);
                put_block(mip->dev, mip->INODE.i_block[13], (char *)ibuf);
            }

            int ibuf2[BLKSIZE] = { 0 };
            get_block(mip->dev, ibuf[indirect_blk], (char *)ibuf2);

            if (ibuf2[indirect_off] == 0) // if there's no data blocks
            {
                ibuf2[indirect_off] = balloc(mip->dev);
                if (ibuf2[indirect_off]==0) {
                    printf("no more disk space\n");
                    return 0;
                }
                zero_out(mip->dev, ibuf2[indirect_off]);
                put_block(mip->dev, ibuf[indirect_blk], (char *)ibuf2);
            }

            blk = ibuf2[indirect_off];
        }

        /*
        OPTIMIZATION OF write Code
        
        As in read(), the below inner while(remain > 0) loop can be optimized:
        Instead of copying one byte at a time and update the control variables on each 
        byte, TRY to copy only ONCE and adjust the control variables accordingly.

        REQUIRED: Optimize the write() code in your project.
        */

     /* all cases come to here : write to the data block */
        char wbuf[BLKSIZE] = { 0 };
        get_block(mip->dev, blk, wbuf);   // read disk block into wbuf[ ]  
        char *cp = wbuf + startByte;      // cp points at startByte in wbuf[]
        int remain = BLKSIZE - startByte;     // number of BYTEs remain in this block
        char * cq = buf; // cq points at buf [ ] 

        if (remain < nbytes) nbytes = remain;

        memmove(cp, cq, nbytes);
        cp += nbytes; cq += nbytes;
        oftp->offset += nbytes;
        nbytes -= nbytes;

        if (oftp->offset > mip->INODE.i_size)
            mip->INODE.i_size = oftp->offset;
        
        /*while (remain > 0){               // write as much as remain allows  
            *cp++ = *cq++;              // cq points at buf[ ]
            nbytes--; remain--;         // dec counts
            oftp->offset++;             // advance offset
            if (oftp->offset > mip->INODE.i_size)  // especially for RW|APPEND mode
                mip->INODE.i_size++;    // inc file size (if offset > fileSize)
            if (nbytes <= 0) break;     // if already nbytes, break
        }*/
        put_block(mip->dev, blk, wbuf);   // write wbuf[ ] to disk
     
     // loop back to outer while to write more .... until nbytes are written
  }

  mip->dirty = 1;       // mark mip dirty for iput() 
  //printf("wrote %d char into file descriptor fd=%d\n", n, fd);          
  return nbytes;
}

int write_file()
{
  // 1. Preprations:
     //ask for a fd   and   a text string to write;
    int fd = 0, nbytes = 0;
    char buf[BLKSIZE] = { 0 };

  // 2. verify fd is indeed opened for WR or RW or APPEND mode

  // 3. copy the text string into a buf[] and get its length as nbytes.

     return(mywrite(fd, buf, nbytes));
}

int my_mv(char * src, char * dest)
{
    // move file
    // check for valid pathname
    if (validPathname(src) == -1) {
      printf("\nfirst pathname is not valid: mv failed\n");
      return -1;
    }
    if (validPathname(dest) == -1) {
      printf("\nsecond pathname is not valid: mv failed\n");
      return -1;
    }

    // verify src exists; get its INODE in ==> you already know its dev
    // 2. check whether src is on the same dev as src
    int sino = getino(src);
    if (sino == -1) {
        printf("\n src file %s does not exist: mv failed\n", src);
        return -1;
    }
    else printf("src file %s exists: passed existence check\n", src);

    if (1) { // change this if to compare devs ?? not sure
             // CASE 1: same dev:
        //3. Hard link dst with src (i.e. same INODE number)
        my_link(src, dest);
        //4. unlink src (i.e. rm src name from its parent directory and reduce INODE's link count by 1).
        my_unlink(src);
    }
    else {
              //CASE 2: not the same dev:
        //3. cp src to dst
        my_cp(src, dest);
        //4. unlink src
        my_unlink(src);
    }
}

int my_cp(char * src, char * dest)
{
     // check for valid pathname
    if (validPathname(src) == -1) {
      printf("\nfirst pathname is not valid: cp failed\n");
      return -1;
    }
    if (validPathname(dest) == -1) {
      printf("\nsecond pathname is not valid: cp failed\n");
      return -1;
    }

    // open both files -- open_file should creat a file if it does not currently exist
    int fd = open_file(src, 0); // R = 0
    int gd = open_file(dest, 1); // W = 1

    // check to see if both files opened correctly
    if (fd == -1) {
        printf("open failure\n");
        close_file(fd); close_file(gd);
        printf("\nsrc file %s did not open correctly as fd is %d: cp failed\n", src, fd);
        return -1;
    }
    else printf("src file %s opened correctly with fd %d: src open check passed\n", src, fd);
    if (gd == -1) { 
        printf("open failure\n");
        close_file(fd); close_file(gd);
        printf("\ndest file %s did not open correctly as fd is %d: cp failed\n", dest, gd);
        return -1;
    }
    else printf("dest file %s opened correctly with fd %d: dest open check passed\n", dest, gd);

    char buf[BLKSIZE];
    bzero(buf, BLKSIZE);

    // copy one file
    int n = 0;
    while(n=myread(fd, buf, BLKSIZE) ){
       mywrite(gd, buf, n);  // notice the n in write()
    }

    close_file(fd); close_file(gd);
    printf("\ncp successful\n");
}