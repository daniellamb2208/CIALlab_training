#define _CRT_SECURE_NO_WARNINGS
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
#include<limits.h>
////////////////////////////////////////////////////////////////////////////////////
//total size = 6byte
struct ENTRY{
	unsigned int ip; //ip最長為32bits = 4byte
	unsigned char len;//同理，1byte足夠表達0~255
	unsigned char port;//實際上是longest matching 的 prefix node name,大小為1byte = 8bit足夠表達0~255
};
////////////////////////////////////////////////////////////////////////////////////
struct list{//structure of binary trie
	unsigned int port;
	struct list *left,*right;
};
//將list結構命名為node
typedef struct list node;
//將node 令一個別名為*btrie
typedef node *btrie;
////////////////////////////////////////////////////////////////////////////////////
/*global variables*/
btrie root;//用於指向整棵樹的root
//query和table做相同的事情，唯一不同的是query會被suffle
struct ENTRY *table;
struct ENTRY *query;
struct ENTRY *input;
int num_entry = 0;
long long int level_node[32];
int N=0;//number of nodes
btrie create_node(){
	btrie temp;
	temp=(btrie)malloc(sizeof(node));
	temp->right=NULL;
	temp->left=NULL;
	temp->port=256;//default port, 256 meaning no name
	return temp;
}
////////////////////////////////////////////////////////////////////////////////////
void add_node(unsigned int ip,unsigned char len,unsigned char nexthop){
	btrie ptr=root;
	int i;
	for(i=0;i<len;i++){
        //mask後該bit == 1，從最高位元開始compare
		if(ip&(1<<(31-i))){
			if(ptr->right==NULL)
				ptr->right=create_node(); // Create Node
			ptr=ptr->right;
			if((i==len-1)&&(ptr->port==256))//lonest matching prefix
				ptr->port=nexthop;
		}
        //mask後該bit == 0
		else{
			if(ptr->left==NULL)
				ptr->left=create_node();
			ptr=ptr->left;
			if((i==len-1)&&(ptr->port==256))
				ptr->port=nexthop;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////
void read_table(char *str,unsigned int *ip,int *len,unsigned int *nexthop){
	char tok[]="./";//用於切割的token
	char buf[100],*str1;
	unsigned int n[4];
    //將前面四個ip位址切出來
	sprintf(buf,"%s",strtok(str,tok));
	n[0]=atoi(buf);
	sprintf(buf,"%s",strtok(NULL,tok));
	n[1]=atoi(buf);
	sprintf(buf,"%s",strtok(NULL,tok));
	n[2]=atoi(buf);
	sprintf(buf,"%s",strtok(NULL,tok));
	n[3]=atoi(buf);
    //*******why nexthop = n[2]?**********
	*nexthop=n[2];
	str1=(char *)strtok(NULL,tok);
    //輸入prefix length的長度
	if(str1!=NULL){
		sprintf(buf,"%s",str1);
		*len=atoi(buf);
	}
	else{
		if(n[1]==0&&n[2]==0&&n[3]==0)
			*len=8;
		else
			if(n[2]==0&&n[3]==0)
				*len=16;
			else
				if(n[3]==0)
					*len=24;
	}
    //輸入ip value
    *ip = n[0]<<24 | n[1]<<16 | n[2]<<8 | n[3];
}
////////////////////////////////////////////////////////////////////////////////////
void set_table(char *file_name){
	FILE *fp;
	int len;
	char string[100];
	unsigned int ip,nexthop;
	fp=fopen(file_name,"r");
	while(fgets(string,50,fp)!=NULL)
		num_entry++;
	
	rewind(fp);
	table=(struct ENTRY *)malloc(num_entry*sizeof(struct ENTRY));
	num_entry=0;
	while(fgets(string,50,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
		table[num_entry].ip=ip;
		table[num_entry].port=nexthop;
		table[num_entry++].len=len;
	}
}
////////////////////////////////////////////////////////////////////////////////////
void create(){
	int i;
	for (int i = 0; i < 32; i++)
		level_node[i] = 0;
	//begin=rdtsc();
	root = create_node();
	for (i = 0; i < num_entry; i++)
		add_node(table[i].ip, table[i].len, table[i].port);
	//end = rdtsc();
}
////////////////////////////////////////////////////////////////////////////////////
void count_levelnode(btrie r, int level)
{
	if (r == NULL)
		return;
	//any direction have node should created subnode
	//if both children is NULL, this node is leave
	if (r->left != NULL || r->right != NULL)
		level_node[level]++;
	count_levelnode(r->left, level + 1);
	count_levelnode(r->right, level + 1);
}

int _pow(int d, int n)
{
    int m = d;
    for(int i =0;i<n;i++)
        d*=m;
    return d;
}
////////////////////////////////////////////////////////////////////////////////////
void DP_cut()
{
	unsigned long long int cost[33][6];
	int cut_point[33][6];
	unsigned long long int temp_cost;
	int temp_cutpoint;
	int cut[3];
	//initialize
	for (int i = 0; i < 33; i++){
		for (int j = 0; j < 5; j++)
		{
			cost[i][j] = ULLONG_MAX;
			cut_point[i][j] = 0;
		}
	}
	//cost[i][0] 表示在level i以下每個node所需要消耗的cost(意思即為把level i以下的tree全部tanle化需要消耗的花費
	for (int i = 1; i < 33; i++)//1-32
	{
		cost[i][1] = _pow(2, i);
		cut_point[i][1] = i;
	}
	//DP count cost
	//from two part to four part 
	for (int j = 2; j < 5; j++){//2-4               
		for (int i = 1; i < 33; i++){//1-32         // highest level btire
			//now count all cost with cost[i][j]
			//index from j-1 to i-1 
			for (int k = j - 1; k < i; k++){//k=cut_point
                //               in table                               node(m)     * 2^(highest level btrie- cut point)
				temp_cost = cost[k][j - 1] + (unsigned long long int)(level_node[k] * _pow(2, i - k));
				temp_cutpoint = k;
				printf("now count cost[%d][%d], k:%d, cost[%d][%d]:%lld, level_node[%d]:%lld ", i, j, k, k, j - 1, cost[k][j - 1], k, level_node[k]);
				printf("pow:%d, tempcost:%lld, cutpoint:%d\n", (int)_pow(2, i - k), temp_cost,cut_point[i][j]);
				if (temp_cost < cost[i][j])
				{
					cost[i][j] = temp_cost;
					cut_point[i][j] = temp_cutpoint;
				}
			}
			printf("//\n");
		}
	}
	printf("\n**********************Best Cost******************************\n\n");
	for (int j = 1; j < 5; j++) {
		for (int i = 1; i < 33; i++) {
			printf("cost[%d][%d] = %llu, cutpoint = %d\n", i, j, cost[i][j], cut_point[i][j]);
		}
		printf("//\n");
	}
	cut[0] = cut_point[32][4];
	cut[1] = cut_point[cut[0]][3];
	cut[2] = cut_point[cut[1]][2];
    //int ccut = cut_point[cut[2]][2];
	printf("first cut: %d, second cut: %d, third cut: %d\n", cut[0], cut[1], cut[2]);
    //printf("%d\n", ccut);
}
////////////////////////////////////////////////////////////////////////////////////
int main(int argc,char *argv[]){
	int i,j;
	//char filename[100] = "dataset/ipv4_rrc_all.txt";
	set_table(argv[1]);
	create();
	count_levelnode(root, 0);
	for (int i = 0; i < 32; i++)
		printf("level:%d, node:%lld\n", i, level_node[i]);
	DP_cut();
	return 0;
}
