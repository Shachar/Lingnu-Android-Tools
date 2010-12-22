#include <stdio.h>
#include <string.h>

#include "bootimg.h"

#define MIN_PAGESIZE 2048

#define min(a,b) ((a)>(b)?(b):(a))

int main( int argc, char *argv[] )
{
    if( argc<2 ) {
        fprintf( stderr, "Usage: split_bootimg boot.img\n" );

        return 1;
    }

    FILE *boot_img=fopen( argv[1], "rb" );
    if( boot_img==NULL ) {
        perror("Failed to open boot.img file");

        return 2;
    }

    boot_img_hdr header;
    if( fread( &header, sizeof(header), 1, boot_img )!=1 ) {
        fprintf(stderr, "Short read on file while reading header %u\n", sizeof(header));

        return 2;
    }

    // Is this the right type of file?
    if( strncmp( BOOT_MAGIC, header.magic, BOOT_MAGIC_SIZE )!=0 ) {
        fprintf( stderr, "Boot file header incorrect\n" );

        return 2;
    }

    // Dump the header information
    printf("Kernel size: %u\nKernel load address: %p\nRam disk size: %u\nRam disk load address: %p\nSecond size: %u\nSecond address: %p\n",
            header.kernel_size, header.kernel_addr, header.ramdisk_size, header.ramdisk_addr, header.second_size, header.second_addr );
    printf("Tags addr: %p\nPage size: %u\nUnused1: 0x%08x\nUnused2: 0x%08x\n\n", header.tags_addr, header.page_size,
            header.unused[0], header.unused[1] );

    printf("Product name: \"%s\"\nCommand line: \"%s\"\n", header.name, header.cmdline );
    printf("Id:");
    unsigned int i;
    for( i=0; i<8; ++i )
        printf(" 0x%08x", header.id[i]);
    printf("\n");

    // Skip forward to the beginning of the first page
    fseek( boot_img, MIN_PAGESIZE, SEEK_SET );

    // Sense out the actual page size
    unsigned int page_size=MIN_PAGESIZE;
    int ch;
    while( fgetc( boot_img )==0 )
        page_size++;

    page_size-=page_size%1024;

    printf("Sensed page size: %u\n", page_size );

    fseek( boot_img, page_size, SEEK_SET );

    char filename[4096];
    unsigned int filepart;
    for( filepart=strlen(argv[1])-1; filepart>0 && argv[1][filepart]!='/'; --filepart )
        ;
    if( argv[1][filepart]=='/' )
        ++filepart;

    // Write the kernel file
    snprintf( filename, sizeof(filename), "%s-kernel", argv[1]+filepart );
    FILE *outfile=fopen(filename, "wb");
    if( outfile==NULL ) {
        perror("Failed to open kernel output file");
        
        return 2;
    }

    size_t written;
    for( i=0; i<header.kernel_size; i+=written ) {
        unsigned char buffer[8196];

        written=fread( buffer, 1, min(sizeof(buffer), header.kernel_size-i), boot_img );
        if( written>0 ) {
            written=fwrite( buffer, 1, written, outfile );

            if( written==0 ) {
                printf("Written=%d\n", written );
                perror("Writing to kernel file failed");

                return 2;
            }
        } else if( written<0 ) {
            perror("Failed to read kernel from boot file");

            return 2;
        }
    }
    fclose(outfile);

    // Write the ramdisk file
    size_t lastpos=ftell( boot_img );
    if( (lastpos % page_size)!=0 ) {
        lastpos += page_size - (lastpos%page_size);

        fseek( boot_img, lastpos, SEEK_SET );
    }

    snprintf( filename, sizeof(filename), "%s-ramdisk.gz", argv[1]+filepart );
    outfile=fopen(filename, "wb");
    if( outfile==NULL ) {
        perror("Failed to open ramdisk output file");
        
        return 2;
    }

    for( i=0; i<header.ramdisk_size; i+=written ) {
        unsigned char buffer[8196];

        written=fread( buffer, 1, min(sizeof(buffer), header.ramdisk_size-i), boot_img );
        if( written>0 ) {
            written=fwrite( buffer, 1, written, outfile );

            if( written==0 ) {
                perror("Writing to ramdisk file failed");

                return 2;
            }
        } else if( written==0 ) {
            fprintf(stderr, "Short read of ramdisk image from boot file\n");

            return 2;
        } else {
            perror("Error reading ram disk from boot file");

            return 2;
        }
    }
    fclose(outfile);

    fclose( boot_img );
}
