#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
//#include <chrono>
//#include <iostream>
static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((unsigned long long) lo) | (((unsigned long long) hi) << 32);
}

struct ENTRY {
    unsigned int ip;
    unsigned char len;
    unsigned char port;
};

struct list {
    unsigned int port;
    struct list *node[16];
};
typedef struct list node;
typedef node *mtrie;

/*global variables*/
mtrie root;
/*unsigned int *query;*/
struct ENTRY *table, *query, *input;
//std::chrono::steady_clock::time_point c_begin, c_end;
unsigned long long int begin, end, total = 0;
unsigned long long int *my_clock;
int num_entry = 0;
int num_query = 0;
int num_input = 0;
int num_node = 0;  // total number of nodes in the binary trie
int N = 0;  // number of nodes

mtrie create_node()
{
    mtrie temp;
    num_node++;
    temp = (mtrie) malloc(sizeof(struct list));
    temp->port = 256;  // default port
    for (int i = 0; i < 16; i++)
        temp->node[i] = NULL;
    return temp;
}


void add_node(unsigned int ip, unsigned char len, unsigned char nexthop)
{
    mtrie ptr = root;
    int i;
    unsigned int n;
    for (i = 4; i < len + 4; i += 4) {
        // 10000000 00000000 00000000 00000000 >> 32-i(4) => 1000*
        // 1000 & 1111 => 1000 => 8
        n = (ip >> (32 - i)) & 0xf;
        // ptr->node[8] is where it should go
        if (!ptr->node[n])
            ptr->node[n] = create_node();
        ptr = ptr->node[n];
        // len is dealt and default port
        if ((len < (i + 4)) && (ptr->port == 256)) {
            ptr->port = nexthop;
        }
    }
}


void read_table(char *str, unsigned int *ip, int *len, unsigned int *nexthop)
{
    char tok[] = "./";
    char buf[100], *str1;
    unsigned int n[4];
    sprintf(buf, "%s", strtok(str, tok));
    n[0] = atoi(buf);
    sprintf(buf, "%s", strtok(NULL, tok));
    n[1] = atoi(buf);
    sprintf(buf, "%s", strtok(NULL, tok));
    n[2] = atoi(buf);
    sprintf(buf, "%s", strtok(NULL, tok));
    n[3] = atoi(buf);
    *nexthop = n[2];

    str1 = (char *) strtok(NULL, tok);
    if (str1 != NULL) {
        sprintf(buf, "%s", str1);
        *len = atoi(buf);
    } 
    else {
        *len = 32;
        if (n[1] == 0 && n[2] == 0 && n[3] == 0)
            *len = 8;
        else if (n[2] == 0 && n[3] == 0)
            *len = 16;
        else if (n[3] == 0)
            *len = 24;
    }
    *ip = n[0]<<24 | n[1]<<16 | n[2]<<8 | n[3];
}


void search(unsigned int ip)
{
    int j, n;
    mtrie current = root, temp = NULL;
    for (j = 7; j >= 0; j--) {
        if (current == NULL)
            break;

        n = (ip >> (j * 4)) & 0xf;
        current = current->node[n];
        if (current && current->port != 256)
            temp = current;
        /*else if(!current)*/
        /*break;*/
    }
    // if (temp == NULL)
    //    printf("default\n");
    // else
    //    printf("%u\n",temp->port);
}


void set_table(char *file_name)
{
    FILE *fp;
    int len;
    char string[100];
    unsigned int ip, nexthop;
    fp = fopen(file_name, "r");
    while (fgets(string, 50, fp) != NULL) 
        num_entry++;
    
    rewind(fp);
    table = (struct ENTRY *) malloc(num_entry * sizeof(struct ENTRY));
    num_entry = 0;
    while (fgets(string, 50, fp) != NULL) {
        read_table(string, &ip, &len, &nexthop);
        table[num_entry].ip = ip;
        table[num_entry].port = nexthop;
        table[num_entry++].len = len;
    }
}

void set_query(char *file_name)
{
    FILE *fp;
    int len;
    char string[100];
    unsigned int ip, nexthop;
    fp = fopen(file_name, "r");
    while (fgets(string, 50, fp) != NULL)
        num_query++;

    rewind(fp);
    query = (struct ENTRY *) malloc(num_query * sizeof(struct ENTRY));
    my_clock = (unsigned long long int *) malloc(num_query * sizeof(unsigned long long int));
    num_query = 0;
    while (fgets(string, 50, fp) != NULL) {
        read_table(string, &ip, &len, &nexthop);
        query[num_query].ip = ip;
        // query[num_query].port = nexthop;
        // query[num_query].len = len;
        my_clock[num_query++] = 10000000;
    }
}

