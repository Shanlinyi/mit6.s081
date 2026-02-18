#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char* path,char* filename){
	char buf[512],*p;
	int fd;
	struct dirent de;
	struct stat st;
	if((fd=open(path,0))<0){
		printf("file can not open.");
		close(fd);
		return;
	}
	if(fstat(fd,&st)<0){
		printf("can not stat.");
		close(fd);
		return;
	}
	if(st.type != T_DIR){
		printf("usage:find <DIRECTORY><FILENAME>");
		return;
	}
	if(strlen(path)+1+DIRSIZ+1>sizeof(buf)){
		printf("path too long");
		return;
	}
	strcpy(buf,path);
	p=buf+strlen(buf);
	while(read(fd,&de,sizeof(de))){
		if(de.inum==0) continue;
		memmove(p,de.name,DIRSIZ);
		p[DIRSIZ]=0;
		if(stat(buf,&st)<0){
			printf("can not stat %s",buf);
			continue;
		}
		if(st.type==T_DIR&&strcmp(p,".")!=0&&strcmp(p,"..")!=0){
			find(buf,filename);
		}else{
			if(strcmp(buf,filename)==0){
				printf("%s\n",buf);
			}	
		}
	}
	close(fd);
}



int main(int argc,char **argv){
	if(argc<3){
		exit(0);
	}
	char target[512];
	target[0]='/';
	strcpy(target+1,argv[2]);
	find(argv[1],target);
	exit(0);
}
