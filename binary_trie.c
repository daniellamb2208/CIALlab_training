#define _CRT_SECURE_NO_WARNINGS
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<time.h>
//#include <chrono>
//#include <iostream>
#define READ_STR_BUF 100

//Use to count computation time on linux sys 
static __inline__ unsigned long long rdtsc(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

struct ENTRY{
	unsigned int ip; 
	unsigned char len;
	unsigned char port;
};

struct list{
	unsigned int port;
	struct list *left,*right;
};
typedef struct list node;
typedef node *btrie;

/*global variables*/
//std::chrono::steady_clock::time_point c_begin, c_end;
unsigned long long int begin, end, total = 0;
unsigned long long int *_clock;
btrie root;
struct ENTRY *table;
struct ENTRY *query;
struct ENTRY *input;
int num_entry = 0;
int num_query = 0;
int num_input = 0;
int num_node = 0;


btrie create_node(){
	btrie temp;
	temp=(btrie)malloc(sizeof(node));
	temp->right=NULL;
	temp->left=NULL;
	temp->port=256;//default port is 8'b00000000
	return temp;
}

//Split one line and reassembly for ip format
void read_table(char *str,unsigned int *ip,int *len,unsigned int *nexthop){
    // Parameter
    // str, a line from dataset
	char tok[]="./";
	char buf[READ_STR_BUF],*str1;
	unsigned int n[4];//store ip value
	sprintf(buf,"%s",strtok(str,tok));    // o.x.x.x/x
	n[0]=atoi(buf);
	sprintf(buf,"%s",strtok(NULL,tok));   // x.)o.x.x/x
	n[1]=atoi(buf);
	sprintf(buf,"%s",strtok(NULL,tok));   // x.x.)o.x/x
	n[2]=atoi(buf);
	sprintf(buf,"%s",strtok(NULL,tok));   // x.x.x.)o/x
	n[3]=atoi(buf);
	*nexthop=n[2];//nexthop = n[2] just mean random nexthop value
	
    //deal with prefix length
	str1=(char *)strtok(NULL,tok);          // x.x.x.x/)o
	if(str1!=NULL){
		sprintf(buf,"%s",str1);
		*len=atoi(buf);
	}
	else{//exception situation
		*len=32;    // default?
        if(n[1]==0&&n[2]==0&&n[3]==0)
			*len=8;
		else
			if(n[2]==0&&n[3]==0)
				*len=16;
			else
				if(n[3]==0)
					*len=24;
	}
	//assign to ip with correct format

    *ip = n[0]<<24 | n[1]<<16 | n[2]<<8 | n[3];
    // + has higher percedence

}
void set_table(char *file_name){
    FILE *fp;
	int len;
	char string[READ_STR_BUF];
	unsigned int ip, nexthop;
	fp=fopen(file_name,"r");
	while(fgets(string,READ_STR_BUF-1,fp)!=NULL)
        //count table size
		num_entry++;
    
	rewind(fp);
	//allocate table space
	table=(struct ENTRY *)malloc(num_entry*sizeof(struct ENTRY));
	num_entry=0;
	//get table info
	while(fgets(string,READ_STR_BUF-1,fp)!=NULL){
		read_table(string,&ip,&len,&nexthop);
		table[num_entry].ip=ip;
		table[num_entry].port=nexthop;
		table[num_entry++].len=len;
	}
}

void set_query(char *file_name){
	FILE *fp;
	int len;
	char string[READ_STR_BUF];
	unsigned int ip, nexthop;
	fp=fopen(file_name,"r");
	while(fgets(string,50,fp)!=NULL)
		num_query++;

	rewind(fp);
	query=(struct ENTRY *)malloc(num_query*sizeof(struct ENTRY));
	//record start timestamp
	_clock=(unsigned long long int *)malloc(num_query*sizeof(unsigned long long int));
	num_query=0;
	//get table info, initialize _clock recorded block
	while(fgets(string, READ_STR_BUF, fp)!=NULL){
		read_table(string, &ip, &len, &nexthop);
        query[num_query].ip=ip;
        //query[num_query].port = nexthop;
        //query[num_query].len = len;
		_clock[num_query++]=10000000;
	}
}

void set_input(char *file_name) {
	FILE *fp;
	int len;
	char string[READ_STR_BUF];
	unsigned int ip, nexthop;
	fp = fopen(file_name, "r");
	while (fgets(string, 50, fp) != NULL)
		num_input++;
	
	rewind(fp);
	//allocate space
	input = (struct ENTRY *)malloc(num_input * sizeof(struct ENTRY));
	num_input = 0;
	
	while (fgets(string, 50, fp) != NULL) {
        //get table info
		read_table(string, &ip, &len, &nexthop);
		input[num_input].ip = ip;
		input[num_input].port = nexthop;
		input[num_input++].len = len;
	}
}

void add_node(unsigned int ip,unsigned char len,unsigned char nexthop){
	btrie ptr=root;
	int i;
	for(i=0;i<len;i++){
		// which bit from head, 1 mean right node, 0 mean left node
        // 255.255.255.255 & 128.0.0.0.0 => 128.0.0.0 (10000000 00000000 00000000 00000000)
        // 1<<(31-i) kinda filter to current bit
		if(ip&(1<<(31-i))){
			if(ptr->right==NULL)
            // child is NULL
				ptr->right=create_node(); 
			ptr=ptr->right;
			// leaf node
            // 255.255.255.255/1 mean 1*
            // Mark the nexthop when port is 256(default)
			if((i==len-1)&&(ptr->port==256))
				ptr->port=nexthop;
		}
		else{
			if(ptr->left==NULL)
				ptr->left=create_node();
			ptr=ptr->left;
			if((i==len-1)&&(ptr->port==256))
				ptr->port=nexthop;
		}
	}
}

//Create binary trie structure
void create(){
	int i;
	begin=rdtsc();//record start time
    //c_begin = std::chrono::steady_clock::now();
	root = create_node();
	for (i = 0; i < num_entry; i++)
		add_node(table[i].ip, table[i].len, table[i].port);
	end = rdtsc();//record end time
    //c_end = std::chrono::steady_clock::now();
}
//Count trie node with inorder
void count_node(btrie r){
	if(r==NULL)
		return;
	count_node(r->left);
	num_node++;
	count_node(r->right);
}
//Count distribution of computation time 
void CountClock()
{
	unsigned int i;
	unsigned int* NumCntClock = (unsigned int* )malloc(50 * sizeof(unsigned int ));
	for(i = 0; i < 50; i++) 
        NumCntClock[i] = 0;
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
		printf("%d\n",NumCntClock[i]);
	return;
}

//Table entry shuffle
void shuffle(struct ENTRY *array, int n){
    srand((unsigned)time(NULL));
    unsigned int ip; 
    unsigned char len, port;
    //struct ENTRY *temp=(struct ENTRY *)malloc(sizeof(struct ENTRY));
    // try to free memory or just using local variable
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

void search(unsigned int ip)
{
	int j;
	btrie current = root, temp = NULL;
	for(j=31;j>=(-1);j--){
		if(current==NULL)
			break;
		if(current->port!=256)
			temp=current;
		if(ip&(1<<j)){
			current=current->right;
		}
		else{
			current=current->left; 
		}
	}
	//inter print for debug
    /*
	if(temp==NULL)
	  printf("default\n");
    else
	  printf("%u\n",temp->port);
    */
}

int main(int argc,char *argv[]){
	int i,j;//index variable
	//argv is filename from input
	set_table(argv[1]);
	set_query(argv[2]);
	set_input(argv[3]);
	create();//build binary trie

    // std::cout <<"Avg. Build Time: " << (c_end - c_begin).count() / num_entry << std::endl;
	printf("Avg. Build Time(rdtsc): %lld\n", (end - begin) / num_entry);
	count_node(root);
	printf("number of nodes created: %d\n",num_node);
	printf("Total memory requirement: %ld KB\n",((num_node*sizeof(node))/1024));

	shuffle(query, num_query);

	//search operation
	for(j=0;j<100;j++){
		for(i=0;i<num_query;i++){
			//c_begin = std::chrono::steady_clock::now();
			begin=rdtsc();
            search(query[i].ip);
			//c_end = std::chrono::steady_clock::now();
			end=rdtsc();
			//if(_clock[i]>(c_end-c_begin).count())
			//	_clock[i]=(c_end-c_begin).count();
			if(_clock[i]>(end-begin))
				_clock[i]=(end-begin);
		}
	}

	total=0;
	for(j=0;j<num_query;j++)
		total+=_clock[j];
	
	printf("Avg. Search: %lld\n",total/num_query);

	//update operation
	begin=rdtsc();
	//c_begin = std::chrono::steady_clock::now();
	for (int i =0;i<num_input;i++)
		add_node(input[i].ip, input[i].len, input[i].port);
	end=rdtsc();
	//c_end = std::chrono::steady_clock::now();
	//std::cout << "Avg. insert Time: " << (c_end - c_begin).count() / num_input << std::endl;
    printf("Avg. insert Time:%lld\n", (end - begin) / num_input);
	num_node=0;
	count_node(root);

    printf("After insertion\n");
	printf("There are %d nodes in binary trie after insert\n", num_node);
	printf("Total memory requirement: %ld KB\n",((num_node*sizeof(node))/1024));
	//printf("%ld\n", sizeof(node));
	//printf("%d\n", num_node);
	CountClock();//detail _clock computation
    
    return 0;
}