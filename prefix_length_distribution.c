#define _CRT_SECURE_NO_WARNINGS
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#define READ_STR_BUF 100
struct ENTRY{
	unsigned int ip; 
	unsigned char len;
	unsigned char port;
};
int n[33];
int error=0;
void extract_len(char *str){
    // Parameter
    // str, a line from dataset
	char tok[]="/";
	char buf[READ_STR_BUF];
	int len;
	sprintf(buf,"%s",strtok(str,tok));    // ip/len
	sprintf(buf,"%s",strtok(NULL,tok));   // x/len
	len = atoi(buf);
    if(len>0&&len<=32)
        n[atoi(buf)]++;
    else{
        printf("Error len: %s\n", buf);
        error++;
    }
}

int main(){
    char string[100];
    char *filename="dataset/ipv4_rrc_all.txt";
    FILE* fp=fopen(filename, "r");
    while(fgets(string, 99, fp)){
        extract_len(string);
    }
    int total=0;
    int max=n[0], index=0;
    for(int i=1;i<33;i++){
        total +=n[i];
        if(n[i]>max){ 
            max=n[i];
            index=i;
        }
        printf("%d ", n[i]);
    }
    printf("\n");
    for(int i=1;i<33;i++){
        printf("%.3f ", n[i]/(float)total*100);
    }
    printf("\n");
    printf("Max len dis: %d\n", max);
    printf("Max len index: %d\n", index);
    printf("%d\n", total);
    printf("%d\n", error);
    
}