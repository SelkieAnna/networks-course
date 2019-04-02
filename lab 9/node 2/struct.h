#define MAX_FILENUMBER 				10

typedef struct _node{

    char nodename[255];
    char ip[255];
    int port;

} node;

typedef struct _node_and_files{

    char nodename[255];
    char ip[255];
    int port;

    int filenumber;
    char files[MAX_FILENUMBER][16];     // 10 files, 16-character names
    
} node_and_files;
