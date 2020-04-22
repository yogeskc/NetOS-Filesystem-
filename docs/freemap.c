/*char *freemap = NULL;

// Freemap functions
void freemap_init(){
 freemap = malloc(BLOCKCOUNT / sizeof(char));
}

void freemap_cleanup(){
 free(freemap);
}*/

/* Example freemap
freemap = {
 0x00011111,
 0x11111111,
 0x11000000,
 0x00000000
};

* Freemap after running freemap_set(0, 4, 16)

freemap = {
 0x00000000,
 0x00000000,
 0x00000000,
 0x00000000
}; */
