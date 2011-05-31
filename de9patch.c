#include <stdio.h>
#include <stdlib.h>

#define PNG_DEBUG 3
#include <png.h>

typedef struct npTc_block_t {
    char wasDeserialized;
    char numXDivs;
    char numYDivs;
    char colors;
    int XDivs[255];
    int YDivs[255];
    int padding_left;
    int padding_right;
    int padding_top;
    int padding_bottom;
} npTc_block;

int width, height;
png_bytep * row_pointers;
png_byte color_type;
png_byte bit_depth;


void usage(char * progname)
{
    printf("usage : %s in_filename out_filename\n", progname);
    exit(0);
}

void fail(char * message)
{
    fprintf(stderr, "%s\n", message);
	exit(10);
}

int read_chunk_custom(png_structp ptr, png_unknown_chunkp chunk) 
{
    printf("read_chunk_custom %s (size %d)\n", chunk->name, chunk->size);

    if (strcmp(chunk->name, "npTc")) 
	return 0;

    npTc_block * np_block = (npTc_block *)png_get_user_chunk_ptr(ptr);
    np_block->numXDivs = chunk->data[1];
    np_block->numYDivs = chunk->data[2];

    // paddings
    memcpy(&np_block->padding_left, &chunk->data[12], 4);
    memcpy(&np_block->padding_right, &chunk->data[16], 4);
    memcpy(&np_block->padding_bottom, &chunk->data[20], 4);
    memcpy(&np_block->padding_top, &chunk->data[24], 4);
    np_block->padding_left = ntohl(np_block->padding_left);
    np_block->padding_right = ntohl(np_block->padding_right);
    np_block->padding_top = ntohl(np_block->padding_top);
    np_block->padding_bottom = ntohl(np_block->padding_bottom);

    int i;    

    // XDivs
    for (i=0; i < np_block->numXDivs; i++) {
	memcpy(&np_block->XDivs[i], (chunk->data+32 + i*4), 4);
	np_block->XDivs[i] = ntohl(np_block->XDivs[i]);
    }

    // YDivs
    for (i=0; i < np_block->numYDivs; i++) {
	memcpy(&np_block->YDivs[i], (chunk->data+32 + (np_block->numXDivs + i)*4), 4);
	np_block->YDivs[i] = ntohl(np_block->YDivs[i]);
    }

    return 10;
}

void read_png_file(char * filename,  npTc_block * np_block)
{
    char header[8];
    int x, y; 

    png_structp png_ptr;
    png_infop info_ptr;
    int number_of_passes;
    
    /* open file and test for it being a png */
    FILE *fp = fopen(filename, "rb");
    if (!fp)
	fail("[read_png_file] File could not be opened for reading");
    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8))
	fail("[read_png_file] not recognized as a PNG file");
 
    /* initialize stuff */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    
    if (!png_ptr)
	fail("[read_png_file] png_create_read_struct failed");
    
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
	fail("[read_png_file] png_create_info_struct failed");
    
    if (setjmp(png_jmpbuf(png_ptr)))
	fail("[read_png_file] Error during init_io");

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);

    png_set_read_user_chunk_fn(png_ptr, np_block,  (png_user_chunk_ptr)read_chunk_custom);
    
    png_read_info(png_ptr, info_ptr);
    
    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);
    color_type = png_get_color_type(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    
    number_of_passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);
    
    
    /* read file */
    if (setjmp(png_jmpbuf(png_ptr)))
	fail("[read_png_file] Error during read_image");

    row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
    for (y=0; y<height; y++) 
	row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));
    
    png_read_image(png_ptr, row_pointers);
    
    fclose(fp);
}