void set_input(char *file_name) {
	FILE *fp;
	int len;
	char string[100];
	unsigned int ip, nexthop;
	fp = fopen(file_name, "r");
	while (fgets(string, 50, fp) != NULL)
		num_input++;
	
	rewind(fp);
	input = (struct ENTRY *)malloc(num_input * sizeof(struct ENTRY));
	num_input = 0;
	
	while (fgets(string, 50, fp) != NULL) {
		read_table(string, &ip, &len, &nexthop);
		input[num_input].ip = ip;
		input[num_input].port = nexthop;
		input[num_input++].len = len;
	}
}

void create()
{
    int i;
    begin = rdtsc();
    //c_begin = std::chrono::steady_clock::now();

    root = create_node();
    for (i = 0; i < num_entry; i++)
        add_node(table[i].ip, table[i].len, table[i].port);
    
    end = rdtsc();
    //c_end = std::chrono::steady_clock::now();
}

void count_node(mtrie r)
{
    if (r == NULL)
        return;
    for (int i = 0; i < 16; i++)
        count_node(r->node[i]);
    N++;
}

void CountClock()
{
    unsigned int i;
    unsigned int *NumCntClock = (unsigned int *) malloc(50 * sizeof(unsigned int));
    for (i = 0; i < 50; i++)
        NumCntClock[i] = 0;
    unsigned long long MinClock = 10000000, MaxClock = 0;
    for (i = 0; i < num_query; i++) {
        if (my_clock[i] > MaxClock)
            MaxClock = my_clock[i];
        if (my_clock[i] < MinClock)
            MinClock = my_clock[i];
        if (my_clock[i] / 100 < 50)
            NumCntClock[my_clock[i] / 100]++;
        else
            NumCntClock[49]++;
    }
    printf("(MaxClock, MinClock) =\t(%5llu, %5llu)\n", MaxClock, MinClock);

    for (i = 0; i < 50; i++) {
        printf("%d\n", NumCntClock[i]);
    }
    return;
}

void shuffle(struct ENTRY *array, int n)
{
    srand((unsigned) time(NULL));
    unsigned int ip; 
    unsigned char len, port;
    int i;
    for (i = 0; i < n - 1; i++) {
        size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
        ip = array[j].ip;
        len = array[j].len;
        port = array[j].port;
        array[j].ip = array[i].ip;
        array[j].len = array[i].len;
        array[j].port = array[i].port;
        array[i].ip = ip;
        array[i].len = len;
        array[i].port = port;
    }
}
void print_trie(struct list *tmp, int count)
{
    printf("layer: %d %d\n", count, tmp->port);
    int i;
    for (i = 0; i < 16; i++) {
        if (tmp->node[i] != NULL) {
            printf("num: %d ", i);
            print_trie(tmp->node[i], count + 1);
        }
    }
}


int main(int argc, char *argv[])
{
    /*if(argc!=3){
        printf("Please execute the file as the following way:\n");
        printf("%s  routing_table_file_name  query_table_file_name\n",argv[0]);
        exit(1);
    }*/
    int i, j;
    set_table(argv[1]);
    set_query(argv[2]);
    set_input(argv[3]);
    create();
    //std::cout <<"Avg. Build Time: " << (c_end - c_begin).count() / num_entry << std::endl;
    //printf("Avg. Build Time:\t%ld\n", (c_end - c_begin) / num_entry);
	printf("Avg. Build Time(rdtsc): %lld\n", (end - begin) / num_entry);

    printf("number of nodes created :%d\n", num_node);
    printf("Total memory requirement :%ld KB\n",
           ((num_node * sizeof(node)) / 1024));
    
    ////////////////////////////////////////////////////////////////////////////
    shuffle(query, num_entry);
    for (j = 0; j < 100; j++) {
        for (i = 0; i < num_query; i++) {
            //c_begin = std::chrono::steady_clock::now();
            begin=rdtsc();
            search(query[i].ip);
            //c_end = std::chrono::steady_clock::now();
            end=rdtsc();
            if (my_clock[i] > (end - begin))
                my_clock[i] = (end - begin);
        }
    }
    total = 0;
    for (j = 0; j < num_query; j++)
        total += my_clock[j];
    
    printf("Avg. Search: %llu\n", total / num_query);
    ////////////////////////////////////////////////////////////////////////////
    
    // update
    //c_begin = std::chrono::steady_clock::now();
    begin=rdtsc();
	for (int i =0;i<num_input;i++)
		add_node(input[i].ip, input[i].len, input[i].port);
	//c_end = std::chrono::steady_clock::now();
	//std::cout << "Avg. insert Time: " << (c_end - c_begin).count() / num_input << std::endl;
    end=rdtsc();
    printf("Avg. insert Time: %lld\n", (end - begin) / num_input);
	//num_node=0;
	count_node(root);
    //printf("%d\n", num_entry);
    //printf("%d\n", num_query);

    printf("Insert entry: %d\n", num_input);
    printf("After insertion\n");
    printf("There are %d nodes in fourthbit trie after insertion\n",N);
   	printf("Total memory requirement: %ld KB\n",((N*sizeof(node))/1024));
    CountClock();
    //print_trie(root, 0);
    return 0;
}