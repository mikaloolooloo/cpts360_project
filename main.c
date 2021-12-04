/****************************************************************************
*                   KCW: mount root file system                             *
*****************************************************************************/
#include "header.h"

extern MINODE *iget();

MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;

char gpath[128]; // global for tokenized components
char *name[64];  // assume at most 64 components in pathname
int   n;         // number of component strings

int fd, dev;
int nblocks, ninodes, bmap, imap, iblk;
char line[128], cmd[32], pathname[128], pathname2[128];

int init()
{
  int i, j;
  MINODE *mip;
  PROC   *p;

  printf("init()\n");

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    p->pid = i;
    p->uid = p->gid = 0;
    p->cwd = 0;
    for (j = 0; j<NFD; j++){
      p->fd[j] = 0;
    }
  }
}

// load root INODE and set root pointer to it
int mount_root()
{  
  printf("mount_root()\n");
  root = iget(dev, 2);
}

void my_menu();

char *disk = "diskimage";
int main(int argc, char *argv[ ])
{
  int ino;
  char buf[BLKSIZE];
  if (argc > 1) disk = argv[1];

  printf("checking EXT2 FS ....");
  if ((fd = open(disk, O_RDWR)) < 0){
    printf("open %s failed\n", disk);
    exit(1);
  }

  dev = fd;    // global dev same as this fd   

  /********** read super block  ****************/
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;

  /* verify it's an ext2 file system ***********/
  if (sp->s_magic != 0xEF53){
      printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
      exit(1);
  }     
  printf("EXT2 FS OK\n");
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;

  get_block(dev, 2, buf); 
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  iblk = gp->bg_inode_table;
  printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, iblk);

  init();  
  mount_root();
  printf("root refCount = %d\n", root->refCount);

  printf("creating P0 as running process\n");
  running = &proc[0];
  running->status = READY;
  running->cwd = iget(dev, 2);
  printf("root refCount = %d\n", root->refCount);

  // WRTIE code here to create P1 as a USER process
  printf("\n************************\n");
  while(1){
    printf("\n[menu|ls|cd|pwd|mkdir|creat|rmdir|link|unlink|symlink|cat|cp|quit]\ninput command :  ");
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;

    if (line[0]==0)
       continue;
    pathname[0] = 0;

    sscanf(line, "%s %s", cmd, pathname);
    printf("\ncmd=%s pathname=%s\n", cmd, pathname);
   
    if (strcmp(cmd, "menu")==0)
       my_menu();
    else if (strcmp(cmd, "ls")==0)
       my_ls(pathname);
    else if (strcmp(cmd, "cd")==0)
       my_cd(pathname);
    else if (strcmp(cmd, "pwd")==0)
       my_pwd(running->cwd);
    else if (strcmp(cmd, "mkdir")==0)
       my_mkdir(pathname);
    else if (strcmp(cmd, "creat")==0)
       my_creat(pathname);
    else if (strcmp(cmd, "rmdir")==0)
       my_rmdir(pathname);
    else if (strcmp(cmd, "link")==0) {
       sscanf(line, "%s %s %s", cmd, pathname, pathname2);
       my_link(pathname, pathname2); }
    else if (strcmp(cmd, "unlink")==0)
       my_unlink(pathname);
    else if (strcmp(cmd, "symlink")==0) {
       sscanf(line, "%s %s %s", cmd, pathname, pathname2);
       my_symlink(pathname, pathname2); }
    else if (strcmp(cmd, "cat")==0)
       my_cat(pathname);
    else if (strcmp(cmd, "cp")==0) {
       sscanf(line, "%s %s %s", cmd, pathname, pathname2);
       my_cp(pathname, pathname2); }
    else if (strcmp(cmd, "open")==0) {
      int d = -1;
      sscanf(line, "%s %s %d", cmd, pathname, &d);
       open_file(pathname, d);
    }
    else if (strcmp(cmd, "quit")==0)
       quit();
  }
}

void my_menu() {
   printf("\n************************\n");
   printf("\n[menu]\nprints out all possible commands and their descriptions\n");
   printf("\n[ls (optional directory name)]\nprints files and directories in the current working directory by default in a long list format\n");
   printf("\n[cd (directory name)]\nchanges the current working directory to the directory listed after this command. use ../ to go to the previous directory\n");
   printf("\n[pwd]\nprints the current working directory\n");
   printf("\n[mkdir (new directory name)]\nmakes a new directory\n");
   printf("\n[creat (new file name)]\ncreates a new file in the current working directory\n");
   printf("\n[rmdir (directory name to remove)]\nremoves an empty directory\n");
   printf("\n[link (filename1) (filename2)]\nhard links filename2 to filename1\n");
   printf("\n[unlink (filename)]\nunlinks filename\n");
   printf("\n[symlink (name1) (name2)]\nsoft links name2 to name1\n");
   printf("\n[cat (filename)]\nprints contents of filename\n");
   printf("\n[cp (filename1) (filename2)]\ncopies filename1 to filename2\n");
   printf("\n[quit]\nquits application\n");
   printf("\n************************\n");
}