void write_png_file(char* filename)
{
        /* create file */
        FILE *fp = fopen(filename, "wb");
        if (!fp)
	    fail("[write_png_file] File could not be opened for writing");

	png_structp png_ptr;
	png_infop info_ptr;
	int y;
   
        /* initialize stuff */
        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (!png_ptr)
                fail("[write_png_file] png_create_write_struct failed");

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
                fail("[write_png_file] png_create_info_struct failed");

        if (setjmp(png_jmpbuf(png_ptr)))
                fail("[write_png_file] Error during init_io");

        png_init_io(png_ptr, fp);

        /* write header */
        if (setjmp(png_jmpbuf(png_ptr)))
                fail("[write_png_file] Error during writing header");

        png_set_IHDR(png_ptr, info_ptr, width, height,
                     bit_depth, color_type, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

        png_write_info(png_ptr, info_ptr);


        /* write bytes */
        if (setjmp(png_jmpbuf(png_ptr)))
                fail("[write_png_file] Error during writing bytes");

        png_write_image(png_ptr, row_pointers);


        /* end write */
        if (setjmp(png_jmpbuf(png_ptr)))
                fail("[write_png_file] Error during end of write");

        png_write_end(png_ptr, NULL);

        /* cleanup heap allocation */
        for (y=0; y<height; y++)
                free(row_pointers[y]);
        free(row_pointers);

        fclose(fp);
}

void add_borders()
{
    void swap_columns(int col1, int col2)
    {
	int y;
	for (y=0; y<height; y++) {
	    png_byte tmp[4];
	    memcpy(tmp, &row_pointers[y][col1], 4);
	    memcpy(&row_pointers[y][col1], &row_pointers[y][col2], 4);
	    memcpy(&row_pointers[y][col2], tmp,4);
	}
    }

    int x,y;
    
    // extend width
    width +=2;
    for (y=0; y < height; y++) {
	row_pointers[y] = realloc(row_pointers[y], sizeof(png_byte)*width*4);
	memset(&row_pointers[y][(width-1)*4], 0, 4);
	memset(&row_pointers[y][(width-2)*4], 0, 4);
    }
    
    // center vertically
    for (x=width*4-4; x >0; x-=4)
	swap_columns(x, x-4); 

    // extend height
    height  +=2;
    row_pointers = (png_bytep*) realloc(row_pointers, sizeof(png_bytep) * height);
    row_pointers[height-2] =  malloc(sizeof(png_byte)*width*4);
    row_pointers[height-1] =  malloc(sizeof(png_byte)*width*4);
    memset(&row_pointers[height-2][0], 0, width*4);
    memset(&row_pointers[height-1][0], 0, width*4);

    // center horizontally
    for (y=height-2; y>0; y--) {
	png_bytep tmp = row_pointers[y];
	row_pointers[y] = row_pointers[y-1];
	row_pointers[y-1] = tmp;
    }
}

void add_patches(npTc_block * np_block)
{
    int i,j;

    // XDivs
    for (i=0; i < np_block->numXDivs; i+=2)
	for (j=np_block->XDivs[i]; j < np_block->XDivs[i+1]; j++)	{
	    png_bytep pix = &(row_pointers[0][(j+1)*4]);
	    pix[3] = 255;
	}

    // YDivs
    for (i=0; i < np_block->numYDivs; i+=2)
	for (j=np_block->YDivs[i]; j < np_block->YDivs[i+1]; j++)	{
	    png_bytep pix = &(row_pointers[(j+1)][0]);		
	    pix[3] = 255;	    
	}
}

void add_paddings(npTc_block * np_block)
{
    int i;
    
    //  padding left / right
    for (i = np_block->padding_left*4; i < (width - np_block->padding_right)*4; i+=4) {
	png_bytep pix = &(row_pointers[height-1][i]);
	pix[3] = 255;
    }

    //  padding bottom / top
    for (i = np_block->padding_bottom; i <= (height - np_block->padding_top); i++) {
	png_bytep pix = &(row_pointers[i][(width-1)*4]);
	pix[3] = 255;
    }

}

int main(int argc, char * argv[])
{
    if (argc < 3)
	usage(argv[0]);

    npTc_block np_block;

    read_png_file(argv[1], &np_block);
    
    add_borders();
    add_patches(&np_block);
    add_paddings(&np_block);
    
    write_png_file(argv[2]);

    /*    printf("padding left %d\n", np_block.padding_left);
    printf("padding right %d\n", np_block.padding_right);
    printf("padding top %d\n", np_block.padding_bottom);
    printf("padding bottom %d\n", np_block.padding_top);*/

    return 0;
}
