#define _CRT_SECURE_NO_WARNINGS
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<time.h>
////////////////////////////////////////////////////////////////////////////////////
struct ENTRY{
	unsigned int ip;
	unsigned char len;
	unsigned char port;
};
////////////////////////////////////////////////////////////////////////////////////
static __inline__ unsigned long long rdtsc(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}
////////////////////////////////////////////////////////////////////////////////////
struct list{//structure of binary trie
	struct ENTRY node_info;
	struct list *left,*right;
	int ordinary;
};
//node是一種list的擴充型態
typedef struct list node;
typedef struct ENTRY entry;
//btrie的type為 node
typedef node *btrie;
////////////////////////////////////////////////////////////////////////////////////
/*global variables*/
btrie root;//用於指向整棵樹的root
//query和table做相同的事情，唯一不同的是query會被suffle
struct ENTRY *table;
struct ENTRY *query;
struct ENTRY *input;
int num_entry = 0;
int num_query = 0;
int num_input = 0;
int N=0;//number of nodes
unsigned long long int begin, end, total = 0;
unsigned long long int *_clock;
int num_node=0;//total number of nodes in the binary trie
////////////////////////////////////////////////////////////////////////////////////
btrie create_node(){
	btrie temp;
	num_node++;
	temp=(btrie)malloc(sizeof(node));
	temp->right=NULL;
	temp->left=NULL;
	temp->node_info.ip = 0;
	temp->node_info.len = 0;
	temp->node_info.port = 255;
	temp->ordinary = 0;
	return temp;
}
////////////////////////////////////////////////////////////////////////////////////
void add_node(entry now){
	btrie ptr=root;
	int ptr_prefix;
	int now_prefix;
	for (int i = 0; i <= now.len; i++)//level, depth
	{
		//insert to empty node
		if (ptr->node_info.ip == 0) {//default ip is 0
			ptr->node_info = now;// ip, len, port
			if (i >= now.len)
				ptr->ordinary = 1;
			break;// first action
		}
		//node is not empty
		else {
			//now_prefix = now.ip >> (31 - now.len);  // adding ip
			//ptr_prefix = ptr->node_info.ip >> (31 - now.len); // the ip already on ptrie, current node
			//insert priority node
			if ((i == now.len) && (ptr->ordinary == 0)) {// prefix == level && x is priority node
                // exchange
				entry temp = ptr->node_info;
				ptr->node_info = now;
				now = temp;
				ptr->ordinary = 1;
			}
			else if ((now.len > ptr->node_info.len)  && (ptr->ordinary == 0)) {
				entry temp = ptr->node_info;
				ptr->node_info = now;
				now = temp;
			}
		}
		//goto right
		if (now.ip&(1 << (31 - i))) {// ip & mask()
			if (ptr->right == NULL)
				ptr->right = create_node();
			ptr = ptr->right;
		}
		else {
			if (ptr->left == NULL)
				ptr->left = create_node();
			ptr = ptr->left;
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
        *len=32;
		if(n[1]==0&&n[2]==0&&n[3]==0)
			*len=8;
		else
			if(n[2]==0&&n[3]==0)
				*len=16;
			else
				if(n[3]==0)
					*len=24;
	}
    *ip = n[0]<<24 | n[1]<<16 | n[2]<<8 | n[3];
}
////////////////////////////////////////////////////////////////////////////////////
void search(unsigned int ip){
	int target = -1;
	int now_prefix;
	int ptr_prefix;
	btrie ptr = root;

	for(int i = 0 ;i<32;i++){
		if (ptr == NULL)
			break;
		now_prefix = ip >> (31 - ptr->node_info.len);
		ptr_prefix = ptr->node_info.ip >> (31 - ptr->node_info.len);
		if (now_prefix == ptr_prefix) {
			target = ptr->node_info.port;
			if (ptr->ordinary == 1)
				break;
		}
		if (ip&(1 << (31-i)))//go right-child
			ptr = ptr->right;
		else 
			ptr = ptr->left;
	}
	//printf("target:%d\n", target);    
}
////////////////////////////////////////////////////////////////////////////////////
void set_input(char *file_name) {
	FILE *fp;
	int len;
	char string[100];
	unsigned int ip, nexthop;
	fp = fopen(file_name, "r");
	while (fgets(string, 50, fp) != NULL)
		//將len, ip, nexthop參數傳址傳入，透過read table function取得值‘
		//每次輸入txt檔內的一行data
		num_input++;

	rewind(fp);
	//記憶體配置
	input = (struct ENTRY *)malloc(num_input * sizeof(struct ENTRY));//用來存放query
	num_input = 0;
	while (fgets(string, 50, fp) != NULL) {
		read_table(string, &ip, &len, &nexthop);
		input[num_input].ip = ip;
		input[num_input].port = nexthop;
		input[num_input++].len = len;
	}
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
void set_query(char *file_name){
	FILE *fp;
	int len;
	char string[100];
	unsigned int ip,nexthop;
	fp=fopen(file_name,"r");
	while(fgets(string,50,fp)!=NULL)
		num_query++;
	
	rewind(fp);
    //記憶體配置
	query=(struct ENTRY *)malloc(num_query*sizeof(struct ENTRY));//用來存放query
	_clock=(unsigned long long int *)malloc(num_query*sizeof(unsigned long long int));//用來存放每輪時間
	num_query=0;
	while(fgets(string,50,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
        query[num_query].ip=ip;
        query[num_query].port = nexthop;
        query[num_query].len = len;
		_clock[num_query++]=10000000;
	}
}
////////////////////////////////////////////////////////////////////////////////////
void create(){
	int i;
	begin = rdtsc();
	root=create_node();
	for(i=0;i<num_entry;i++)
		add_node(table[i]);
	end=rdtsc();
}
////////////////////////////////////////////////////////////////////////////////////
void count_node(btrie r){
	if(r==NULL)
		return;
	count_node(r->left);
	N++;
	count_node(r->right);
}
////////////////////////////////////////////////////////////////////////////////////
void CountClock()
{
	unsigned int i;
	unsigned int* NumCntClock = (unsigned int* )malloc(50 * sizeof(unsigned int ));
	for(i = 0; i < 50; i++) NumCntClock[i] = 0;
	unsigned long long MinClock = 10000000, MaxClock = 0;
	for(i = 0; i < num_query; i++)
	{
		if(_clock[i] > MaxClock) MaxClock = _clock[i];
		if(_clock[i] < MinClock) MinClock = _clock[i];
		if(_clock[i] / 100 < 50) NumCntClock[_clock[i] / 100]++;
		else NumCntClock[49]++;
	}
	printf("(MaxClock, MinClock) = (%5llu, %5llu)\n", MaxClock, MinClock);
	
	for(i = 0; i < 50; i++)
	{
		printf("%d\n",NumCntClock[i]);
	}
	return;
}
////////////////////////////////////////////////////////////////////////////////////
void shuffle(struct ENTRY *array, int n) {
    srand((unsigned)time(NULL));
    unsigned int ip;
	unsigned char len, port;
    for (int i = 0; i < n - 1; i++) {
        size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
        ip=array[j].ip;
        len=array[j].len;
        port=array[j].port;
        array[j].ip = array[i].ip;
        array[j].len = array[i].len;
        array[j].port = array[i].port;
        array[i].ip = ip;
        array[i].len = len;
        array[i].port = port;
    }
}
////////////////////////////////////////////////////////////////////////////////////
int compare(const void *a, const void *b)
{
	const struct ENTRY *x = a;
	const struct ENTRY *y = b;
	return (y->len > x->len);// from big to little
}
int main(int argc,char *argv[]){
	int i,j;

	set_table(argv[1]);
	set_query(argv[2]);
	set_input(argv[3]);
	qsort(table, num_entry, sizeof(table[0]), compare);
    //for(int k=0;k<num_entry;k++)
    //    printf("%d ", table[k].len);
	create();
	
	printf("Avg. build: %llu\n",(end-begin)/num_entry);
	printf("number of nodes: %d\n",num_node);
	printf("Total memory requirement: %ld KB\n",((num_node*sizeof(node))/1024));

	shuffle(query, num_query);//洗亂query順序
	////////////////////////////////////////////////////////////////////////////
    //連續做100次Search取平均時間
	for(j=0;j<100;j++){
		for(i=0;i<num_query;i++){
			begin=rdtsc();
            search(query[i].ip);
			end=rdtsc();
			if(_clock[i]>(end-begin))
				_clock[i]=(end-begin);
		}
	}
	total=0;
	for(j=0;j<num_query;j++)
		total+=_clock[j];
	printf("Avg. Search: %llu\n",total/num_query);
	CountClock();//計算耗費最多時間，最少時間，計算所需cycle數的分佈情況
	////////////////////////////////////////////////////////////////////////////
	begin = rdtsc();
	for (int i =0;i<num_input;i++)
		add_node(input[i]);
	end = rdtsc();
	printf("Avg. insert Time:%llu\n", (end - begin) / num_input);
	return 0;
}
